#include "localrag/text/TextChunker.h"
#include <algorithm>
#include <stdexcept>

namespace localrag {

namespace {

bool isUtf8Continuation(unsigned char value) {
    return (value & 0xC0U) == 0x80U;
}

std::size_t nextCodePointBoundary(const std::string& text, std::size_t position) {
    while (position < text.size() && isUtf8Continuation(static_cast<unsigned char>(text[position]))) {
        ++position;
    }
    return position;
}

}

TextChunker::TextChunker(std::size_t chunkSize, std::size_t overlap)
    : chunkSize_(chunkSize), overlap_(overlap) {
    if (chunkSize_ == 0) {
        throw std::invalid_argument("chunk size must be positive");
    }
    if (overlap_ >= chunkSize_) {
        throw std::invalid_argument("overlap must be smaller than chunk size");
    }
}

std::vector<TextChunk> TextChunker::split(const std::string& text) const {
    std::vector<TextChunk> chunks;
    if (text.empty()) {
        return chunks;
    }

    const std::size_t step = chunkSize_ - overlap_;
    std::size_t start = 0;
    std::size_t index = 0;

    while (start < text.size()) {
        const std::size_t end = nextCodePointBoundary(text, std::min(start + chunkSize_, text.size()));
        const std::size_t length = end - start;
        if (length == 0) {
            break;
        }
        chunks.push_back(TextChunk{index++, text.substr(start, length)});
        if (end >= text.size()) {
            break;
        }
        start = nextCodePointBoundary(text, start + step);
    }

    return chunks;
}

} // namespace localrag
