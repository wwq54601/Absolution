#include "indexer.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

Indexer::Indexer(const std::string& workspace_path)
    : workspace_path_(workspace_path) {}

std::vector<CodeFile> Indexer::scan() {
    std::vector<CodeFile> files;

    for (auto& entry : std::filesystem::recursive_directory_iterator(workspace_path_)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();

        if (!should_index(path)) continue;

        CodeFile file;
        file.path = path;
        file.content = read_file(path);

        if (!file.content.empty()) {
            files.push_back(file);
        }
    }

    return files;
}

bool Indexer::should_index(const std::string& path) const {
    std::string p = path;
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    const std::vector<std::string> ignored = {
        "node_modules",
        ".git",
        "build",
        "dist",
        ".next",
        ".venv",
        "target"
    };

    for (const auto& item : ignored) {
        if (p.find(item) != std::string::npos) return false;
    }

    const std::vector<std::string> exts = {
        ".cpp", ".h", ".hpp", ".c",
        ".ts", ".tsx", ".js", ".jsx",
        ".py", ".rs", ".go", ".java",
        ".json", ".md", ".cmake"
    };

    for (const auto& ext : exts) {
        if (p.size() >= ext.size() &&
            p.substr(p.size() - ext.size()) == ext) {
            return true;
        }
    }

    return false;
}

std::string Indexer::read_file(const std::string& path) const {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}