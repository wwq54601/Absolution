#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct CachedEmbedding {
    std::string key;
    std::string file_path;
    std::string content;
    std::vector<float> embedding;
};

class EmbeddingCache {
public:
    explicit EmbeddingCache(const std::string& workspace_path);

    void load();
    void save();

    bool get(
        const std::string& key,
        CachedEmbedding& out
    ) const;

    void put(
        const std::string& key,
        const std::string& file_path,
        const std::string& content,
        const std::vector<float>& embedding
    );

    static std::string make_key(
        const std::string& file_path,
        const std::string& content
    );

private:
    std::string cache_path_;
    std::unordered_map<std::string, CachedEmbedding> items_;

    static std::string hash_text(const std::string& text);
    static std::string escape(const std::string& text);
    static std::string unescape(const std::string& text);

    static std::vector<float> parse_vector(const std::string& text);
    static std::string vector_to_string(const std::vector<float>& values);
};