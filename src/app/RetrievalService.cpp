#include "localrag/app/RetrievalService.h"

namespace localrag {

RetrievalService::RetrievalService(IVectorIndex& vectorIndex)
    : vectorIndex_(vectorIndex) {}

std::vector<RetrievedChunk> RetrievalService::search(const std::vector<float>& queryEmbedding, int topK) {
    const auto rawResults = vectorIndex_.search(queryEmbedding, topK);
    std::vector<RetrievedChunk> chunks;
    chunks.reserve(rawResults.size());

    int rank = 1;
    for (const auto& result : rawResults) {
        chunks.push_back(RetrievedChunk{result.chunkId, rank++, result.score, ""});
    }
    return chunks;
}

} // namespace localrag
