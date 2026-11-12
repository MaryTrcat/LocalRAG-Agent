#pragma once

#include "localrag/core/ChatMessage.h"
#include <string>
#include <vector>

namespace localrag {

class IChatClient {
public:
    virtual ~IChatClient() = default;
    virtual std::string complete(const std::vector<ChatMessage>& messages) = 0;
};

} // namespace localrag
