#include "vector_index.h"

#include <cmath>
#include <algorithm>

void VectorIndex::clear() {
    chunks_.clear();
}

size_t VectorIndex::size() const {
    return chunks_.size();
}

void VectorIndex::add(
    const std::string& id,
    const std::string& file_path,
    const std::string& content,
    const std::vector<float>& embedding
) {
    if (embedding.empty()) return;

    EmbeddedChunk chunk;
    chunk.id = id;
    chunk.file_path = file_path;
    chunk.content = content;
    chunk.embedding = embedding;

    chunks_.push_back(chunk);
}

std::vector<VectorSearchResult> VectorIndex::search(
    const std::vector<float>& query_embedding,
    int limit
) const {
    std::vector<VectorSearchResult> results;

    if (query_embedding.empty()) {
        return results;
    }

    for (const auto& chunk : chunks_) {
        float score = cosine_similarity(query_embedding, chunk.embedding);

        VectorSearchResult result;
        result.file_path = chunk.file_path;
        result.content = chunk.content;
        result.score = score;

        results.push_back(result);
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });

    if ((int)results.size() > limit) {
        results.resize(limit);
    }

    return results;
}

bool VectorIndex::empty() const {
    return chunks_.empty();
}

float VectorIndex::cosine_similarity(
    const std::vector<float>& a,
    const std::vector<float>& b
) const {
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 0.0f;
    }

    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;

    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }

    if (norm_a == 0.0f || norm_b == 0.0f) {
        return 0.0f;
    }

    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}