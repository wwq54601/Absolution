#include "unified_diff.h"

#include <sstream>
#include <regex>

UnifiedDiff UnifiedDiffParser::parse(const std::string& diff) const {
    UnifiedDiff parsed;

    std::istringstream stream(diff);
    std::string line;

    DiffHunk current;
    bool in_hunk = false;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.rfind("--- ", 0) == 0) {
            parsed.old_file = line.substr(4);
            continue;
        }

        if (line.rfind("+++ ", 0) == 0) {
            parsed.new_file = line.substr(4);
            continue;
        }

        if (line.rfind("@@ ", 0) == 0) {
            if (in_hunk) {
                parsed.hunks.push_back(current);
                current = DiffHunk{};
            }

            if (parse_hunk_header(line, current)) {
                in_hunk = true;
            }

            continue;
        }

        if (in_hunk) {
            if (line.empty()) {
                current.lines.push_back({' ', ""});
                continue;
            }

            char prefix = line[0];

            if (prefix == ' ' || prefix == '+' || prefix == '-') {
                current.lines.push_back({
                    prefix,
                    line.substr(1)
                });
            }
        }
    }

    if (in_hunk) {
        parsed.hunks.push_back(current);
    }

    return parsed;
}

bool UnifiedDiffParser::parse_hunk_header(
    const std::string& line,
    DiffHunk& hunk
) const {
    std::regex re(R"(@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@)");
    std::smatch match;

    if (!std::regex_search(line, match, re)) {
        return false;
    }

    hunk.old_start = std::stoi(match[1]);
    hunk.old_count = match[2].matched ? std::stoi(match[2]) : 1;
    hunk.new_start = std::stoi(match[3]);
    hunk.new_count = match[4].matched ? std::stoi(match[4]) : 1;

    return true;
}

UnifiedPatchResult UnifiedDiffApplier::apply(
    const std::string& old_content,
    const UnifiedDiff& diff
) const {
    UnifiedPatchResult result;

    auto original = split_lines(old_content);
    std::vector<std::string> output;

    size_t source_index = 0;

    for (const auto& hunk : diff.hunks) {
        size_t hunk_start = hunk.old_start > 0
            ? static_cast<size_t>(hunk.old_start - 1)
            : 0;

        if (hunk_start < source_index) {
            result.error = "Overlapping or out-of-order hunk.";
            return result;
        }

        while (source_index < hunk_start && source_index < original.size()) {
            output.push_back(original[source_index]);
            source_index++;
        }

        for (const auto& line : hunk.lines) {
            if (line.type == ' ') {
                if (source_index >= original.size()) {
                    result.error = "Context line exceeds source size.";
                    return result;
                }

                if (original[source_index] != line.text) {
                    result.error =
                        "Context mismatch. Expected: `" +
                        line.text +
                        "` Got: `" +
                        original[source_index] +
                        "`";
                    return result;
                }

                output.push_back(original[source_index]);
                source_index++;
            }

            else if (line.type == '-') {
                if (source_index >= original.size()) {
                    result.error = "Delete line exceeds source size.";
                    return result;
                }

                if (original[source_index] != line.text) {
                    result.error =
                        "Delete mismatch. Expected: `" +
                        line.text +
                        "` Got: `" +
                        original[source_index] +
                        "`";
                    return result;
                }

                source_index++;
            }

            else if (line.type == '+') {
                output.push_back(line.text);
            }
        }
    }

    while (source_index < original.size()) {
        output.push_back(original[source_index]);
        source_index++;
    }

    result.success = true;
    result.new_content = join_lines(output);
    return result;
}

std::vector<std::string> UnifiedDiffApplier::split_lines(
    const std::string& text
) const {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        lines.push_back(line);
    }

    return lines;
}

std::string UnifiedDiffApplier::join_lines(
    const std::vector<std::string>& lines
) const {
    std::stringstream ss;

    for (size_t i = 0; i < lines.size(); i++) {
        ss << lines[i];

        if (i + 1 < lines.size()) {
            ss << "\n";
        }
    }

    return ss.str();
}