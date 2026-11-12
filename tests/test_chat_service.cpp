#include "localrag/app/ChatService.h"
#include "localrag/infra/FakeChatClient.h"
#include "localrag/infra/FakeEmbeddingClient.h"
#include "localrag/infra/InMemoryStorage.h"
#include "localrag/infra/InMemoryVectorIndex.h"
#include <cassert>

namespace {

class FixedEmbeddingClient final : public localrag::IEmbeddingClient {
public:
    std::vector<float> embed(const std::string&) override {
        return {1.0F, 0.0F};
    }
};

} // namespace

void test_chat_service_builds_answer_and_saves_logs() {
    localrag::InMemoryStorage storage;
    storage.seedChunk(localrag::ChunkRecord{101, 1, 0, "RAG 是检索增强生成。", 10, ""});

    localrag::FakeEmbeddingClient embedding;
    localrag::FakeChatClient chat;
    localrag::InMemoryVectorIndex index;
    index.add(101, embedding.embed("RAG 是检索增强生成。"));

    localrag::ChatService service(storage, embedding, chat, index);
    const auto answer = service.ask("什么是 RAG？", 1);

    assert(answer.find("MVP 模拟回答") != std::string::npos);
    assert(storage.messageCount() == 2);
    assert(storage.retrievalLogCount() == 1);
}

void test_chat_service_recovers_keyword_match_when_vector_rank_is_wrong() {
    localrag::InMemoryStorage storage;
    storage.seedChunk(localrag::ChunkRecord{101, 1, 0, "切片长度和重叠应该怎么设置？", 20, ""});
    storage.seedChunk(localrag::ChunkRecord{202, 1, 1, "LocalRAG 的检索链路是：提问、召回、重排，再交给模型生成回答。", 40, ""});

    FixedEmbeddingClient embedding;
    localrag::FakeChatClient chat;
    localrag::InMemoryVectorIndex index;
    index.add(101, {1.0F, 0.0F});
    index.add(202, {0.1F, 0.0F});

    localrag::ChatService service(storage, embedding, chat, index);
    const auto answer = service.ask("请说明 LocalRAG 的检索链路", 1);

    assert(answer.find("提问、召回、重排") != std::string::npos);
}
