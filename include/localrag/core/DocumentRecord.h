#pragma once

#include <cstdint>
#include <string>

namespace localrag {

struct DocumentRecord {
    int64_t id = 0;
    std::string title;
    std::string filePath;
    std::string fileType;
    std::string contentHash;
    std::string createdAt;
};

} // namespace localrag
