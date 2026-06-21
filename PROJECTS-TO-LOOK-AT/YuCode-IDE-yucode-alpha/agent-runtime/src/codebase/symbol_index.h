#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "../indexing/file_analysis.h"

struct Symbol {
    std::string name;
    std::string kind;
    std::string file_path;
    int line_start = 0;
    int line_end = 0;
    std::string signature;
};

class SymbolIndex {
public:
    void build(const std::string& workspace_path);

    void build_from_analysis_cache(
        const std::unordered_map<std::string, FileAnalysis>& cache
    );

    void update_file(
        const std::string& file_path,
        const FileAnalysis& analysis
    );

    void remove_file(const std::string& file_path);

    std::vector<Symbol> search(const std::string& query, int limit = 20) const;
    std::vector<Symbol> symbols_for_file(const std::string& file_path) const;

    std::string build_context(const std::string& query, int limit = 20) const;

    size_t size() const;

private:
    std::vector<Symbol> symbols_;
    std::unordered_map<std::string, std::vector<Symbol>> by_file_;

    void index_file(const std::string& file_path);
    void extract_cpp(const std::string& file_path, const std::string& content);
    void extract_python(const std::string& file_path, const std::string& content);
    void extract_js_ts(const std::string& file_path, const std::string& content);

    void add_symbol(const Symbol& symbol);
    void add_symbols_from_analysis(const std::string& file_path, const FileAnalysis& analysis);

    std::string read_file(const std::string& file_path) const;
    bool should_index(const std::string& file_path) const;

    static std::string lower(std::string value);
};