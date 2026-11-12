#include "DesktopController.h"

#include "localrag/app/DocumentIngestionService.h"
#include "localrag/core/IChatClient.h"
#include "localrag/infra/FakeChatClient.h"
#include "localrag/infra/FakeEmbeddingClient.h"
#include "localrag/infra/InMemoryVectorIndex.h"
#include "localrag/infra/OpenAICompatibleChatClient.h"
#include "localrag/infra/SQLiteStorage.h"
#include "localrag/text/PromptBuilder.h"
#include "localrag/text/TextChunker.h"

#include <QFileInfo>
#include <QHash>
#include <QProcessEnvironment>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace localrag {

DesktopController::DesktopController(DesktopConfig config)
    : config_(std::move(config)),
      storage_(std::make_unique<SQLiteStorage>(config_.databasePath.toStdString())),
      embedding_(std::make_unique<FakeEmbeddingClient>()) {}

DesktopController::~DesktopController() = default;

void DesktopController::initialize() {
    storage_->initialize();
}

DesktopSnapshot DesktopController::snapshot() const {
    DesktopSnapshot result;
    const auto documents = storage_->listDocuments();
    const auto chunks = storage_->getAllChunks();
    std::unordered_map<int64_t, int> chunkCounts;
    for (const auto& chunk : chunks) {
        ++chunkCounts[chunk.documentId];
    }
    for (const auto& document : documents) {
        result.documents.push_back(DesktopDocument{
            document.id,
            QString::fromStdString(document.title),
            QString::fromStdString(document.filePath),
            QString::fromStdString(document.fileType),
            chunkCounts[document.id]
        });
    }
    result.stats.documents = static_cast<int>(documents.size());
    result.stats.chunks = static_cast<int>(chunks.size());
    result.stats.messages = static_cast<int>(storage_->messageCount());
    result.stats.retrievalLogs = static_cast<int>(storage_->retrievalLogCount());
    return result;
}

int DesktopController::importDocument(const QString& filePath) {
    if (filePath.trimmed().isEmpty()) {
        throw std::invalid_argument("文件路径为空");
    }
    InMemoryVectorIndex index;
    DocumentIngestionService service(*storage_, *embedding_, index,
                                     TextChunker(static_cast<std::size_t>(config_.chunkSize),
                                                 static_cast<std::size_t>(config_.chunkOverlap)));
    return service.importFile(filePath.toStdString());
}

DesktopAnswer DesktopController::ask(const QString& question) {
    const auto trimmed = question.trimmed();
    if (trimmed.isEmpty()) {
        throw std::invalid_argument("问题不能为空");
    }
    const auto chunks = storage_->getAllChunks();
    if (chunks.empty()) {
        throw std::runtime_error("知识库为空，请先导入 Markdown 或 TXT 文件");
    }

    InMemoryVectorIndex index;
    std::unordered_map<int64_t, ChunkRecord> chunkMap;
    std::unordered_map<int64_t, std::string> documentTitles;
    std::unordered_map<int64_t, std::string> documentPaths;
    for (const auto& document : storage_->listDocuments()) {
        documentTitles[document.id] = document.title;
        documentPaths[document.id] = document.filePath;
    }
    for (const auto& chunk : chunks) {
        chunkMap[chunk.id] = chunk;
        index.add(chunk.id, embedding_->embed(chunk.content));
    }

    const auto rawResults = index.search(embedding_->embed(trimmed.toStdString()), config_.topK);
    std::vector<ChatMessage> messages;
    const auto conversationId = storage_->createConversation(ConversationRecord{0, trimmed.toStdString(), "", ""});
    const auto userMessageId = storage_->createMessage(ChatMessage{0, conversationId, "user", trimmed.toStdString(), ""});
    std::vector<PromptContext> contexts;
    DesktopAnswer result;
    int rank = 1;
    for (const auto& raw : rawResults) {
        const auto found = chunkMap.find(raw.chunkId);
        if (found == chunkMap.end()) {
            continue;
        }
        contexts.push_back(PromptContext{rank, found->second.content});
        storage_->createRetrievalLog(RetrievalLogRecord{0, conversationId, userMessageId, raw.chunkId, rank, raw.score, ""});
        result.sources.push_back(DesktopSource{
            raw.chunkId,
            rank,
            raw.score,
            QString::fromStdString(documentTitles[found->second.documentId]),
            QString::fromStdString(documentPaths[found->second.documentId]),
            QString::fromStdString(found->second.content)
        });
        ++rank;
    }

    PromptBuilder builder;
    const auto prompt = builder.build(trimmed.toStdString(), contexts);
    const auto apiKey = QProcessEnvironment::systemEnvironment().value(config_.apiKeyEnv);
    std::unique_ptr<IChatClient> chat;
    if (apiKey.isEmpty()) {
        chat = std::make_unique<FakeChatClient>();
        result.fakeMode = true;
    } else {
        chat = std::make_unique<OpenAICompatibleChatClient>(OpenAIChatConfig{
            config_.baseUrl.toStdString(), apiKey.toStdString(), config_.chatModel.toStdString()
        });
    }
    result.answer = QString::fromStdString(chat->complete({ChatMessage{0, conversationId, "user", prompt, ""}}));
    storage_->createMessage(ChatMessage{0, conversationId, "assistant", result.answer.toStdString(), ""});
    return result;
}

void DesktopController::resetConversation() {}

} // namespace localrag
