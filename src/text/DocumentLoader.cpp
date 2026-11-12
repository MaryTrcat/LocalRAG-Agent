#include "localrag/text/DocumentLoader.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace localrag {

LoadedDocument DocumentLoader::load(const std::string& filePath) const {
    const std::filesystem::path path(filePath);
    const auto extension = path.extension().string();
    if (extension != ".txt" && extension != ".md") {
        throw std::invalid_argument("only .txt and .md files are supported");
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to open document: " + filePath);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const auto content = buffer.str();
    if (content.empty()) {
        throw std::invalid_argument("document is empty");
    }

    return LoadedDocument{path.stem().string(), path.string(), extension.substr(1), content};
}

} // namespace localrag
