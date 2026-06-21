#include "ast_patch_engine.h"

#include <sstream>

AstPatchResult AstPatchEngine::replace_function(
    const std::string& file_path,
    const std::string& old_content,
    const std::string& function_name,
    const std::string& replacement
) {
    AstPatchResult result;

    size_t start = 0;
    size_t end = 0;

    if (!find_function_range(old_content, function_name, start, end)) {
        result.error = "function not found: " + function_name;
        return result;
    }

    std::string old_text = old_content.substr(start, end - start);

    result.new_content = old_content;
    result.new_content.replace(start, end - start, replacement);

    result.unified_diff = make_simple_diff(old_text, replacement);
    result.success = true;

    return result;
}

bool AstPatchEngine::find_function_range(
    const std::string& content,
    const std::string& function_name,
    size_t& start,
    size_t& end
) {
    std::string pattern = function_name + "(";

    size_t name_pos = content.find(pattern);

    if (name_pos == std::string::npos) {
        return false;
    }

    size_t line_start = content.rfind('\n', name_pos);

    if (line_start == std::string::npos) {
        line_start = 0;
    } else {
        line_start++;
    }

    size_t open_brace = content.find('{', name_pos);

    if (open_brace == std::string::npos) {
        return false;
    }

    size_t close_brace = find_matching_brace(content, open_brace);

    if (close_brace == std::string::npos) {
        return false;
    }

    start = line_start;
    end = close_brace + 1;

    if (end < content.size() && content[end] == '\n') {
        end++;
    }

    return true;
}

size_t AstPatchEngine::find_matching_brace(
    const std::string& content,
    size_t open_brace
) {
    int depth = 0;

    for (size_t i = open_brace; i < content.size(); i++) {
        if (content[i] == '{') {
            depth++;
        }

        if (content[i] == '}') {
            depth--;

            if (depth == 0) {
                return i;
            }
        }
    }

    return std::string::npos;
}

std::string AstPatchEngine::make_simple_diff(
    const std::string& old_text,
    const std::string& new_text
) {
    std::stringstream diff;

    diff << "--- old\n";
    diff << "+++ new\n";
    diff << "@@ -1,1 +1,1 @@\n";

    std::istringstream old_stream(old_text);
    std::string line;

    while (std::getline(old_stream, line)) {
        diff << "-" << line << "\n";
    }

    std::istringstream new_stream(new_text);

    while (std::getline(new_stream, line)) {
        diff << "+" << line << "\n";
    }

    return diff.str();
}