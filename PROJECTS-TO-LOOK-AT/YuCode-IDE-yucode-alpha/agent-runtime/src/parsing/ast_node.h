#pragma once
#include <string>
#include <vector>

struct AstNode {
    std::string kind;
    std::string name;
    std::string file_path;

    int line_start = 0;
    int line_end = 0;

    std::string signature;
    std::string text;

    std::vector<AstNode> children;
};