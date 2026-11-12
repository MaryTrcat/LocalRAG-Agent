#include "localrag/app/ChatService.h"
#include "localrag/app/RetrievalService.h"
#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using localrag::ChunkRecord;

std::string lowerAscii(std::string value) {
    for (auto& character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool isUtf8Continuation(unsigned char value) {
    return (value & 0xC0U) == 0x80U;
}

std::size_t utf8CodePointLength(const std::string& text, std::size_t index) {
    const auto first = static_cast<unsigned char>(text[index]);
    if (first < 0x80U) return 1;
    if ((first & 0xE0U) == 0xC0U && index + 1 < text.size() && isUtf8Continuation(static_cast<unsigned char>(text[index + 1]))) return 2;
    if ((first & 0xF0U) == 0xE0U && index + 2 < text.size() && isUtf8Continuation(static_cast<unsigned char>(text[index + 1])) && isUtf8Continuation(static_cast<unsigned char>(text[index + 2]))) return 3;
    if ((first & 0xF8U) == 0xF0U && index + 3 < text.size() && isUtf8Continuation(static_cast<unsigned char>(text[index + 1])) && isUtf8Continuation(static_cast<unsigned char>(text[index + 2])) && isUtf8Continuation(static_cast<unsigned char>(text[index + 3]))) return 4;
    return 1;
}

std::vector<std::string> lexicalTerms(std::string_view text) {
    std::vector<std::string> terms;
    std::string asciiWord;
    std::vector<std::string> unicodeRun;

    const auto flushAscii = [&]() {
        if (asciiWord.size() >= 2) terms.push_back(lowerAscii(asciiWord));
        asciiWord.clear();
    };
    const auto flushUnicode = [&]() {
        for (std::size_t index = 1; index < unicodeRun.size(); ++index) {
            terms.push_back(unicodeRun[index - 1] + unicodeRun[index]);
        }
        unicodeRun.clear();
    };

    const std::string value(text);
    for (std::size_t index = 0; index < value.size();) {
        const auto byte = static_cast<unsigned char>(value[index]);
        if (byte < 0x80U) {
            if (std::isalnum(byte)) {
                flushUnicode();
                asciiWord.push_back(static_cast<char>(byte));
            } else {
                flushAscii();
                flushUnicode();
            }
            ++index;
            continue;
        }

        flushAscii();
        const auto length = utf8CodePointLength(value, index);
        unicodeRun.push_back(value.substr(index, length));
        index += length;
    }
    flushAscii();
    flushUnicode();
    return terms;
}

std::string lowerAsciiCopy(std::string value) {
    return lowerAscii(std::move(value));
}

float lexicalScore(std::string_view question, const ChunkRecord& chunk) {
    const auto terms = lexicalTerms(question);
    if (terms.empty()) return 0.0F;

    const auto content = lowerAsciiCopy(chunk.content);
    float score = 0.0F;
    for (const auto& term : terms) {
        if (content.find(term) != std::string::npos) {
            score += term.size() >= 3 ? 2.0F : 1.0F;
        }
    }
    return score;
}

} // namespace

namespace localrag {

ChatService::ChatService(IStorage& storage, IEmbeddingClient& embeddingClient, IChatClient& chatClient, IVectorIndex& vectorIndex)
    : storage_(storage), embeddingClient_(embeddingClient), chatClient_(chatClient), vectorIndex_(vectorIndex) {}

std::string ChatService::ask(const std::string& question, int topK) {
    const auto conversationId = storage_.createConversation(ConversationRecord{0, question, "", ""});
    const auto userMessageId = storage_.createMessage(ChatMessage{0, conversationId, "user", question, ""});

    const auto queryEmbedding = embeddingClient_.embed(question);
    RetrievalService retrieval(vectorIndex_);
    const auto vectorResults = retrieval.search(queryEmbedding, -1);
    std::unordered_map<int64_t, float> vectorScores;
    for (const auto& item : vectorResults) {
        vectorScores[item.chunkId] = item.score;
    }

    struct Candidate {
        ChunkRecord chunk;
        float lexical = 0.0F;
        float vector = 0.0F;
    };
    std::vector<Candidate> candidates;
    for (const auto& chunk : storage_.getAllChunks()) {
        const auto vectorScore = vectorScores.find(chunk.id);
        candidates.push_back(Candidate{chunk, lexicalScore(question, chunk), vectorScore == vectorScores.end() ? 0.0F : vectorScore->second});
    }
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (left.lexical != right.lexical) return left.lexical > right.lexical;
        return left.vector > right.vector;
    });
    if (topK >= 0 && static_cast<std::size_t>(topK) < candidates.size()) {
        candidates.resize(static_cast<std::size_t>(topK));
    }

    std::vector<PromptContext> contexts;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const auto& candidate = candidates[index];
        const auto rank = static_cast<int>(index + 1);
        contexts.push_back(PromptContext{rank, candidate.chunk.content});
        storage_.createRetrievalLog(RetrievalLogRecord{0, conversationId, userMessageId, candidate.chunk.id, rank, candidate.vector, ""});
    }

    const auto prompt = promptBuilder_.build(question, contexts);
    const auto answer = chatClient_.complete({ChatMessage{0, conversationId, "user", prompt, ""}});
    storage_.createMessage(ChatMessage{0, conversationId, "assistant", answer, ""});
    return answer;
}

} // namespace localrag
