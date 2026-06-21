#pragma once
#include <string>
#include <vector>
#include "action.h"
#include "memory.h"
#include <functional>

struct AgentRequest {
    std::string query;
    std::string workspace_path;
    std::string active_file;
    std::string selected_text;
    std::string extra_context;

    std::string mode; // ask | edit | fix | refactor | test

    std::string project_type;
    std::string build_command;
    std::string test_command;
    std::string project_summary;
    std::string session_context;
};

struct AgentResponse {
    bool success = false;
    std::string final_message;
    std::vector<AgentStepResult> steps;
    std::vector<std::string> pending_change_ids;
};

class ChangeSet;
class CodebaseIndex;

class Agent {
public:
    Agent(ChangeSet* change_set, CodebaseIndex* codebase_index);

    AgentResponse run(const AgentRequest& request);
    AgentResponse run_stream(
    const AgentRequest& request,
    std::function<void(const std::string&)> on_chunk
);

private:
    ChangeSet* change_set_ = nullptr;
    CodebaseIndex* codebase_index_ = nullptr;
    int max_steps_ = 14;
};