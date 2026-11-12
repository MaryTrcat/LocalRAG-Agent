#pragma once

#include "localrag/core/IVectorIndex.h"
#include "localrag/core/RetrievalTypes.h"
#include <vector>

namespace localrag {

class RetrievalService {
public:
    explicit RetrievalService(IVectorIndex& vectorIndex);
    std::vector<RetrievedChunk> search(const std::vector<float>& queryEmbedding, int topK);

private:
    IVectorIndex& vectorIndex_;
};

} // namespace localrag
