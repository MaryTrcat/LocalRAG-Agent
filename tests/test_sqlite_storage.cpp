#include "localrag/infra/SQLiteStorage.h"
#include <cassert>
#include <filesystem>

void test_sqlite_storage_persists_documents_chunks_and_logs() {
    const auto dbPath = std::filesystem::temp_directory_path() / "localrag_storage_test.db";
    std::filesystem::remove(dbPath);

    {
        localrag::SQLiteStorage storage(dbPath.string());
        storage.initialize();

        const auto documentId = storage.createDocument(localrag::DocumentRecord{0, "RAG 文档", "rag.md", "md", "hash", "2026-06-29"});
        const auto secondDocumentId = storage.createDocument(localrag::DocumentRecord{0, "另一份资料", "other.txt", "txt", "hash-2", "2026-06-29"});
        const auto documents = storage.listDocuments();
        const auto chunkId = storage.createChunk(localrag::ChunkRecord{0, documentId, 0, "RAG 是检索增强生成。", 10, "2026-06-29"});
        storage.createChunk(localrag::ChunkRecord{0, secondDocumentId, 0, "独立资料内容。", 8, "2026-06-29"});
        const auto chunks = storage.getChunksByIds({chunkId});

        assert(documentId > 0);
        assert(documents.size() == 2);
        assert(documents[0].title == "RAG 文档");
        assert(chunkId > 0);
        assert(chunks.size() == 1);
        assert(chunks[0].content == "RAG 是检索增强生成。");

        const auto conversationId = storage.createConversation(localrag::ConversationRecord{0, "测试会话", "2026-06-29", "2026-06-29"});
        const auto messageId = storage.createMessage(localrag::ChatMessage{0, conversationId, "user", "什么是 RAG？", "2026-06-29"});
        storage.createRetrievalLog(localrag::RetrievalLogRecord{0, conversationId, messageId, chunkId, 1, 0.9F, "2026-06-29"});

        assert(storage.documentCount() == 2);
        assert(storage.chunkCount() == 2);
        assert(storage.messageCount() == 1);
        assert(storage.retrievalLogCount() == 1);

        storage.deleteDocument(documentId);
        assert(storage.documentCount() == 1);
        assert(storage.chunkCount() == 1);
        assert(storage.retrievalLogCount() == 0);

        storage.clearAll();
        assert(storage.documentCount() == 0);
        assert(storage.chunkCount() == 0);
        assert(storage.messageCount() == 0);
        assert(storage.retrievalLogCount() == 0);
    }

    std::filesystem::remove(dbPath);
}
