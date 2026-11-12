cmake_minimum_required(VERSION 3.16)
project(LocalRAGAgent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/LocalRagDependencies.cmake)

add_library(localrag_core
    src/text/TextChunker.cpp
    src/text/PromptBuilder.cpp
    src/text/DocumentLoader.cpp
    src/app/RetrievalService.cpp
    src/app/ChatService.cpp
    src/app/DocumentIngestionService.cpp
    src/infra/FakeEmbeddingClient.cpp
    src/infra/FakeChatClient.cpp
    src/infra/InMemoryStorage.cpp
    src/infra/InMemoryVectorIndex.cpp
    src/infra/OpenAICompatibleChatClient.cpp
    src/infra/SQLiteStorage.cpp
)

target_include_directories(localrag_core PUBLIC include)
target_include_directories(localrag_core PUBLIC ${LOCALRAG_SQLITE_INCLUDE_DIR})
target_link_libraries(localrag_core PUBLIC
    ${LOCALRAG_SQLITE_LIBRARY}
    winhttp
)

add_executable(localrag src/cli/main.cpp)
target_link_libraries(localrag PRIVATE localrag_core)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)

add_executable(localrag_desktop
    src/desktop/main.cpp
    src/ui/MainWindow.cpp
    src/ui/KnowledgeSidebar.cpp
    src/ui/ChatPanel.cpp
    src/ui/SourcePanel.cpp
    src/ui/DesktopController.cpp
    src/ui/ConfigStore.cpp
)
target_include_directories(localrag_desktop PRIVATE include src/ui)
target_link_libraries(localrag_desktop PRIVATE localrag_core Qt6::Core Qt6::Widgets Qt6::Concurrent)
if(MINGW)
    target_link_options(localrag_desktop PRIVATE -mwindows)
endif()

enable_testing()
add_subdirectory(tests)
