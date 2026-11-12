#pragma once

#include <string>
#include <vector>

namespace localrag {

class IEmbeddingClient {
public:
    virtual ~IEmbeddingClient() = default;
    virtual std::vector<float> embed(const std::string& text) = 0;
};

} // namespace localrag
