#pragma once

#include "localrag/core/IEmbeddingClient.h"
#include "localrag/core/IStorage.h"
#include "localrag/core/IVectorIndex.h"
#include "localrag/text/DocumentLoader.h"
#include "localrag/text/TextChunker.h"
#include <string>

namespace localrag {

class DocumentIngestionService {
public:
    DocumentIngestionService(IStorage& storage, IEmbeddingClient& embeddingClient, IVectorIndex& vectorIndex, TextChunker chunker);
    int importFile(const std::string& filePath);

private:
    IStorage& storage_;
    IEmbeddingClient& embeddingClient_;
    IVectorIndex& vectorIndex_;
    TextChunker chunker_;
    DocumentLoader loader_;
};

} // namespace localrag
