#pragma once
#include <string>

inline std::string extract_json(const std::string& raw) {
    size_t start = raw.find('{');
    size_t end = raw.rfind('}');

    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return raw;
    }

    return raw.substr(start, end - start + 1);
}