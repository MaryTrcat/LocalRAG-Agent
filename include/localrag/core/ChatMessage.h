#pragma once

#include <cstdint>
#include <string>

namespace localrag {

struct ChatMessage {
    int64_t id = 0;
    int64_t conversationId = 0;
    std::string role;
    std::string content;
    std::string createdAt;
};

} // namespace localrag
