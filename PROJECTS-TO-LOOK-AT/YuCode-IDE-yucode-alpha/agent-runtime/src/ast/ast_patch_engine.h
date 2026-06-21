#pragma once
#include <string>

struct AstPatchResult {
    bool success = false;
    std::string new_content;
    std::string error;
    std::string unified_diff;
};

class AstPatchEngine {
public:
    AstPatchResult replace_function(
        const std::string& file_path,
        const std::string& old_content,
        const std::string& function_name,
        const std::string& replacement
    );

private:
    bool find_function_range(
        const std::string& content,
        const std::string& function_name,
        size_t& start,
        size_t& end
    );

    size_t find_matching_brace(
        const std::string& content,
        size_t open_brace
    );

    std::string make_simple_diff(
        const std::string& old_text,
        const std::string& new_text
    );
};