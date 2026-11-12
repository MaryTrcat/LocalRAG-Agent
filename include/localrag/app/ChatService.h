#pragma once

#include "localrag/core/IChatClient.h"
#include "localrag/core/IEmbeddingClient.h"
#include "localrag/core/IStorage.h"
#include "localrag/core/IVectorIndex.h"
#include "localrag/text/PromptBuilder.h"
#include <string>

namespace localrag {

class ChatService {
public:
    ChatService(IStorage& storage, IEmbeddingClient& embeddingClient, IChatClient& chatClient, IVectorIndex& vectorIndex);
    std::string ask(const std::string& question, int topK);

private:
    IStorage& storage_;
    IEmbeddingClient& embeddingClient_;
    IChatClient& chatClient_;
    IVectorIndex& vectorIndex_;
    PromptBuilder promptBuilder_;
};

} // namespace localrag
