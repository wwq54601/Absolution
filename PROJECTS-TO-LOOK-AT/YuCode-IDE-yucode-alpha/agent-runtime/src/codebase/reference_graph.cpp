#include "reference_graph.h"

#include <sstream>
#include <algorithm>
#include <cctype>

void ReferenceGraph::build(
    const std::vector<CodeFile>& files,
    const SymbolIndex& symbol_index
) {
    graph_.clear();

    for (const auto& file : files) {
        auto symbols = symbol_index.symbols_for_file(file.path);

        for (const auto& symbol : symbols) {
            SymbolReferenceInfo info;
            info.symbol = symbol;
            info.references = find_references(symbol, files);
            graph_.push_back(info);
        }
    }
}

std::vector<SymbolReferenceInfo> ReferenceGraph::search(
    const std::string& query,
    int limit
) const {
    std::vector<SymbolReferenceInfo> results;
    std::string q = lower(query);

    for (const auto& item : graph_) {
        std::string name = lower(item.symbol.name);
        std::string file = lower(item.symbol.file_path);
        std::string sig = lower(item.symbol.signature);

        if (
            name.find(q) != std::string::npos ||
            file.find(q) != std::string::npos ||
            sig.find(q) != std::string::npos ||
            q.find(name) != std::string::npos
        ) {
            results.push_back(item);

            if ((int)results.size() >= limit) {
                break;
            }
        }
    }

    return results;
}

std::string ReferenceGraph::build_context(
    const std::string& query,
    int limit
) const {
    auto results = search(query, limit);

    std::stringstream ss;
    ss << "REFERENCE GRAPH\n";

    if (results.empty()) {
        ss << "No matching references found.\n";
        return ss.str();
    }

    for (const auto& item : results) {
        ss << "- " << item.symbol.kind << " " << item.symbol.name
           << " defined in " << item.symbol.file_path
           << " lines " << item.symbol.line_start << "-" << item.symbol.line_end
           << "\n";

        int shown = 0;

        for (const auto& ref : item.references) {
            if (shown >= 8) break;

            ss << "  ref: " << ref.file_path
               << ":" << ref.line
               << " | " << ref.line_text
               << "\n";

            shown++;
        }
    }

    return ss.str();
}

std::vector<ReferenceLocation> ReferenceGraph::find_references(
    const Symbol& symbol,
    const std::vector<CodeFile>& files
) const {
    std::vector<ReferenceLocation> refs;

    if (symbol.name.empty()) {
        return refs;
    }

    std::string simple_name = symbol.name;

    size_t scope_pos = simple_name.rfind("::");
    if (scope_pos != std::string::npos) {
        simple_name = simple_name.substr(scope_pos + 2);
    }

    for (const auto& file : files) {
        std::istringstream stream(file.content);
        std::string line;
        int line_number = 1;

        while (std::getline(stream, line)) {
            if (contains_word(line, simple_name)) {
                ReferenceLocation ref;
                ref.file_path = file.path;
                ref.line = line_number;
                ref.line_text = line;
                refs.push_back(ref);
            }

            line_number++;
        }
    }

    return refs;
}

std::string ReferenceGraph::lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return value;
}

bool ReferenceGraph::contains_word(
    const std::string& text,
    const std::string& word
) {
    if (word.empty()) return false;

    size_t pos = text.find(word);

    while (pos != std::string::npos) {
        bool left_ok =
            pos == 0 ||
            !(std::isalnum(static_cast<unsigned char>(text[pos - 1])) || text[pos - 1] == '_');

        size_t end = pos + word.size();

        bool right_ok =
            end >= text.size() ||
            !(std::isalnum(static_cast<unsigned char>(text[end])) || text[end] == '_');

        if (left_ok && right_ok) {
            return true;
        }

        pos = text.find(word, pos + word.size());
    }

    return false;
}

size_t ReferenceGraph::size() const {
    return graph_.size();
}

void ReferenceGraph::update_file(
    const std::string& file_path,
    const std::string& content,
    const SymbolIndex& symbol_index
) {
    remove_file(file_path);

    auto file_symbols = symbol_index.symbols_for_file(file_path);

    for (const auto& symbol : file_symbols) {
        SymbolReferenceInfo info;
        info.symbol = symbol;
        graph_.push_back(info);
    }

    for (auto& item : graph_) {
        auto refs = find_references_in_content(
            item.symbol,
            file_path,
            content
        );

        item.references.insert(
            item.references.end(),
            refs.begin(),
            refs.end()
        );
    }
}

void ReferenceGraph::remove_file(const std::string& file_path) {
    graph_.erase(
        std::remove_if(
            graph_.begin(),
            graph_.end(),
            [&](const SymbolReferenceInfo& item) {
                return item.symbol.file_path == file_path;
            }
        ),
        graph_.end()
    );

    for (auto& item : graph_) {
        item.references.erase(
            std::remove_if(
                item.references.begin(),
                item.references.end(),
                [&](const ReferenceLocation& ref) {
                    return ref.file_path == file_path;
                }
            ),
            item.references.end()
        );
    }
}

std::vector<ReferenceLocation> ReferenceGraph::find_references_in_content(
    const Symbol& symbol,
    const std::string& file_path,
    const std::string& content
) const {
    std::vector<ReferenceLocation> refs;

    if (symbol.name.empty()) {
        return refs;
    }

    std::string simple_name = symbol.name;

    size_t scope_pos = simple_name.rfind("::");
    if (scope_pos != std::string::npos) {
        simple_name = simple_name.substr(scope_pos + 2);
    }

    std::istringstream stream(content);
    std::string line;
    int line_number = 1;

    while (std::getline(stream, line)) {
        if (contains_word(line, simple_name)) {
            ReferenceLocation ref;
            ref.file_path = file_path;
            ref.line = line_number;
            ref.line_text = line;
            refs.push_back(ref);
        }

        line_number++;
    }

    return refs;
}