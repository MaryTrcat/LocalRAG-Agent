#include "localrag/text/PromptBuilder.h"
#include <cassert>
#include <string>

void test_prompt_builder_includes_question_and_numbered_context() {
    localrag::PromptBuilder builder;
    std::vector<localrag::PromptContext> contexts = {
        {1, "第一段资料"},
        {2, "第二段资料"}
    };

    const auto prompt = builder.build("什么是 RAG？", contexts);

    assert(prompt.find("什么是 RAG？") != std::string::npos);
    assert(prompt.find("[1] 第一段资料") != std::string::npos);
    assert(prompt.find("[2] 第二段资料") != std::string::npos);
    assert(prompt.find("请用中文回答") != std::string::npos);
}
