#include "planner.h"
#include "../llm/llm_client.h"
#include <sstream>

Planner::Planner(LLMClient* llm) : llm_(llm) {}

std::string Planner::create_plan(const AgentRequest& request) {
    std::stringstream prompt;

    prompt << R"(
You are YuCode Planner.

Create a compact implementation plan for a coding agent.

Return plain text only.

Include:
- goal
- likely files
- symbols
- edit strategy
- verification strategy

User request:
)";

    prompt << request.query << "\n";

    prompt << "\nMode:\n" << request.mode << "\n";

    if (!request.project_summary.empty()) {
    prompt << "\n" << request.project_summary << "\n";
}

    if (!request.session_context.empty()) {
    prompt << "\n" << request.session_context << "\n";
}

    if (!request.active_file.empty()) {
        prompt << "\nActive file:\n" << request.active_file << "\n";
    }

    if (!request.selected_text.empty()) {
    prompt << "\nSelected text:\n";
    prompt << request.selected_text.substr(0, 8000) << "\n";

    if (!request.extra_context.empty()) {
    prompt << "\nExtra context:\n";
    prompt << request.extra_context.substr(0, 12000) << "\n";
}

    prompt << "\nProject type:\n" << request.project_type << "\n";

if (!request.build_command.empty()) {
    prompt << "\nBuild command:\n" << request.build_command << "\n";
}

if (!request.test_command.empty()) {
    prompt << "\nTest command:\n" << request.test_command << "\n";
}
}

    return llm_->generate(prompt.str());
}