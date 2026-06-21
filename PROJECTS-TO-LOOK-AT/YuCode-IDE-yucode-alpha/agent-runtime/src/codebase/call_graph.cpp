#include "call_graph.h"
#include "../parsing/ts_query_engine.h"

#include <sstream>
#include <algorithm>
#include <unordered_set>

void CallGraph::build(const std::vector<CodeFile>& files) {
    functions_.clear();

    TSQueryEngine engine;

    for (const auto& file : files) {
        auto functions = engine.find_functions(file.path, file.content);

        for (const auto& fn : functions) {
            FunctionCallInfo info;
            info.function_name = fn.name;
            info.file_path = fn.file_path;
            info.line_start = fn.line_start;
            info.line_end = fn.line_end;

            auto calls = engine.find_calls(file.path, fn.text);

            std::unordered_set<std::string> unique;

            for (const auto& call : calls) {
                if (call.name.empty()) continue;
                if (call.name == fn.name) continue;

                unique.insert(call.name);
            }

            for (const auto& name : unique) {
                info.calls.push_back(name);
            }

            std::sort(info.calls.begin(), info.calls.end());

            functions_.push_back(info);
        }
    }
}

void CallGraph::update_file(
    const std::string& file_path,
    const std::string& content
) {
    remove_file(file_path);

    TSQueryEngine engine;

    auto functions = engine.find_functions(file_path, content);

    for (const auto& fn : functions) {
        FunctionCallInfo info;
        info.function_name = fn.name;
        info.file_path = fn.file_path;
        info.line_start = fn.line_start;
        info.line_end = fn.line_end;

        auto calls = engine.find_calls(file_path, fn.text);

        std::unordered_set<std::string> unique;

        for (const auto& call : calls) {
            if (call.name.empty()) continue;
            if (call.name == fn.name) continue;

            unique.insert(call.name);
        }

        for (const auto& name : unique) {
            info.calls.push_back(name);
        }

        std::sort(info.calls.begin(), info.calls.end());

        functions_.push_back(info);
    }
}

void CallGraph::remove_file(const std::string& file_path) {
    functions_.erase(
        std::remove_if(
            functions_.begin(),
            functions_.end(),
            [&](const FunctionCallInfo& info) {
                return info.file_path == file_path;
            }
        ),
        functions_.end()
    );
}

std::vector<FunctionCallInfo> CallGraph::search(
    const std::string& query,
    int limit
) const {
    std::vector<FunctionCallInfo> results;
    std::string q = lower(query);

    for (const auto& fn : functions_) {
        if (
            lower(fn.function_name).find(q) != std::string::npos ||
            lower(fn.file_path).find(q) != std::string::npos ||
            q.find(lower(fn.function_name)) != std::string::npos
        ) {
            results.push_back(fn);

            if ((int)results.size() >= limit) {
                break;
            }
        }
    }

    return results;
}

std::string CallGraph::build_context(
    const std::string& query,
    int limit
) const {
    auto results = search(query, limit);

    std::stringstream ss;
    ss << "CALL GRAPH\n";

    if (results.empty()) {
        ss << "No matching call graph entries found.\n";
        return ss.str();
    }

    for (const auto& fn : results) {
        ss << "- function " << fn.function_name
           << " in " << fn.file_path
           << " lines " << fn.line_start << "-" << fn.line_end
           << "\n";

        if (fn.calls.empty()) {
            ss << "  calls: none\n";
            continue;
        }

        ss << "  calls:\n";

        int shown = 0;
        for (const auto& call : fn.calls) {
            if (shown >= 20) break;
            ss << "    - " << call << "\n";
            shown++;
        }
    }

    return ss.str();
}

std::string CallGraph::lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return value;
}

bool CallGraph::contains_case_insensitive(
    const std::string& text,
    const std::string& query
) {
    return lower(text).find(lower(query)) != std::string::npos;
}

size_t CallGraph::size() const {
    return functions_.size();
}