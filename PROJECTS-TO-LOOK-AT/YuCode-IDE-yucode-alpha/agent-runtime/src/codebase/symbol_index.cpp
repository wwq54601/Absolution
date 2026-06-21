#include "symbol_index.h"
#include "../parsing/tree_sitter_cpp_parser.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

void SymbolIndex::build(const std::string& workspace_path) {
    symbols_.clear();
    by_file_.clear();

    for (auto& entry : std::filesystem::recursive_directory_iterator(workspace_path)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();

        if (!should_index(path)) continue;

        index_file(path);
    }
}

void SymbolIndex::build_from_analysis_cache(
    const std::unordered_map<std::string, FileAnalysis>& cache
) {
    symbols_.clear();
    by_file_.clear();

    for (const auto& [file_path, analysis] : cache) {
        add_symbols_from_analysis(file_path, analysis);
    }
}

void SymbolIndex::update_file(
    const std::string& file_path,
    const FileAnalysis& analysis
) {
    remove_file(file_path);
    add_symbols_from_analysis(file_path, analysis);
}

void SymbolIndex::remove_file(const std::string& file_path) {
    by_file_.erase(file_path);

    symbols_.erase(
        std::remove_if(
            symbols_.begin(),
            symbols_.end(),
            [&](const Symbol& symbol) {
                return symbol.file_path == file_path;
            }
        ),
        symbols_.end()
    );
}

void SymbolIndex::add_symbols_from_analysis(
    const std::string& file_path,
    const FileAnalysis& analysis
) {
    for (const auto& item : analysis.symbols) {
        Symbol symbol;
        symbol.kind = item.kind;
        symbol.name = item.name;
        symbol.file_path = file_path;
        symbol.line_start = item.line_start;
        symbol.line_end = item.line_end;
        symbol.signature = item.signature;

        add_symbol(symbol);
    }
}

std::vector<Symbol> SymbolIndex::search(const std::string& query, int limit) const {
    std::vector<Symbol> results;
    std::string q = lower(query);

    for (const auto& symbol : symbols_) {
        std::string name = lower(symbol.name);
        std::string sig = lower(symbol.signature);
        std::string file = lower(symbol.file_path);

        bool match =
            name.find(q) != std::string::npos ||
            sig.find(q) != std::string::npos ||
            file.find(q) != std::string::npos ||
            q.find(name) != std::string::npos;

        if (match) {
            results.push_back(symbol);
            if ((int)results.size() >= limit) break;
        }
    }

    return results;
}

std::vector<Symbol> SymbolIndex::symbols_for_file(const std::string& file_path) const {
    auto it = by_file_.find(file_path);
    if (it == by_file_.end()) return {};
    return it->second;
}

std::string SymbolIndex::build_context(const std::string& query, int limit) const {
    auto results = search(query, limit);

    std::stringstream ss;
    ss << "SYMBOL INDEX\n";

    if (results.empty()) {
        ss << "No matching symbols found.\n";
        return ss.str();
    }

    for (const auto& symbol : results) {
        ss << "- " << symbol.kind << " " << symbol.name
           << " in " << symbol.file_path
           << " lines " << symbol.line_start << "-" << symbol.line_end
           << "\n  signature: " << symbol.signature << "\n";
    }

    return ss.str();
}

static bool ends_with(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.substr(text.size() - suffix.size()) == suffix;
}

void SymbolIndex::index_file(const std::string& file_path) {
    std::string content = read_file(file_path);
    if (content.empty()) return;

    std::string p = lower(file_path);

    if (
        ends_with(p, ".cpp") ||
        ends_with(p, ".cc") ||
        ends_with(p, ".cxx") ||
        ends_with(p, ".h") ||
        ends_with(p, ".hpp")
    ) {
        extract_cpp(file_path, content);
    }

    else if (ends_with(p, ".py")) {
        extract_python(file_path, content);
    }

    else if (
        ends_with(p, ".js") ||
        ends_with(p, ".jsx") ||
        ends_with(p, ".ts") ||
        ends_with(p, ".tsx")
    ) {
        extract_js_ts(file_path, content);
    }
}

