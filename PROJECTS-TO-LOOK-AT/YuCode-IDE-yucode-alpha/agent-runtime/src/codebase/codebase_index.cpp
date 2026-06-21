#include "codebase_index.h"
#include "../util/path_utils.h"
#include "../config/yucode_config.h"
#include "../embeddings/embedding_cache.h"
#include <sstream>
#include <fstream>

#include <algorithm>
#include "../indexing/file_analyzer.h"

CodebaseIndex::CodebaseIndex(const std::string& workspace_path)
    : workspace_path_(normalize_path(workspace_path)) {}

void CodebaseIndex::build() {
    Indexer indexer(workspace_path_);
    files_ = indexer.scan();

    file_map_.clear();

    for (const auto& file : files_) {
        file_map_[normalize_path(file.path)] = file.content;
    }

    file_analysis_cache_.clear();

FileAnalyzer analyzer;

for (const auto& file : files_) {
    file_analysis_cache_[normalize_path(file.path)] =
        analyzer.analyze(file);
}

    rebuild_derived_indexes(true);
}

const CallGraph& CodebaseIndex::calls() const {
    return call_graph_;
}

const ReferenceGraph& CodebaseIndex::references() const {
    return reference_graph_;
}

const std::string& CodebaseIndex::workspace_path() const {
    return workspace_path_;
}

const std::vector<CodeFile>& CodebaseIndex::files() const {
    return files_;
}

const SymbolIndex& CodebaseIndex::symbols() const {
    return symbol_index_;
}

std::string CodebaseIndex::get_file_content(const std::string& file_path) const {
    std::string normalized = normalize_path(file_path);

    auto it = file_map_.find(normalized);
    if (it == file_map_.end()) return "";

    return it->second;
}

std::vector<CodeFile> CodebaseIndex::search_files(const std::string& query, int limit) const {
    std::vector<CodeFile> results;

    for (const auto& file : files_) {
        if (
            contains_case_insensitive(file.path, query) ||
            contains_case_insensitive(file.content, query)
        ) {
            results.push_back(file);
            if ((int)results.size() >= limit) break;
        }
    }

    if (results.empty()) {
        for (const auto& file : files_) {
            results.push_back(file);
            if ((int)results.size() >= limit) break;
        }
    }

    return results;
}

bool CodebaseIndex::contains_case_insensitive(
    const std::string& text,
    const std::string& query
) const {
    std::string a = text;
    std::string b = query;

    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);

    return a.find(b) != std::string::npos;
}

void CodebaseIndex::build_vector_index() {
    vector_index_.clear();

    YuCodeConfig config = YuCodeConfig::load();

    if (!config.embedding_enabled) {
        return;
    }

    embedding_request_.provider = config.embedding_provider;
    embedding_request_.base_url = config.embedding_base_url;
    embedding_request_.model = config.embedding_model;

    OpenAICompatibleEmbeddingProvider openai_provider;
    OllamaEmbeddingProvider ollama_provider;

    EmbeddingProvider* provider = &openai_provider;

    if (embedding_request_.provider == "ollama") {
        provider = &ollama_provider;
    }

    EmbeddingCache cache(workspace_path_);
    cache.load();

    int chunk_id = 0;
    bool cache_changed = false;

    for (const auto& file : files_) {
        auto chunks = chunk_content(file.content);

        for (const auto& chunk : chunks) {
            std::string key = EmbeddingCache::make_key(
                file.path,
                chunk
            );

            CachedEmbedding cached;

            if (cache.get(key, cached)) {
                vector_index_.add(
                    "chunk_" + std::to_string(chunk_id++),
                    cached.file_path,
                    cached.content,
                    cached.embedding
                );

                continue;
            }

            auto embedding = provider->embed(chunk, embedding_request_);

            if (embedding.empty()) {
                continue;
            }

            vector_index_.add(
                "chunk_" + std::to_string(chunk_id++),
                file.path,
                chunk,
                embedding
            );

            cache.put(
                key,
                file.path,
                chunk,
                embedding
            );

            cache_changed = true;
        }
    }

    if (cache_changed) {
        cache.save();
    }
}

std::vector<std::string> CodebaseIndex::chunk_content(const std::string& content) const {
    std::vector<std::string> chunks;

    const size_t max_size = 4000;

    for (size_t i = 0; i < content.size(); i += max_size) {
        chunks.push_back(content.substr(i, max_size));
    }

    return chunks;
}

std::vector<VectorSearchResult> CodebaseIndex::semantic_search(
    const std::string& query,
    int limit
) const {
    if (vector_index_.empty()) {
        return {};
    }

    OpenAICompatibleEmbeddingProvider provider;
    auto query_embedding = provider.embed(query, embedding_request_);

    if (query_embedding.empty()) {
        return {};
    }

    return vector_index_.search(query_embedding, limit);
}

size_t CodebaseIndex::vector_size() const {
    return vector_index_.size();
}

void CodebaseIndex::rebuild_derived_indexes(bool rebuild_embeddings) {
    symbol_index_.build_from_analysis_cache(file_analysis_cache_);

    reference_graph_.build(files_, symbol_index_);
    call_graph_.build(files_);

    if (rebuild_embeddings) {
        build_vector_index();
    }
}

void CodebaseIndex::update_file(const std::string& file_path) {
    std::string normalized = normalize_path(file_path);

    std::ifstream in(normalized, std::ios::binary);

    if (!in.is_open()) {
        remove_file(normalized);
        return;
    }

    std::stringstream ss;
    ss << in.rdbuf();

    CodeFile updated;
    updated.path = normalized;
    updated.content = ss.str();

    bool replaced = false;

    for (auto& file : files_) {
        if (normalize_path(file.path) == normalized) {
            file = updated;
            replaced = true;
            break;
        }
    }

    if (!replaced) {
        files_.push_back(updated);
    }

    file_map_[normalized] = updated.content;

    FileAnalyzer analyzer;

FileAnalysis analysis = analyzer.analyze(updated);

file_analysis_cache_[normalized] = analysis;

symbol_index_.update_file(normalized, analysis);

reference_graph_.update_file(
    normalized,
    updated.content,
    symbol_index_
);
call_graph_.update_file(
    normalized,
    updated.content
);
}

void CodebaseIndex::remove_file(const std::string& file_path) {
    std::string normalized = normalize_path(file_path);

    files_.erase(
        std::remove_if(
            files_.begin(),
            files_.end(),
            [&](const CodeFile& file) {
                return normalize_path(file.path) == normalized;
            }
        ),
        files_.end()
    );

    file_map_.erase(normalized);
file_analysis_cache_.erase(normalized);

symbol_index_.remove_file(normalized);

reference_graph_.remove_file(normalized);
call_graph_.remove_file(normalized);
}