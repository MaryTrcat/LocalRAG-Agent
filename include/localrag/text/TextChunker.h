#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace localrag {

struct TextChunk {
    std::size_t index;
    std::string content;
};

class TextChunker {
public:
    TextChunker(std::size_t chunkSize, std::size_t overlap);
    std::vector<TextChunk> split(const std::string& text) const;

private:
    std::size_t chunkSize_;
    std::size_t overlap_;
};

} // namespace localrag
