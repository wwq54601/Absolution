#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "symbol_index.h"
#include "indexer.h"

struct ReferenceLocation {
    std::string file_path;
    int line = 0;
    std::string line_text;
};

struct SymbolReferenceInfo {
    Symbol symbol;
    std::vector<ReferenceLocation> references;
};

class ReferenceGraph {
public:
    void build(
        const std::vector<CodeFile>& files,
        const SymbolIndex& symbol_index
    );

    std::vector<SymbolReferenceInfo> search(
        const std::string& query,
        int limit = 20
    ) const;

    std::string build_context(
        const std::string& query,
        int limit = 20
    ) const;

    size_t size() const;

    void update_file(
    const std::string& file_path,
    const std::string& content,
    const SymbolIndex& symbol_index
    );

    void remove_file(const std::string& file_path);

private:
    std::vector<SymbolReferenceInfo> graph_;

    std::vector<ReferenceLocation> find_references(
        const Symbol& symbol,
        const std::vector<CodeFile>& files
    ) const;

    static std::string lower(std::string value);
    static bool contains_word(const std::string& text, const std::string& word);
    std::vector<ReferenceLocation> find_references_in_content(
    const Symbol& symbol,
    const std::string& file_path,
    const std::string& content
) const;
};