void SymbolIndex::extract_cpp(const std::string& file_path, const std::string& content) {
    TreeSitterCppParser parser;
    auto nodes = parser.parse_file(file_path, content);

    for (const auto& node : nodes) {
        Symbol symbol;
        symbol.kind = node.kind;
        symbol.name = node.name;
        symbol.file_path = node.file_path;
        symbol.line_start = node.line_start;
        symbol.line_end = node.line_end;
        symbol.signature = node.signature;

        add_symbol(symbol);
    }
}

void SymbolIndex::extract_python(const std::string& file_path, const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    int line_number = 1;

    std::regex class_regex(R"(^\s*class\s+([A-Za-z_][A-Za-z0-9_]*))");
    std::regex func_regex(R"(^\s*def\s+([A-Za-z_][A-Za-z0-9_]*)\s*\()");

    while (std::getline(stream, line)) {
        std::smatch match;

        if (std::regex_search(line, match, class_regex)) {
            Symbol symbol;
            symbol.kind = "class";
            symbol.name = match[1];
            symbol.file_path = file_path;
            symbol.line_start = line_number;
            symbol.line_end = line_number;
            symbol.signature = line;
            add_symbol(symbol);
        }

        if (std::regex_search(line, match, func_regex)) {
            Symbol symbol;
            symbol.kind = "function";
            symbol.name = match[1];
            symbol.file_path = file_path;
            symbol.line_start = line_number;
            symbol.line_end = line_number;
            symbol.signature = line;
            add_symbol(symbol);
        }

        line_number++;
    }
}

void SymbolIndex::extract_js_ts(const std::string& file_path, const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    int line_number = 1;

    std::regex class_regex(R"(\bclass\s+([A-Za-z_][A-Za-z0-9_]*))");
    std::regex function_regex(R"(\bfunction\s+([A-Za-z_][A-Za-z0-9_]*)\s*\()");
    std::regex arrow_regex(R"(\b(?:const|let|var)\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>)");

    while (std::getline(stream, line)) {
        std::smatch match;

        if (std::regex_search(line, match, class_regex)) {
            Symbol symbol;
            symbol.kind = "class";
            symbol.name = match[1];
            symbol.file_path = file_path;
            symbol.line_start = line_number;
            symbol.line_end = line_number;
            symbol.signature = line;
            add_symbol(symbol);
        }

        if (std::regex_search(line, match, function_regex)) {
            Symbol symbol;
            symbol.kind = "function";
            symbol.name = match[1];
            symbol.file_path = file_path;
            symbol.line_start = line_number;
            symbol.line_end = line_number;
            symbol.signature = line;
            add_symbol(symbol);
        }

        if (std::regex_search(line, match, arrow_regex)) {
            Symbol symbol;
            symbol.kind = "function";
            symbol.name = match[1];
            symbol.file_path = file_path;
            symbol.line_start = line_number;
            symbol.line_end = line_number;
            symbol.signature = line;
            add_symbol(symbol);
        }

        line_number++;
    }
}

std::string SymbolIndex::read_file(const std::string& file_path) const {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool SymbolIndex::should_index(const std::string& file_path) const {
    std::string p = lower(file_path);

    const std::vector<std::string> ignored = {
        "\\.git\\",
        "/.git/",
        "node_modules",
        "\\build\\",
        "/build/",
        "\\dist\\",
        "/dist/",
        ".next",
        ".venv",
        "\\target\\",
        "/target/"
    };

    for (const auto& item : ignored) {
        if (p.find(item) != std::string::npos) return false;
    }

    const std::vector<std::string> exts = {
        ".cpp", ".cc", ".cxx", ".h", ".hpp",
        ".py",
        ".js", ".jsx", ".ts", ".tsx"
    };

    for (const auto& ext : exts) {
        if (p.size() >= ext.size() && p.substr(p.size() - ext.size()) == ext) {
            return true;
        }
    }

    return false;
}

void SymbolIndex::add_symbol(const Symbol& symbol) {
    symbols_.push_back(symbol);
    by_file_[symbol.file_path].push_back(symbol);
}

std::string SymbolIndex::lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return value;
}

size_t SymbolIndex::size() const {
    return symbols_.size();
}