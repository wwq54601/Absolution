#include "project_summary.h"
#include "../util/path_utils.h"

#include <filesystem>
#include <sstream>
#include <algorithm>

std::string ProjectSummary::to_prompt() const {
    std::stringstream ss;

    ss << "PROJECT SUMMARY\n";
    ss << "workspace: " << workspace_path << "\n";
    ss << "type: " << project_type << "\n";

    if (!build_command.empty()) {
        ss << "build_command: " << build_command << "\n";
    }

    if (!test_command.empty()) {
        ss << "test_command: " << test_command << "\n";
    }

    ss << "important_files:\n";
    for (const auto& file : important_files) {
        ss << "- " << file << "\n";
    }

    ss << "top_level_dirs:\n";
    for (const auto& dir : top_level_dirs) {
        ss << "- " << dir << "\n";
    }

    return ss.str();
}

ProjectSummary ProjectSummaryBuilder::build(
    const std::string& workspace_path,
    const std::string& project_type,
    const std::string& build_command,
    const std::string& test_command
) {
    ProjectSummary summary;
    summary.workspace_path = normalize_path(workspace_path);
    summary.project_type = project_type;
    summary.build_command = build_command;
    summary.test_command = test_command;
    summary.important_files = find_important_files(workspace_path);
    summary.top_level_dirs = find_top_level_dirs(workspace_path);

    return summary;
}

std::vector<std::string> ProjectSummaryBuilder::find_important_files(
    const std::string& workspace_path
) {
    std::vector<std::string> names = {
        "README.md",
        "package.json",
        "CMakeLists.txt",
        "pyproject.toml",
        "requirements.txt",
        "Cargo.toml",
        "go.mod",
        "src/main.cpp",
        "src/main.ts",
        "src/main.py",
        "main.cpp",
        "main.py"
    };

    std::vector<std::string> found;
    std::string root = normalize_path(workspace_path);

    for (const auto& name : names) {
        std::filesystem::path p = std::filesystem::path(root) / name;

        if (std::filesystem::exists(p)) {
            found.push_back(normalize_path(p.string()));
        }
    }

    return found;
}

std::vector<std::string> ProjectSummaryBuilder::find_top_level_dirs(
    const std::string& workspace_path
) {
    std::vector<std::string> dirs;
    std::string root = normalize_path(workspace_path);

    for (auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().string();

        if (
            name == ".git" ||
            name == "node_modules" ||
            name == "build" ||
            name == "dist" ||
            name == ".venv" ||
            name == "target"
        ) {
            continue;
        }

        dirs.push_back(name);

        if (dirs.size() >= 20) break;
    }

    std::sort(dirs.begin(), dirs.end());
    return dirs;
}