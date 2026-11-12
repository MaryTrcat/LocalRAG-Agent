#include "localrag/text/TextChunker.h"
#include <cassert>
#include <stdexcept>

namespace {

bool isValidUtf8(const std::string& text) {
    for (std::size_t index = 0; index < text.size();) {
        const auto byte = static_cast<unsigned char>(text[index]);
        std::size_t length = 0;
        if (byte <= 0x7F) {
            length = 1;
        } else if (byte >= 0xC2 && byte <= 0xDF) {
            length = 2;
        } else if (byte >= 0xE0 && byte <= 0xEF) {
            length = 3;
        } else if (byte >= 0xF0 && byte <= 0xF4) {
            length = 4;
        } else {
            return false;
        }
        if (index + length > text.size()) return false;
        for (std::size_t offset = 1; offset < length; ++offset) {
            if ((static_cast<unsigned char>(text[index + offset]) & 0xC0U) != 0x80U) return false;
        }
        index += length;
    }
    return true;
}

}

void test_chunker_splits_text_with_overlap() {
    localrag::TextChunker chunker(10, 3);
    auto chunks = chunker.split("abcdefghijklmnopqrstuvwxyz");

    assert(chunks.size() == 4);
    assert(chunks[0].content == "abcdefghij");
    assert(chunks[1].content == "hijklmnopq");
    assert(chunks[2].content == "opqrstuvwx");
    assert(chunks[3].content == "vwxyz");
}

void test_chunker_rejects_invalid_overlap() {
    bool thrown = false;
    try {
        localrag::TextChunker chunker(10, 10);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
}

void test_chunker_keeps_utf8_code_points_intact() {
    const std::string text = "本地向量索引可以减少回答幻觉";
    localrag::TextChunker chunker(10, 3);
    const auto chunks = chunker.split(text);

    assert(chunks.size() > 1);
    for (const auto& chunk : chunks) {
        assert(isValidUtf8(chunk.content));
    }
}
