#pragma once
#include <string>
#include <vector>

struct SessionMessage {
    std::string role;      // user | assistant | system
    std::string content;
};

class SessionStore {
public:
    void add_user_message(const std::string& content);
    void add_assistant_message(const std::string& content);
    void add_system_message(const std::string& content);

    std::string build_prompt_context(int max_messages = 12) const;
    void clear();

private:
    std::vector<SessionMessage> messages_;
};