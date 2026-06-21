#include "file_editor.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

static std::filesystem::path to_fs_path(const std::string& path) {
    std::string fixed = path;

#ifdef _WIN32
    std::replace(fixed.begin(), fixed.end(), '/', '\\');
#endif

    return std::filesystem::path(fixed);
}

std::string FileEditor::read_file(const std::string& path) {
    auto fs_path = to_fs_path(path);

    std::ifstream file(fs_path, std::ios::binary);
    if (!file.is_open()) return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool FileEditor::rewrite_file(const std::string& path, const std::string& content) {
    auto fs_path = to_fs_path(path);

    if (!fs_path.parent_path().empty()) {
        std::filesystem::create_directories(fs_path.parent_path());
    }

    static constexpr bool kCreateBackups = false;

if (kCreateBackups && std::filesystem::exists(fs_path)) {
    if (!backup_file(fs_path.string())) {
        return false;
    }
}

    std::ofstream file(fs_path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;

    file << content;
    return true;
}

bool FileEditor::file_exists(const std::string& path) const {
    return std::filesystem::exists(to_fs_path(path));
}

bool FileEditor::backup_file(const std::string& path) {
    try {
        auto fs_path = to_fs_path(path);

        std::filesystem::copy_file(
            fs_path,
            fs_path.string() + ".yucode.bak",
            std::filesystem::copy_options::overwrite_existing
        );

        return true;
    } catch (...) {
        return false;
    }
}