#include "project_detector.h"
#include "../util/path_utils.h"

#include <filesystem>

bool ProjectDetector::exists(const std::string& path) const {
    return std::filesystem::exists(normalize_path(path));
}

ProjectInfo ProjectDetector::detect(const std::string& workspace_path) {
    std::string root = normalize_path(workspace_path);

    ProjectInfo info;
    info.type = "unknown";

    auto has = [&](const std::string& file) {
        std::string path = root + "/" + file;
        if (exists(path)) {
            info.detected_files.push_back(file);
            return true;
        }

        return false;
    };

    if (has("CMakeLists.txt")) {
        info.type = "cmake-cpp";
        info.build_command = "cmake --build build --config Release";
        info.test_command = "ctest --test-dir build --output-on-failure";
        return info;
    }

    if (has("package.json")) {
        info.type = "node";
        info.build_command = "npm run build";
        info.test_command = "npm test";
        return info;
    }

    if (has("pyproject.toml")) {
        info.type = "python";
        info.build_command = "";
        info.test_command = "python -m pytest";
        return info;
    }

    if (has("requirements.txt")) {
        info.type = "python";
        info.build_command = "";
        info.test_command = "python -m pytest";
        return info;
    }

    if (has("Cargo.toml")) {
        info.type = "rust";
        info.build_command = "cargo build";
        info.test_command = "cargo test";
        return info;
    }

    if (has("go.mod")) {
        info.type = "go";
        info.build_command = "go build ./...";
        info.test_command = "go test ./...";
        return info;
    }

    return info;
}