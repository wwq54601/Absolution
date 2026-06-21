#pragma once

#include <string>
#include <vector>

struct FileSymbol {
    std::string name;
    std::string kind;
    int line_start = 0;
    int line_end = 0;
    std::string signature;
};

struct FileReference {
    std::string symbol;
    int line = 0;
    std::string line_text;
};

struct FileCall {
    std::string caller;
    std::string callee;
    int line = 0;
};

struct FileAnalysis {
    std::string file_path;

    std::vector<FileSymbol> symbols;
    std::vector<FileReference> references;
    std::vector<FileCall> calls;
};