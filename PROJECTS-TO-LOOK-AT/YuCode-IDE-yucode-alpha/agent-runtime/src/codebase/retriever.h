#pragma once
#include <string>
#include "../agent/memory.h"
#include "codebase_index.h"

class Retriever {
public:
    explicit Retriever(CodebaseIndex* index);

    std::string retrieve(
        const std::string& query,
        const std::string& active_file,
        const AgentMemory& memory
    );

    std::string search(const std::string& query);
    std::string search_symbols(const std::string& query);
    std::string search_references(const std::string& query);
    std::string search_calls(const std::string& query);
    std::string analyze_impact(const std::string& query);
    std::string semantic_search(const std::string& query);

private:
    CodebaseIndex* index_;

    bool contains_case_insensitive(
        const std::string& text,
        const std::string& query
    ) const;
};