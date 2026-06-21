#include "session_store.h"

#include <sstream>

void SessionStore::add_user_message(const std::string& content) {
    messages_.push_back({"user", content});
}

void SessionStore::add_assistant_message(const std::string& content) {
    messages_.push_back({"assistant", content});
}

void SessionStore::add_system_message(const std::string& content) {
    messages_.push_back({"system", content});
}

std::string SessionStore::build_prompt_context(int max_messages) const {
    std::stringstream ss;

    ss << "SESSION HISTORY\n";

    if (messages_.empty()) {
        ss << "No previous messages.\n";
        return ss.str();
    }

    int start = 0;
    if ((int)messages_.size() > max_messages) {
        start = (int)messages_.size() - max_messages;
    }

    for (int i = start; i < (int)messages_.size(); i++) {
        ss << messages_[i].role << ": ";
        ss << messages_[i].content.substr(0, 1200) << "\n";
    }

    return ss.str();
}

void SessionStore::clear() {
    messages_.clear();
}