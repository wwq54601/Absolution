#pragma once
#include <string>
#include <algorithm>
#include <filesystem>

inline std::string normalize_path(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');

    try {
        std::filesystem::path p(path);
        if (p.is_relative()) {
            p = std::filesystem::absolute(p);
        }

        std::string result = p.lexically_normal().string();
        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    } catch (...) {
        return path;
    }
}

inline bool same_path(const std::string& a, const std::string& b) {
    return normalize_path(a) == normalize_path(b);
}