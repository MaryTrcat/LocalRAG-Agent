#pragma once

#include "localrag/core/IVectorIndex.h"
#include <utility>
#include <vector>

namespace localrag {

class InMemoryVectorIndex final : public IVectorIndex {
public:
    void add(int64_t chunkId, const std::vector<float>& embedding) override;
    std::vector<SearchResult> search(const std::vector<float>& queryEmbedding, int topK) override;
    void save(const std::string& path) override;
    void load(const std::string& path) override;
    std::size_t size() const;

private:
    std::vector<std::pair<int64_t, std::vector<float>>> vectors_;
};

} // namespace localrag
