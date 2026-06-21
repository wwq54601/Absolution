#pragma once
#include <string>
#include <vector>
#include "ast_node.h"

class AstParser {
public:
    virtual ~AstParser() = default;

    virtual std::vector<AstNode> parse_file(
        const std::string& file_path,
        const std::string& content
    ) = 0;
};