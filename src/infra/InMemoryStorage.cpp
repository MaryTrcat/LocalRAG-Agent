#include "localrag/infra/InMemoryStorage.h"
#include <algorithm>

namespace localrag {

int64_t InMemoryStorage::createDocument(const DocumentRecord& document) {
    auto copy = document;
    copy.id = nextDocumentId_++;
    documents_.push_back(copy);
    return copy.id;
}

std::vector<DocumentRecord> InMemoryStorage::listDocuments() {
    return documents_;
}

int64_t InMemoryStorage::createChunk(const ChunkRecord& chunk) {
    auto copy = chunk;
    copy.id = nextChunkId_++;
    chunks_.push_back(copy);
    return copy.id;
}

std::vector<ChunkRecord> InMemoryStorage::getChunksByIds(const std::vector<int64_t>& ids) {
    std::vector<ChunkRecord> result;
    for (const auto id : ids) {
        const auto found = std::find_if(chunks_.begin(), chunks_.end(), [id](const ChunkRecord& chunk) {
            return chunk.id == id;
        });
        if (found != chunks_.end()) {
            result.push_back(*found);
        }
    }
    return result;
}

std::vector<ChunkRecord> InMemoryStorage::getAllChunks() {
    return chunks_;
}

int64_t InMemoryStorage::createConversation(const ConversationRecord& conversation) {
    auto copy = conversation;
    copy.id = nextConversationId_++;
    conversations_.push_back(copy);
    return copy.id;
}

int64_t InMemoryStorage::createMessage(const ChatMessage& message) {
    auto copy = message;
    copy.id = nextMessageId_++;
    messages_.push_back(copy);
    return copy.id;
}

void InMemoryStorage::createRetrievalLog(const RetrievalLogRecord& log) {
    auto copy = log;
    copy.id = nextRetrievalLogId_++;
    retrievalLogs_.push_back(copy);
}

void InMemoryStorage::clearAll() {
    retrievalLogs_.clear();
    messages_.clear();
    conversations_.clear();
    chunks_.clear();
    documents_.clear();
    nextDocumentId_ = 1;
    nextChunkId_ = 1;
    nextConversationId_ = 1;
    nextMessageId_ = 1;
    nextRetrievalLogId_ = 1;
}

void InMemoryStorage::seedChunk(const ChunkRecord& chunk) {
    chunks_.push_back(chunk);
    nextChunkId_ = std::max(nextChunkId_, chunk.id + 1);
}

std::size_t InMemoryStorage::documentCount() const {
    return documents_.size();
}

std::size_t InMemoryStorage::chunkCount() const {
    return chunks_.size();
}

std::size_t InMemoryStorage::messageCount() const {
    return messages_.size();
}

std::size_t InMemoryStorage::retrievalLogCount() const {
    return retrievalLogs_.size();
}

} // namespace localrag
