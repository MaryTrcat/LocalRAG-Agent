#include "localrag/text/PromptBuilder.h"
#include <sstream>

namespace localrag {

std::string PromptBuilder::build(const std::string& question, const std::vector<PromptContext>& contexts) const {
    std::ostringstream out;
    out << "你是一个本地知识库助手。请尽量只根据提供的上下文回答用户问题。\n";
    out << "如果上下文信息不足，请明确说明知识库中没有足够信息。\n\n";
    out << "上下文：\n";
    for (const auto& context : contexts) {
        out << "[" << context.sourceId << "] " << context.content << "\n";
    }
    out << "\n用户问题：\n";
    out << question << "\n\n";
    out << "请用中文回答。使用上下文时，请标注来源编号。";
    return out.str();
}

} // namespace localrag
