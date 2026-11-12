#include "localrag/infra/SQLiteStorage.h"
#include <sstream>
#include <stdexcept>
#include <utility>

namespace localrag {

namespace {

void checkSqlite(int rc, sqlite3* db, const std::string& operation) {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        const char* message = db ? sqlite3_errmsg(db) : "unknown sqlite error";
        throw std::runtime_error(operation + " failed: " + message);
    }
}

std::string columnText(sqlite3_stmt* stmt, int column) {
    const auto* text = sqlite3_column_text(stmt, column);
    return text ? reinterpret_cast<const char*>(text) : "";
}

void bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    checkSqlite(sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT), nullptr, "sqlite bind text");
}

} // namespace

SQLiteStorage::SQLiteStorage(std::string dbPath)
    : dbPath_(std::move(dbPath)) {}

SQLiteStorage::~SQLiteStorage() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void SQLiteStorage::initialize() {
    db();
    execute("PRAGMA foreign_keys = ON;");
    execute(
        "CREATE TABLE IF NOT EXISTS documents ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "file_path TEXT NOT NULL,"
        "file_type TEXT NOT NULL,"
        "content_hash TEXT NOT NULL,"
        "created_at TEXT NOT NULL"
        ");");
    execute(
        "CREATE TABLE IF NOT EXISTS chunks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "document_id INTEGER NOT NULL,"
        "chunk_index INTEGER NOT NULL,"
        "content TEXT NOT NULL,"
        "token_estimate INTEGER NOT NULL,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY (document_id) REFERENCES documents(id)"
        ");");
    execute(
        "CREATE TABLE IF NOT EXISTS conversations ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ");");
    execute(
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "conversation_id INTEGER NOT NULL,"
        "role TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY (conversation_id) REFERENCES conversations(id)"
        ");");
    execute(
        "CREATE TABLE IF NOT EXISTS retrieval_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "conversation_id INTEGER NOT NULL,"
        "message_id INTEGER NOT NULL,"
        "chunk_id INTEGER NOT NULL,"
        "rank INTEGER NOT NULL,"
        "score REAL NOT NULL,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY (conversation_id) REFERENCES conversations(id),"
        "FOREIGN KEY (message_id) REFERENCES messages(id),"
        "FOREIGN KEY (chunk_id) REFERENCES chunks(id)"
        ");");
}

int64_t SQLiteStorage::createDocument(const DocumentRecord& document) {
    const char* sql = "INSERT INTO documents(title, file_path, file_type, content_hash, created_at) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare createDocument");
    bindText(stmt, 1, document.title);
    bindText(stmt, 2, document.filePath);
    bindText(stmt, 3, document.fileType);
    bindText(stmt, 4, document.contentHash);
    bindText(stmt, 5, document.createdAt);
    checkSqlite(sqlite3_step(stmt), db(), "createDocument");
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db());
}

void SQLiteStorage::deleteDocument(int64_t documentId) {
    sqlite3_stmt* stmt = nullptr;
    const auto prepare = [&](const char* sql) {
        checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare deleteDocument");
        checkSqlite(sqlite3_bind_int64(stmt, 1, documentId), db(), "bind deleteDocument document id");
        checkSqlite(sqlite3_step(stmt), db(), "deleteDocument");
        sqlite3_finalize(stmt);
        stmt = nullptr;
    };

    prepare("DELETE FROM retrieval_logs WHERE chunk_id IN (SELECT id FROM chunks WHERE document_id = ?);");
    prepare("DELETE FROM chunks WHERE document_id = ?;");
    prepare("DELETE FROM documents WHERE id = ?;");
}

std::vector<DocumentRecord> SQLiteStorage::listDocuments() {
    std::vector<DocumentRecord> documents;
    const char* sql = "SELECT id, title, file_path, file_type, content_hash, created_at FROM documents ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare listDocuments");

    while (true) {
        const auto rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            documents.push_back(DocumentRecord{
                sqlite3_column_int64(stmt, 0),
                columnText(stmt, 1),
                columnText(stmt, 2),
                columnText(stmt, 3),
                columnText(stmt, 4),
                columnText(stmt, 5)
            });
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        sqlite3_finalize(stmt);
        checkSqlite(rc, db(), "listDocuments");
    }

    sqlite3_finalize(stmt);
    return documents;
}

int64_t SQLiteStorage::createChunk(const ChunkRecord& chunk) {
    const char* sql = "INSERT INTO chunks(document_id, chunk_index, content, token_estimate, created_at) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare createChunk");
    checkSqlite(sqlite3_bind_int64(stmt, 1, chunk.documentId), db(), "bind chunk document_id");
    checkSqlite(sqlite3_bind_int(stmt, 2, chunk.chunkIndex), db(), "bind chunk index");
    bindText(stmt, 3, chunk.content);
    checkSqlite(sqlite3_bind_int(stmt, 4, chunk.tokenEstimate), db(), "bind chunk token_estimate");
    bindText(stmt, 5, chunk.createdAt);
    checkSqlite(sqlite3_step(stmt), db(), "createChunk");
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db());
}

