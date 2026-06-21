#pragma once
#include <string>
#include <vector>

struct ProjectInfo {
    std::string type;
    std::string build_command;
    std::string test_command;
    std::vector<std::string> detected_files;
};

class ProjectDetector {
public:
    ProjectInfo detect(const std::string& workspace_path);

private:
    bool exists(const std::string& path) const;
};