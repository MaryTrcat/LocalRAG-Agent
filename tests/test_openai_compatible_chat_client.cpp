#include "localrag/infra/OpenAICompatibleChatClient.h"
#include <cassert>
#include <string>

void test_openai_client_escapes_json_strings() {
    const auto escaped = localrag::escapeJsonString("第一行\n\"RAG\"\\测试");

    assert(escaped == "第一行\\n\\\"RAG\\\"\\\\测试");
}

void test_openai_client_replaces_invalid_utf8() {
    const std::string invalid = std::string("前") + "\xA0" + "后";

    const auto escaped = localrag::escapeJsonString(invalid);

    assert(escaped == "前�后");
}

void test_openai_client_extracts_chat_content() {
    const std::string response = R"({"id":"x","choices":[{"message":{"role":"assistant","content":"RAG 可以减少幻觉。"}}]})";

    const auto content = localrag::extractChatCompletionContent(response);

    assert(content == "RAG 可以减少幻觉。");
}
