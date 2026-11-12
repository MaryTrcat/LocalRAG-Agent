#include "localrag/app/ChatService.h"
#include "localrag/app/DocumentIngestionService.h"
#include "localrag/infra/FakeChatClient.h"
#include "localrag/infra/FakeEmbeddingClient.h"
#include "localrag/infra/InMemoryVectorIndex.h"
#include "localrag/infra/OpenAICompatibleChatClient.h"
#include "localrag/infra/SQLiteStorage.h"
#include "localrag/text/TextChunker.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

void printHelp() {
    std::cout << "LocalRAG Agent MVP\n";
    std::cout << "Commands:\n";
    std::cout << "  localrag init\n";
    std::cout << "  localrag import <file>\n";
    std::cout << "  localrag ask \"question\"\n";
    std::cout << "  localrag list-docs\n";
    std::cout << "  localrag delete <document-id>\n";
    std::cout << "  localrag stats\n";
    std::cout << "  localrag clear\n";
}

std::filesystem::path databasePath() {
    return std::filesystem::path("data") / "localrag.db";
}

std::string envOrDefault(const char* name, const std::string& defaultValue) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return defaultValue;
    }
    return value;
}

} // namespace

int runMain(int argc, char** argv) {
    try {
        if (argc < 2) {
            printHelp();
            return 0;
        }

        const std::string command = argv[1];
        if (command == "init") {
            std::filesystem::create_directories("data");
            localrag::SQLiteStorage storage(databasePath().string());
            storage.initialize();
            std::cout << "initialized data directory\n";
            return 0;
        }

        std::filesystem::create_directories("data");
        localrag::SQLiteStorage storage(databasePath().string());
        storage.initialize();
        localrag::FakeEmbeddingClient embedding;
        localrag::InMemoryVectorIndex index;

        if (command == "list-docs") {
            const auto documents = storage.listDocuments();
            if (documents.empty()) {
                std::cout << "no documents imported\n";
                return 0;
            }
            for (const auto& document : documents) {
                std::cout << document.id << " | " << document.title << " | " << document.filePath << "\n";
            }
            return 0;
        }

        if (command == "stats") {
            std::cout << "documents: " << storage.documentCount() << "\n";
            std::cout << "chunks: " << storage.chunkCount() << "\n";
            std::cout << "messages: " << storage.messageCount() << "\n";
            std::cout << "retrieval_logs: " << storage.retrievalLogCount() << "\n";
            return 0;
        }

        if (command == "clear") {
            storage.clearAll();
            std::cout << "knowledge base cleared\n";
            return 0;
        }

        if (command == "delete") {
            if (argc < 3) {
                std::cerr << "missing document id\n";
                return 1;
            }
            const auto documentId = std::stoll(argv[2]);
            storage.deleteDocument(documentId);
            std::cout << "document deleted\n";
            return 0;
        }

        if (command == "import") {
            if (argc < 3) {
                std::cerr << "missing file path\n";
                return 1;
            }
            localrag::DocumentIngestionService service(storage, embedding, index, localrag::TextChunker(800, 120));
            const auto imported = service.importFile(argv[2]);
            index.save((std::filesystem::path("data") / "inmemory.index").string());
            std::cout << "imported chunks: " << imported << "\n";
            std::cout << "total persisted chunks: " << storage.chunkCount() << "\n";
            return 0;
        }

        if (command == "ask") {
            if (argc < 3) {
                std::cerr << "missing question\n";
                return 1;
            }

            const auto chunks = storage.getAllChunks();
            if (chunks.empty()) {
                std::cerr << "knowledge base is empty, please run import first\n";
                return 1;
            }
            for (const auto& chunk : chunks) {
                index.add(chunk.id, embedding.embed(chunk.content));
            }

            std::unique_ptr<localrag::IChatClient> chat;
            const auto apiKey = envOrDefault("LOCALRAG_API_KEY", "");
            if (apiKey.empty()) {
                std::cout << "[warning] LOCALRAG_API_KEY is not set, using fake chat client.\n";
                chat = std::make_unique<localrag::FakeChatClient>();
            } else {
                chat = std::make_unique<localrag::OpenAICompatibleChatClient>(localrag::OpenAIChatConfig{
                    envOrDefault("LOCALRAG_BASE_URL", "https://api.deepseek.com/v1"),
                    apiKey,
                    envOrDefault("LOCALRAG_CHAT_MODEL", "deepseek-chat")
                });
            }

            localrag::ChatService service(storage, embedding, *chat, index);
            std::cout << service.ask(argv[2], 1) << "\n";
            return 0;
        }

        printHelp();
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
}

#ifdef _WIN32
std::string utf8FromWide(const wchar_t* value) {
    if (value == nullptr || *value == L'\0') return {};
    const auto size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.resize(static_cast<std::size_t>(size - 1));
    return result;
}

int main() {
    int argc = 0;
    auto* wideArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wideArgv == nullptr) return 1;

    std::vector<std::string> utf8Args;
    utf8Args.reserve(static_cast<std::size_t>(argc));
    for (int index = 0; index < argc; ++index) {
        utf8Args.push_back(utf8FromWide(wideArgv[index]));
    }

    std::vector<char*> narrowArgs;
    narrowArgs.reserve(utf8Args.size());
    for (auto& argument : utf8Args) {
        narrowArgs.push_back(argument.data());
    }
    const auto result = runMain(argc, narrowArgs.data());
    LocalFree(wideArgv);
    return result;
}
#else
int main(int argc, char** argv) {
    return runMain(argc, argv);
}
#endif
