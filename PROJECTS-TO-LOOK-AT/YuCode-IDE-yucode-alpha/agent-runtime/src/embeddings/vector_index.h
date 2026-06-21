#pragma once
#include <string>
#include <vector>

struct EmbeddedChunk {
    std::string id;
    std::string file_path;
    std::string content;
    std::vector<float> embedding;
};

struct VectorSearchResult {
    std::string file_path;
    std::string content;
    float score = 0.0f;
};

class VectorIndex {
public:
    void clear();

    void add(
        const std::string& id,
        const std::string& file_path,
        const std::string& content,
        const std::vector<float>& embedding
    );

    size_t size() const;

    std::vector<VectorSearchResult> search(
        const std::vector<float>& query_embedding,
        int limit = 5
    ) const;

    bool empty() const;

private:
    std::vector<EmbeddedChunk> chunks_;

    float cosine_similarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const;
};