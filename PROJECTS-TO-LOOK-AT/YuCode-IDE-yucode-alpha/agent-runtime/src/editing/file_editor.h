#pragma once
#include <string>

class FileEditor {
public:
    std::string read_file(const std::string& path);

    bool rewrite_file(
        const std::string& path,
        const std::string& content
    );

    bool file_exists(const std::string& path) const;

private:
    bool backup_file(const std::string& path);
};