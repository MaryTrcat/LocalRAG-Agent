#pragma once

#include "localrag/core/ChatMessage.h"
#include "localrag/core/ChunkRecord.h"
#include "localrag/core/DocumentRecord.h"
#include <cstdint>
#include <string>
#include <vector>

namespace localrag {

struct ConversationRecord {
    int64_t id = 0;
    std::string title;
    std::string createdAt;
    std::string updatedAt;
};

struct RetrievalLogRecord {
    int64_t id = 0;
    int64_t conversationId = 0;
    int64_t messageId = 0;
    int64_t chunkId = 0;
    int rank = 0;
    float score = 0.0F;
    std::string createdAt;
};

class IStorage {
public:
    virtual ~IStorage() = default;
    virtual int64_t createDocument(const DocumentRecord& document) = 0;
    virtual std::vector<DocumentRecord> listDocuments() = 0;
    virtual int64_t createChunk(const ChunkRecord& chunk) = 0;
    virtual std::vector<ChunkRecord> getChunksByIds(const std::vector<int64_t>& ids) = 0;
    virtual std::vector<ChunkRecord> getAllChunks() = 0;
    virtual int64_t createConversation(const ConversationRecord& conversation) = 0;
    virtual int64_t createMessage(const ChatMessage& message) = 0;
    virtual void createRetrievalLog(const RetrievalLogRecord& log) = 0;
    virtual void clearAll() = 0;
};

} // namespace localrag
