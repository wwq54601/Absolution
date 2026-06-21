cmake_minimum_required(VERSION 3.20)

project(yucode_agent_runtime)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(yucode-agent
    src/main.cpp
    src/server.cpp

    src/agent/agent.cpp
    src/agent/planner.cpp
    src/agent/executor.cpp

    src/codebase/indexer.cpp
    src/codebase/retriever.cpp

    src/editing/file_editor.cpp
    src/editing/change_set.cpp
    src/codebase/symbol_index.cpp

    src/codebase/codebase_index.cpp

    src/tools/command_runner.cpp
    src/tools/error_parser.cpp

    src/editing/patch_engine.cpp

    src/project/project_detector.cpp
    src/codebase/project_summary.cpp
    src/session/session_store.cpp
    src/parsing/cpp_ast_parser.cpp

    src/parsing/tree_sitter_cpp_parser.cpp
    src/parsing/ts_query_engine.cpp

    src/codebase/call_graph.cpp

    third_party/tree-sitter/lib/src/lib.c
third_party/tree-sitter/cpp/src/parser.c
third_party/tree-sitter/cpp/src/scanner.c
src/codebase/reference_graph.cpp

src/embeddings/embedding_provider.cpp
src/embeddings/vector_index.cpp

src/config/yucode_config.cpp

src/llm/openai_compatible_client.cpp
src/embeddings/embedding_cache.cpp
src/model_runtime/model_runtime_manager.cpp
src/editing/unified_diff.cpp

src/indexing/file_analysis.cpp
src/indexing/file_analyzer.cpp
src/ast/ast_patch_engine.cpp
)

target_include_directories(yucode-agent PRIVATE
    src
    third_party
    third_party/tree-sitter/lib/include
)