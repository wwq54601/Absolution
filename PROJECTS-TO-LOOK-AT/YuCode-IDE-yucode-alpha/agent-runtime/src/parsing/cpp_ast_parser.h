#pragma once
#include "ast_parser.h"

class CppAstParser : public AstParser {
public:
    std::vector<AstNode> parse_file(
        const std::string& file_path,
        const std::string& content
    ) override;

private:
    AstNode make_node(
        const std::string& kind,
        const std::string& name,
        const std::string& file_path,
        int line_start,
        int line_end,
        const std::string& signature,
        const std::string& text
    );

    int find_block_end(
        const std::vector<std::string>& lines,
        int start_line
    );

    std::string join_lines(
        const std::vector<std::string>& lines,
        int start,
        int end
    );
};