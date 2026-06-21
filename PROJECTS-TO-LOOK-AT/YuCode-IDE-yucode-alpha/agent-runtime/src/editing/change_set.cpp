#include "change_set.h"

std::string ChangeSet::create(
    const std::string& file_path,
    const std::string& old_content,
    const std::string& new_content,
    const std::string& explanation
) {
    return create(
        file_path,
        old_content,
        new_content,
        explanation,
        ""
    );
}

std::string ChangeSet::create(
    const std::string& file_path,
    const std::string& old_content,
    const std::string& new_content,
    const std::string& explanation,
    const std::string& unified_diff
) {
    std::string id = "change_" + std::to_string(next_id_++);

    PendingChange change;
    change.id = id;
    change.file_path = file_path;
    change.old_content = old_content;
    change.new_content = new_content;
    change.explanation = explanation;
    change.unified_diff = unified_diff;

    changes_[id] = change;

    return id;
}

std::string ChangeSet::create_multi(
    const std::vector<PendingChangeFile>& files,
    const std::string& explanation
) {
    std::string id = "change_" + std::to_string(next_id_++);

    PendingChange change;
    change.id = id;
    change.explanation = explanation;
    change.files = files;

    if (!files.empty()) {
        change.file_path = files[0].file_path;
        change.old_content = files[0].old_content;
        change.new_content = files[0].new_content;
        change.unified_diff = files[0].unified_diff;
    }

    changes_[id] = change;

    return id;
}

bool ChangeSet::get(const std::string& id, PendingChange& out) const {
    auto it = changes_.find(id);
    if (it == changes_.end()) return false;

    out = it->second;
    return true;
}

bool ChangeSet::remove(const std::string& id) {
    return changes_.erase(id) > 0;
}

std::vector<PendingChange> ChangeSet::list() const {
    std::vector<PendingChange> result;

    for (const auto& pair : changes_) {
        result.push_back(pair.second);
    }

    return result;
}