#pragma once

#include "localrag/core/IChatClient.h"

namespace localrag {

class FakeChatClient final : public IChatClient {
public:
    std::string complete(const std::vector<ChatMessage>& messages) override;
};

} // namespace localrag
