add_executable(localrag_tests
    test_main.cpp
    test_text_chunker.cpp
    test_prompt_builder.cpp
    test_retrieval_service.cpp
    test_chat_service.cpp
    test_document_ingestion_service.cpp
    test_openai_compatible_chat_client.cpp
    test_sqlite_storage.cpp
)

target_link_libraries(localrag_tests PRIVATE localrag_core)

add_test(NAME localrag_tests COMMAND localrag_tests)
