#pragma once
#include <string>
#include "agent.h"
#include "action.h"
#include "memory.h"
#include <functional>

class LLMClient;

class Executor {
public:
    explicit Executor(LLMClient* llm);

    AgentAction next_action(
        const AgentRequest& request,
        const std::string& plan,
        const std::string& context,
        const AgentMemory& memory
    );

    AgentAction next_action_stream(
    const AgentRequest& request,
    const std::string& plan,
    const std::string& context,
    const AgentMemory& memory,
    std::function<void(const std::string&)> on_chunk
);

private:
    LLMClient* llm_;

    AgentAction parse_action(
    const std::string& raw,
    const AgentRequest& request
);
};