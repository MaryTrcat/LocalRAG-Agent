#pragma once

#include "localrag/core/IChatClient.h"
#include <string>

namespace localrag {

struct OpenAIChatConfig {
    std::string baseUrl;
    std::string apiKey;
    std::string model;
};

std::string escapeJsonString(const std::string& value);
std::string extractChatCompletionContent(const std::string& responseBody);

class OpenAICompatibleChatClient final : public IChatClient {
public:
    explicit OpenAICompatibleChatClient(OpenAIChatConfig config);
    std::string complete(const std::vector<ChatMessage>& messages) override;

private:
    OpenAIChatConfig config_;
};

} // namespace localrag
