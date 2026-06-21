#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "indexer.h"

struct FunctionCallInfo {
    std::string function_name;
    std::string file_path;
    int line_start = 0;
    int line_end = 0;

    std::vector<std::string> calls;
};

class CallGraph {
public:
    void build(const std::vector<CodeFile>& files);

    void update_file(
        const std::string& file_path,
        const std::string& content
    );

    void remove_file(const std::string& file_path);

    std::vector<FunctionCallInfo> search(
        const std::string& query,
        int limit = 20
    ) const;

    std::string build_context(
        const std::string& query,
        int limit = 20
    ) const;

    size_t size() const;

private:
    std::vector<FunctionCallInfo> functions_;

    static std::string lower(std::string value);
    static bool contains_case_insensitive(
        const std::string& text,
        const std::string& query
    );
};