std::vector<ChunkRecord> SQLiteStorage::getChunksByIds(const std::vector<int64_t>& ids) {
    std::vector<ChunkRecord> chunks;
    const char* sql = "SELECT id, document_id, chunk_index, content, token_estimate, created_at FROM chunks WHERE id = ?;";

    for (const auto id : ids) {
        sqlite3_stmt* stmt = nullptr;
        checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare getChunksByIds");
        checkSqlite(sqlite3_bind_int64(stmt, 1, id), db(), "bind chunk id");
        const auto rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            chunks.push_back(ChunkRecord{
                sqlite3_column_int64(stmt, 0),
                sqlite3_column_int64(stmt, 1),
                sqlite3_column_int(stmt, 2),
                columnText(stmt, 3),
                sqlite3_column_int(stmt, 4),
                columnText(stmt, 5)
            });
        } else if (rc != SQLITE_DONE) {
            checkSqlite(rc, db(), "getChunksByIds");
        }
        sqlite3_finalize(stmt);
    }

    return chunks;
}

std::vector<ChunkRecord> SQLiteStorage::getAllChunks() {
    std::vector<ChunkRecord> chunks;
    const char* sql = "SELECT id, document_id, chunk_index, content, token_estimate, created_at FROM chunks ORDER BY id;";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare getAllChunks");

    while (true) {
        const auto rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            chunks.push_back(ChunkRecord{
                sqlite3_column_int64(stmt, 0),
                sqlite3_column_int64(stmt, 1),
                sqlite3_column_int(stmt, 2),
                columnText(stmt, 3),
                sqlite3_column_int(stmt, 4),
                columnText(stmt, 5)
            });
            continue;
        }
        if (rc == SQLITE_DONE) {
            break;
        }
        sqlite3_finalize(stmt);
        checkSqlite(rc, db(), "getAllChunks");
    }

    sqlite3_finalize(stmt);
    return chunks;
}

int64_t SQLiteStorage::createConversation(const ConversationRecord& conversation) {
    const char* sql = "INSERT INTO conversations(title, created_at, updated_at) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare createConversation");
    bindText(stmt, 1, conversation.title);
    bindText(stmt, 2, conversation.createdAt);
    bindText(stmt, 3, conversation.updatedAt);
    checkSqlite(sqlite3_step(stmt), db(), "createConversation");
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db());
}

int64_t SQLiteStorage::createMessage(const ChatMessage& message) {
    const char* sql = "INSERT INTO messages(conversation_id, role, content, created_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare createMessage");
    checkSqlite(sqlite3_bind_int64(stmt, 1, message.conversationId), db(), "bind message conversation_id");
    bindText(stmt, 2, message.role);
    bindText(stmt, 3, message.content);
    bindText(stmt, 4, message.createdAt);
    checkSqlite(sqlite3_step(stmt), db(), "createMessage");
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(db());
}

void SQLiteStorage::createRetrievalLog(const RetrievalLogRecord& log) {
    const char* sql = "INSERT INTO retrieval_logs(conversation_id, message_id, chunk_id, rank, score, created_at) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(db(), sql, -1, &stmt, nullptr), db(), "prepare createRetrievalLog");
    checkSqlite(sqlite3_bind_int64(stmt, 1, log.conversationId), db(), "bind log conversation_id");
    checkSqlite(sqlite3_bind_int64(stmt, 2, log.messageId), db(), "bind log message_id");
    checkSqlite(sqlite3_bind_int64(stmt, 3, log.chunkId), db(), "bind log chunk_id");
    checkSqlite(sqlite3_bind_int(stmt, 4, log.rank), db(), "bind log rank");
    checkSqlite(sqlite3_bind_double(stmt, 5, log.score), db(), "bind log score");
    bindText(stmt, 6, log.createdAt);
    checkSqlite(sqlite3_step(stmt), db(), "createRetrievalLog");
    sqlite3_finalize(stmt);
}

void SQLiteStorage::clearAll() {
    execute("DELETE FROM retrieval_logs;");
    execute("DELETE FROM messages;");
    execute("DELETE FROM conversations;");
    execute("DELETE FROM chunks;");
    execute("DELETE FROM documents;");
    execute("DELETE FROM sqlite_sequence WHERE name IN ('retrieval_logs', 'messages', 'conversations', 'chunks', 'documents');");
}

std::size_t SQLiteStorage::documentCount() const {
    return countRows("documents");
}

std::size_t SQLiteStorage::chunkCount() const {
    return countRows("chunks");
}

std::size_t SQLiteStorage::messageCount() const {
    return countRows("messages");
}

std::size_t SQLiteStorage::retrievalLogCount() const {
    return countRows("retrieval_logs");
}

sqlite3* SQLiteStorage::db() {
    if (db_ == nullptr) {
        checkSqlite(sqlite3_open(dbPath_.c_str(), &db_), db_, "open database");
    }
    return db_;
}

const sqlite3* SQLiteStorage::db() const {
    return db_;
}

void SQLiteStorage::execute(const std::string& sql) const {
    char* error = nullptr;
    const auto rc = sqlite3_exec(const_cast<sqlite3*>(db()), sql.c_str(), nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        std::string message = error ? error : "unknown sqlite error";
        sqlite3_free(error);
        throw std::runtime_error("execute sql failed: " + message);
    }
}

std::size_t SQLiteStorage::countRows(const std::string& tableName) const {
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM " << tableName << ";";
    sqlite3_stmt* stmt = nullptr;
    checkSqlite(sqlite3_prepare_v2(const_cast<sqlite3*>(db()), sql.str().c_str(), -1, &stmt, nullptr), const_cast<sqlite3*>(db()), "prepare countRows");
    checkSqlite(sqlite3_step(stmt), const_cast<sqlite3*>(db()), "countRows");
    const auto count = static_cast<std::size_t>(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);
    return count;
}

} // namespace localrag
