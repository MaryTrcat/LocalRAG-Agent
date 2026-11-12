#pragma once

#include <cstdint>
#include <string>

namespace localrag {

struct SearchResult {
    int64_t chunkId = 0;
    float score = 0.0F;
};

struct RetrievedChunk {
    int64_t chunkId = 0;
    int rank = 0;
    float score = 0.0F;
    std::string content;
};

} // namespace localrag
