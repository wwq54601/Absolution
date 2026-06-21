#pragma once
#include <string>
#include <vector>

struct ProjectSummary {
    std::string workspace_path;
    std::string project_type;
    std::string build_command;
    std::string test_command;

    std::vector<std::string> important_files;
    std::vector<std::string> top_level_dirs;

    std::string to_prompt() const;
};

class ProjectSummaryBuilder {
public:
    ProjectSummary build(
        const std::string& workspace_path,
        const std::string& project_type,
        const std::string& build_command,
        const std::string& test_command
    );

private:
    std::vector<std::string> find_important_files(const std::string& workspace_path);
    std::vector<std::string> find_top_level_dirs(const std::string& workspace_path);
};