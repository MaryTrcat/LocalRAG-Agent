#pragma once

#include "localrag/core/IEmbeddingClient.h"

namespace localrag {

class FakeEmbeddingClient final : public IEmbeddingClient {
public:
    std::vector<float> embed(const std::string& text) override;
};

} // namespace localrag
