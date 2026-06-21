#include "cpp_ast_parser.h"

#include <sstream>
#include <regex>
#include <algorithm>

std::vector<AstNode> CppAstParser::parse_file(
    const std::string& file_path,
    const std::string& content
) {
    std::vector<AstNode> nodes;

    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    std::regex class_regex(R"(\b(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*))");

    std::regex function_regex(
        R"(^\s*(?:[\w:<>\*&]+\s+)+([A-Za-z_][A-Za-z0-9_:]*)\s*\([^;]*\)\s*(?:const\s*)?(?:\{|$))"
    );

    for (int i = 0; i < (int)lines.size(); i++) {
        std::smatch match;
        const std::string& current = lines[i];

        if (std::regex_search(current, match, class_regex)) {
            int end = find_block_end(lines, i);

            nodes.push_back(make_node(
                match[1],
                match[2],
                file_path,
                i + 1,
                end + 1,
                current,
                join_lines(lines, i, end)
            ));
        }

        if (std::regex_search(current, match, function_regex)) {
            std::string name = match[1];

            if (
                name == "if" ||
                name == "for" ||
                name == "while" ||
                name == "switch" ||
                name == "catch"
            ) {
                continue;
            }

            int end = find_block_end(lines, i);

            nodes.push_back(make_node(
                name.find("::") != std::string::npos ? "method" : "function",
                name,
                file_path,
                i + 1,
                end + 1,
                current,
                join_lines(lines, i, end)
            ));
        }
    }

    return nodes;
}

AstNode CppAstParser::make_node(
    const std::string& kind,
    const std::string& name,
    const std::string& file_path,
    int line_start,
    int line_end,
    const std::string& signature,
    const std::string& text
) {
    AstNode node;
    node.kind = kind;
    node.name = name;
    node.file_path = file_path;
    node.line_start = line_start;
    node.line_end = line_end;
    node.signature = signature;
    node.text = text;
    return node;
}

int CppAstParser::find_block_end(
    const std::vector<std::string>& lines,
    int start_line
) {
    int brace_balance = 0;
    bool seen_open = false;

    for (int i = start_line; i < (int)lines.size(); i++) {
        for (char c : lines[i]) {
            if (c == '{') {
                brace_balance++;
                seen_open = true;
            }

            if (c == '}') {
                brace_balance--;
            }
        }

        if (seen_open && brace_balance <= 0) {
            return i;
        }
    }

    return start_line;
}

std::string CppAstParser::join_lines(
    const std::vector<std::string>& lines,
    int start,
    int end
) {
    std::stringstream ss;

    if (start < 0) start = 0;
    if (end >= (int)lines.size()) end = (int)lines.size() - 1;

    for (int i = start; i <= end; i++) {
        ss << lines[i] << "\n";
    }

    return ss.str();
}