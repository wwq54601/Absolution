#include "embedding_cache.h"
#include "../util/path_utils.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

EmbeddingCache::EmbeddingCache(const std::string& workspace_path) {
    std::filesystem::path root = normalize_path(workspace_path);
    std::filesystem::path dir = root / ".yucode" / "cache";

    std::filesystem::create_directories(dir);

    cache_path_ = (dir / "embeddings.tsv").string();
}

void EmbeddingCache::load() {
    items_.clear();

    std::ifstream in(cache_path_, std::ios::binary);
    if (!in.is_open()) return;

    std::string line;

    while (std::getline(in, line)) {
        std::stringstream ss(line);

        std::string key;
        std::string file_path;
        std::string content;
        std::string vector_text;

        if (!std::getline(ss, key, '\t')) continue;
        if (!std::getline(ss, file_path, '\t')) continue;
        if (!std::getline(ss, content, '\t')) continue;
        if (!std::getline(ss, vector_text, '\t')) continue;

        CachedEmbedding item;
        item.key = key;
        item.file_path = unescape(file_path);
        item.content = unescape(content);
        item.embedding = parse_vector(vector_text);

        if (!item.embedding.empty()) {
            items_[key] = item;
        }
    }
}

void EmbeddingCache::save() {
    std::ofstream out(cache_path_, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return;

    for (const auto& pair : items_) {
        const auto& item = pair.second;

        out << item.key << "\t"
            << escape(item.file_path) << "\t"
            << escape(item.content) << "\t"
            << vector_to_string(item.embedding) << "\n";
    }
}

bool EmbeddingCache::get(
    const std::string& key,
    CachedEmbedding& out
) const {
    auto it = items_.find(key);

    if (it == items_.end()) {
        return false;
    }

    out = it->second;
    return true;
}

void EmbeddingCache::put(
    const std::string& key,
    const std::string& file_path,
    const std::string& content,
    const std::vector<float>& embedding
) {
    if (embedding.empty()) return;

    CachedEmbedding item;
    item.key = key;
    item.file_path = file_path;
    item.content = content;
    item.embedding = embedding;

    items_[key] = item;
}

std::string EmbeddingCache::make_key(
    const std::string& file_path,
    const std::string& content
) {
    return normalize_path(file_path) + "::" + hash_text(content);
}

std::string EmbeddingCache::hash_text(const std::string& text) {
    // FNV-1a 64-bit
    uint64_t hash = 1469598103934665603ull;

    for (unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ull;
    }

    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string EmbeddingCache::escape(const std::string& text) {
    std::string out;

    for (char c : text) {
        if (c == '\\') out += "\\\\";
        else if (c == '\t') out += "\\t";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }

    return out;
}

std::string EmbeddingCache::unescape(const std::string& text) {
    std::string out;
    bool escaped = false;

    for (char c : text) {
        if (escaped) {
            if (c == 't') out += '\t';
            else if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == '\\') out += '\\';
            else out += c;

            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        out += c;
    }

    return out;
}

std::vector<float> EmbeddingCache::parse_vector(const std::string& text) {
    std::vector<float> values;
    std::stringstream ss(text);
    std::string item;

    while (std::getline(ss, item, ',')) {
        try {
            values.push_back(std::stof(item));
        } catch (...) {
        }
    }

    return values;
}

std::string EmbeddingCache::vector_to_string(const std::vector<float>& values) {
    std::stringstream ss;
    ss << std::setprecision(8);

    for (size_t i = 0; i < values.size(); i++) {
        if (i > 0) ss << ",";
        ss << values[i];
    }

    return ss.str();
}