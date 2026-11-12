#pragma once

#include "localrag/core/RetrievalTypes.h"
#include <cstdint>
#include <string>
#include <vector>

namespace localrag {

class IVectorIndex {
public:
    virtual ~IVectorIndex() = default;
    virtual void add(int64_t chunkId, const std::vector<float>& embedding) = 0;
    virtual std::vector<SearchResult> search(const std::vector<float>& queryEmbedding, int topK) = 0;
    virtual void save(const std::string& path) = 0;
    virtual void load(const std::string& path) = 0;
};

} // namespace localrag
