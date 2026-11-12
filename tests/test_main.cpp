#include <iostream>

void test_chunker_splits_text_with_overlap();
void test_chunker_rejects_invalid_overlap();
void test_chunker_keeps_utf8_code_points_intact();
void test_prompt_builder_includes_question_and_numbered_context();
void test_retrieval_service_returns_ranked_results();
void test_chat_service_builds_answer_and_saves_logs();
void test_chat_service_recovers_keyword_match_when_vector_rank_is_wrong();
void test_document_ingestion_imports_chunks_and_vectors();
void test_openai_client_escapes_json_strings();
void test_openai_client_replaces_invalid_utf8();
void test_openai_client_extracts_chat_content();
void test_sqlite_storage_persists_documents_chunks_and_logs();

int main() {
    test_chunker_splits_text_with_overlap();
    test_chunker_rejects_invalid_overlap();
    test_chunker_keeps_utf8_code_points_intact();
    test_prompt_builder_includes_question_and_numbered_context();
    test_retrieval_service_returns_ranked_results();
    test_chat_service_builds_answer_and_saves_logs();
    test_chat_service_recovers_keyword_match_when_vector_rank_is_wrong();
    test_document_ingestion_imports_chunks_and_vectors();
    test_openai_client_escapes_json_strings();
    test_openai_client_replaces_invalid_utf8();
    test_openai_client_extracts_chat_content();
    test_sqlite_storage_persists_documents_chunks_and_logs();
    std::cout << "All LocalRAG tests passed\n";
    return 0;
}
