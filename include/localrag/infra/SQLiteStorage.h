#pragma once

#include "localrag/core/IStorage.h"
#include <sqlite3.h>
#include <string>

namespace localrag {

class SQLiteStorage final : public IStorage {
public:
    explicit SQLiteStorage(std::string dbPath);
    ~SQLiteStorage() override;

    SQLiteStorage(const SQLiteStorage&) = delete;
    SQLiteStorage& operator=(const SQLiteStorage&) = delete;

    void initialize();

    int64_t createDocument(const DocumentRecord& document) override;
    void deleteDocument(int64_t documentId);
    std::vector<DocumentRecord> listDocuments() override;
    int64_t createChunk(const ChunkRecord& chunk) override;
    std::vector<ChunkRecord> getChunksByIds(const std::vector<int64_t>& ids) override;
    std::vector<ChunkRecord> getAllChunks() override;
    int64_t createConversation(const ConversationRecord& conversation) override;
    int64_t createMessage(const ChatMessage& message) override;
    void createRetrievalLog(const RetrievalLogRecord& log) override;
    void clearAll() override;

    std::size_t documentCount() const;
    std::size_t chunkCount() const;
    std::size_t messageCount() const;
    std::size_t retrievalLogCount() const;

private:
    sqlite3* db();
    const sqlite3* db() const;
    void execute(const std::string& sql) const;
    std::size_t countRows(const std::string& tableName) const;

    std::string dbPath_;
    sqlite3* db_ = nullptr;
};

} // namespace localrag
