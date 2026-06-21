#pragma once
#include <string>
#include <vector>

struct CodeFile {
    std::string path;
    std::string content;
};

class Indexer {
public:
    explicit Indexer(const std::string& workspace_path);

    std::vector<CodeFile> scan();

private:
    std::string workspace_path_;

    bool should_index(const std::string& path) const;
    std::string read_file(const std::string& path) const;
};