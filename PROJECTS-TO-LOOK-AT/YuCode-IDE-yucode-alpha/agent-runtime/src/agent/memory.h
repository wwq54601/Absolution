#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "../util/path_utils.h"
#include <unordered_map>

struct AgentMemory {
    std::vector<std::string> visited_files;
    std::vector<std::string> edited_files;
    std::vector<std::string> observations;
    std::unordered_map<std::string, std::string> file_contents;

    void remember_file(const std::string& file) {
    visited_files.push_back(normalize_path(file));
}

    void remember_edit(const std::string& file) {
    edited_files.push_back(normalize_path(file));
}

    void remember_observation(const std::string& text) {
        observations.push_back(text);
    }

    bool has_visited_file(const std::string& file) const {
    std::string target = normalize_path(file);

    return std::find(
        visited_files.begin(),
        visited_files.end(),
        target
    ) != visited_files.end();
}

void remember_file_content(const std::string& file, const std::string& content) {
    file_contents[normalize_path(file)] = content;
}

bool has_file_content(const std::string& file) const {
    return file_contents.find(normalize_path(file)) != file_contents.end();
}
};