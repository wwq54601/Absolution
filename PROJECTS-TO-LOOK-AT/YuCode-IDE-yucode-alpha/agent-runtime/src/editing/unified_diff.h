#pragma once
#include <string>
#include <vector>

struct DiffLine {
    char type = ' '; // ' ', '+', '-'
    std::string text;
};

struct DiffHunk {
    int old_start = 0;
    int old_count = 0;
    int new_start = 0;
    int new_count = 0;

    std::vector<DiffLine> lines;
};

struct UnifiedDiff {
    std::string old_file;
    std::string new_file;
    std::vector<DiffHunk> hunks;
};

struct UnifiedPatchResult {
    bool success = false;
    std::string new_content;
    std::string error;
};

class UnifiedDiffParser {
public:
    UnifiedDiff parse(const std::string& diff) const;

private:
    bool parse_hunk_header(
        const std::string& line,
        DiffHunk& hunk
    ) const;
};

class UnifiedDiffApplier {
public:
    UnifiedPatchResult apply(
        const std::string& old_content,
        const UnifiedDiff& diff
    ) const;

private:
    std::vector<std::string> split_lines(const std::string& text) const;
    std::string join_lines(const std::vector<std::string>& lines) const;
};