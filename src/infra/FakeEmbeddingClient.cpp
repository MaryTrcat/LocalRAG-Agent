#include "localrag/infra/FakeEmbeddingClient.h"
#include <algorithm>

namespace localrag {

std::vector<float> FakeEmbeddingClient::embed(const std::string& text) {
    std::vector<float> values(8, 0.0F);
    for (std::size_t i = 0; i < text.size(); ++i) {
        values[i % values.size()] += static_cast<unsigned char>(text[i]) / 255.0F;
    }

    const auto maxValue = *std::max_element(values.begin(), values.end());
    if (maxValue > 0.0F) {
        for (auto& value : values) {
            value /= maxValue;
        }
    }
    return values;
}

} // namespace localrag
