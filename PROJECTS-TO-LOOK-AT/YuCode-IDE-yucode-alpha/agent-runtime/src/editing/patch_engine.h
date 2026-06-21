#pragma once
#include <string>

struct PatchResult {
    bool success = false;
    std::string new_content;
    std::string error;
    std::string unified_diff;
};

class PatchEngine {
public:
    PatchResult apply_find_replace(
        const std::string& old_content,
        const std::string& find_text,
        const std::string& replace_text
    );
};