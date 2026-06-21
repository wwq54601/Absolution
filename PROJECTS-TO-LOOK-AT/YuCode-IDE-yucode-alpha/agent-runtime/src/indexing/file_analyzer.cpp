#include "file_analyzer.h"

#include "../parsing/tree_sitter_cpp_parser.h"
#include "../parsing/ts_query_engine.h"

#include <sstream>
#include <unordered_set>

FileAnalysis FileAnalyzer::analyze(const CodeFile& file) {
    FileAnalysis analysis;
    analysis.file_path = file.path;

    TreeSitterCppParser parser;
    auto nodes = parser.parse_file(file.path, file.content);

    for (const auto& node : nodes) {
        FileSymbol symbol;
        symbol.name = node.name;
        symbol.kind = node.kind;
        symbol.line_start = node.line_start;
        symbol.line_end = node.line_end;
        symbol.signature = node.signature;

        analysis.symbols.push_back(symbol);
    }

    TSQueryEngine query_engine;
    auto functions = query_engine.find_functions(file.path, file.content);

    for (const auto& fn : functions) {
        auto calls = query_engine.find_calls(file.path, fn.text);

        std::unordered_set<std::string> unique_calls;

        for (const auto& call : calls) {
            if (call.name.empty()) continue;
            if (call.name == fn.name) continue;

            unique_calls.insert(call.name);
        }

        for (const auto& callee : unique_calls) {
            FileCall call;
            call.caller = fn.name;
            call.callee = callee;
            call.line = fn.line_start;

            analysis.calls.push_back(call);
        }
    }

    std::istringstream stream(file.content);
    std::string line;
    int line_number = 1;

    while (std::getline(stream, line)) {
        for (const auto& symbol : analysis.symbols) {
            if (symbol.name.empty()) continue;

            if (line.find(symbol.name) != std::string::npos) {
                FileReference ref;
                ref.symbol = symbol.name;
                ref.line = line_number;
                ref.line_text = line;

                analysis.references.push_back(ref);
            }
        }

        line_number++;
    }

    return analysis;
}