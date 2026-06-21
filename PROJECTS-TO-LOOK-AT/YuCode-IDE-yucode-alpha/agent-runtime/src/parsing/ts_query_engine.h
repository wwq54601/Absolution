#pragma once
#include <string>
#include <vector>

struct QueryMatch {
    std::string kind;
    std::string name;
    std::string file_path;

    int line_start = 0;
    int line_end = 0;

    std::string text;
};

class TSQueryEngine {
public:
    std::vector<QueryMatch> find_functions(
        const std::string& file_path,
        const std::string& source
    );

    std::vector<QueryMatch> find_classes(
        const std::string& file_path,
        const std::string& source
    );

    std::vector<QueryMatch> find_calls(
        const std::string& file_path,
        const std::string& source
    );

    std::vector<QueryMatch> find_includes(
        const std::string& file_path,
        const std::string& source
    );

private:
    std::vector<QueryMatch> run_query(
        const std::string& file_path,
        const std::string& source,
        const std::string& query,
        const std::string& kind
    );

    std::string node_text(
        const std::string& source,
        void* node_ptr
    );

    std::string capture_name_from_match(
        const std::string& source,
        void* node_ptr
    );
};