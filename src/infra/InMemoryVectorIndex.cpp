#include "localrag/infra/InMemoryVectorIndex.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace localrag {

namespace {

float dot(const std::vector<float>& left, const std::vector<float>& right) {
    if (left.size() != right.size()) {
        throw std::invalid_argument("embedding dimensions do not match");
    }
    float value = 0.0F;
    for (std::size_t i = 0; i < left.size(); ++i) {
        value += left[i] * right[i];
    }
    return value;
}

} // namespace

void InMemoryVectorIndex::add(int64_t chunkId, const std::vector<float>& embedding) {
    vectors_.push_back({chunkId, embedding});
}

std::vector<SearchResult> InMemoryVectorIndex::search(const std::vector<float>& queryEmbedding, int topK) {
    std::vector<SearchResult> results;
    for (const auto& item : vectors_) {
        results.push_back(SearchResult{item.first, dot(queryEmbedding, item.second)});
    }

    std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b) {
        return a.score > b.score;
    });

    if (topK >= 0 && static_cast<std::size_t>(topK) < results.size()) {
        results.resize(static_cast<std::size_t>(topK));
    }

    return results;
}

void InMemoryVectorIndex::save(const std::string& path) {
    std::ofstream file(path);
    file << vectors_.size() << "\n";
}

void InMemoryVectorIndex::load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return;
    }
}

std::size_t InMemoryVectorIndex::size() const {
    return vectors_.size();
}

} // namespace localrag
