#include "localrag/infra/FakeChatClient.h"

namespace localrag {

std::string FakeChatClient::complete(const std::vector<ChatMessage>& messages) {
    if (messages.empty()) {
        return "没有收到问题。";
    }
    return "这是一个 MVP 模拟回答：\n" + messages.back().content;
}

} // namespace localrag
