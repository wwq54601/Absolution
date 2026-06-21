#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct PendingChangeFile {
    std::string file_path;
    std::string old_content;
    std::string new_content;
    std::string unified_diff;
};

struct PendingChange {
    std::string id;
    std::string file_path;
    std::string old_content;
    std::string new_content;
    std::string explanation;
    std::string unified_diff;

    std::vector<PendingChangeFile> files;
};

class ChangeSet {
public:

std::string create(
    const std::string& file_path,
    const std::string& old_content,
    const std::string& new_content,
    const std::string& explanation
);

    std::string create(
        const std::string& file_path,
        const std::string& old_content,
        const std::string& new_content,
        const std::string& explanation,
        const std::string& unified_diff
    );

    std::string create_multi(
    const std::vector<PendingChangeFile>& files,
    const std::string& explanation
);

    bool get(const std::string& id, PendingChange& out) const;
    bool remove(const std::string& id);

    std::vector<PendingChange> list() const;

private:
    std::unordered_map<std::string, PendingChange> changes_;
    int next_id_ = 1;
};