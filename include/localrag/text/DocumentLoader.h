#pragma once

#include <string>

namespace localrag {

struct LoadedDocument {
    std::string title;
    std::string filePath;
    std::string fileType;
    std::string content;
};

class DocumentLoader {
public:
    LoadedDocument load(const std::string& filePath) const;
};

} // namespace localrag
