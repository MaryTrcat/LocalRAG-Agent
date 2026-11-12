#include "localrag/app/RetrievalService.h"
#include "localrag/infra/InMemoryVectorIndex.h"
#include <cassert>

void test_retrieval_service_returns_ranked_results() {
    localrag::InMemoryVectorIndex index;
    index.add(101, {1.0F, 0.0F});
    index.add(202, {0.2F, 0.0F});

    localrag::RetrievalService service(index);
    const auto results = service.search({1.0F, 0.0F}, 2);

    assert(results.size() == 2);
    assert(results[0].chunkId == 101);
    assert(results[0].rank == 1);
    assert(results[1].chunkId == 202);
    assert(results[1].rank == 2);
}
