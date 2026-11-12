#pragma once

#include "localrag/core/IStorage.h"
#include <vector>

namespace localrag {

class InMemoryStorage final : public IStorage {
public:
    int64_t createDocument(const DocumentRecord& document) override;
    std::vector<DocumentRecord> listDocuments() override;
    int64_t createChunk(const ChunkRecord& chunk) override;
    std::vector<ChunkRecord> getChunksByIds(const std::vector<int64_t>& ids) override;
    std::vector<ChunkRecord> getAllChunks() override;
    int64_t createConversation(const ConversationRecord& conversation) override;
    int64_t createMessage(const ChatMessage& message) override;
    void createRetrievalLog(const RetrievalLogRecord& log) override;
    void clearAll() override;

    void seedChunk(const ChunkRecord& chunk);
    std::size_t documentCount() const;
    std::size_t chunkCount() const;
    std::size_t messageCount() const;
    std::size_t retrievalLogCount() const;

private:
    int64_t nextDocumentId_ = 1;
    int64_t nextChunkId_ = 1;
    int64_t nextConversationId_ = 1;
    int64_t nextMessageId_ = 1;
    int64_t nextRetrievalLogId_ = 1;
    std::vector<DocumentRecord> documents_;
    std::vector<ChunkRecord> chunks_;
    std::vector<ConversationRecord> conversations_;
    std::vector<ChatMessage> messages_;
    std::vector<RetrievalLogRecord> retrievalLogs_;
};

} // namespace localrag
