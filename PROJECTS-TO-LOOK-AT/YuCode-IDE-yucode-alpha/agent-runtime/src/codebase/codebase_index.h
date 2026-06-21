#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "indexer.h"
#include "symbol_index.h"
#include "reference_graph.h"
#include "call_graph.h"
#include "../embeddings/vector_index.h"
#include "../embeddings/embedding_provider.h"
#include "../indexing/file_analysis.h"

class CodebaseIndex {
public:
    explicit CodebaseIndex(const std::string& workspace_path);

    void build();

    const std::string& workspace_path() const;
    const std::vector<CodeFile>& files() const;
    const SymbolIndex& symbols() const;
    const ReferenceGraph& references() const;
    const CallGraph& calls() const;
    size_t vector_size() const;

    void update_file(const std::string& file_path);
void remove_file(const std::string& file_path);

    std::vector<VectorSearchResult> semantic_search(
    const std::string& query,
    int limit = 5
) const;

    std::string get_file_content(const std::string& file_path) const;
    std::vector<CodeFile> search_files(const std::string& query, int limit = 5) const;

private:
    std::string workspace_path_;
    std::vector<CodeFile> files_;
    std::unordered_map<std::string, std::string> file_map_;
    SymbolIndex symbol_index_;
    ReferenceGraph reference_graph_;
    CallGraph call_graph_;
    void rebuild_derived_indexes(bool rebuild_embeddings);
    std::unordered_map<std::string, FileAnalysis> file_analysis_cache_;

    VectorIndex vector_index_;
EmbeddingRequest embedding_request_;

void build_vector_index();
std::vector<std::string> chunk_content(const std::string& content) const;

    bool contains_case_insensitive(
        const std::string& text,
        const std::string& query
    ) const;
};