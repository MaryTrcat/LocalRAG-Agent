#include "localrag/app/DocumentIngestionService.h"
#include <utility>

namespace localrag {

DocumentIngestionService::DocumentIngestionService(IStorage& storage, IEmbeddingClient& embeddingClient, IVectorIndex& vectorIndex, TextChunker chunker)
    : storage_(storage), embeddingClient_(embeddingClient), vectorIndex_(vectorIndex), chunker_(std::move(chunker)) {}

int DocumentIngestionService::importFile(const std::string& filePath) {
    const auto document = loader_.load(filePath);
    const auto documentId = storage_.createDocument(DocumentRecord{0, document.title, document.filePath, document.fileType, "", ""});
    const auto chunks = chunker_.split(document.content);

    int imported = 0;
    for (const auto& chunk : chunks) {
        const auto chunkId = storage_.createChunk(ChunkRecord{0, documentId, static_cast<int>(chunk.index), chunk.content, static_cast<int>(chunk.content.size()), ""});
        vectorIndex_.add(chunkId, embeddingClient_.embed(chunk.content));
        ++imported;
    }
    return imported;
}

} // namespace localrag
