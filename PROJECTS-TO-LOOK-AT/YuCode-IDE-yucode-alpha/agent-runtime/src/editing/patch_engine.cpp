#include "patch_engine.h"
#include <sstream>

PatchResult PatchEngine::apply_find_replace(
    const std::string& old_content,
    const std::string& find_text,
    const std::string& replace_text
) {
    PatchResult result;

    if (find_text.empty()) {
        result.error = "find_text is empty";
        return result;
    }

    size_t pos = old_content.find(find_text);

    if (pos == std::string::npos) {
        result.error = "find_text not found in file";
        return result;
    }

    if (old_content.find(find_text, pos + find_text.size()) != std::string::npos) {
        result.error = "find_text is not unique in file";
        return result;
    }

    result.new_content = old_content;
    result.new_content.replace(pos, find_text.size(), replace_text);
    std::stringstream diff;

diff << "--- old\n";
diff << "+++ new\n";
diff << "@@ -1,1 +1,1 @@\n";

diff << "-" << find_text << "\n";
diff << "+" << replace_text << "\n";

result.unified_diff = diff.str();
    result.success = true;

    return result;
}