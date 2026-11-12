#include "localrag/app/DocumentIngestionService.h"
#include "localrag/infra/FakeEmbeddingClient.h"
#include "localrag/infra/InMemoryStorage.h"
#include "localrag/infra/InMemoryVectorIndex.h"
#include "localrag/text/TextChunker.h"
#include <cassert>
#include <filesystem>
#include <fstream>

void test_document_ingestion_imports_chunks_and_vectors() {
    const auto tempPath = std::filesystem::temp_directory_path() / "localrag_ingestion_test.md";
    {
        std::ofstream file(tempPath);
        file << "abcdefghijklmnopqrstuvwxyz";
    }

    localrag::InMemoryStorage storage;
    localrag::FakeEmbeddingClient embedding;
    localrag::InMemoryVectorIndex index;
    localrag::DocumentIngestionService service(storage, embedding, index, localrag::TextChunker(10, 3));

    const auto imported = service.importFile(tempPath.string());

    assert(imported == 4);
    assert(storage.documentCount() == 1);
    assert(storage.chunkCount() == 4);
    assert(index.size() == 4);

    std::filesystem::remove(tempPath);
}
