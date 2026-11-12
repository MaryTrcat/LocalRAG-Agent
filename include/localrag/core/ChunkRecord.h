#pragma once

#include <cstdint>
#include <string>

namespace localrag {

struct ChunkRecord {
    int64_t id = 0;
    int64_t documentId = 0;
    int chunkIndex = 0;
    std::string content;
    int tokenEstimate = 0;
    std::string createdAt;
};

} // namespace localrag
