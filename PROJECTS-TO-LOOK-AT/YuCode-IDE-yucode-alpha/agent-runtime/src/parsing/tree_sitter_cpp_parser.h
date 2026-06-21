#pragma once
#include "ast_parser.h"

class TreeSitterCppParser : public AstParser {
public:
    std::vector<AstNode> parse_file(
        const std::string& file_path,
        const std::string& content
    ) override;
};