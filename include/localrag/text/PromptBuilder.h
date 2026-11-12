#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace localrag {

struct PromptContext {
    int64_t sourceId;
    std::string content;
};

class PromptBuilder {
public:
    std::string build(const std::string& question, const std::vector<PromptContext>& contexts) const;
};

} // namespace localrag
