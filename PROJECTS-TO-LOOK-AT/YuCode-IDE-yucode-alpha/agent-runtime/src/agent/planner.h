#pragma once
#include <string>
#include "agent.h"

class LLMClient;

class Planner {
public:
    explicit Planner(LLMClient* llm);

    std::string create_plan(const AgentRequest& request);

private:
    LLMClient* llm_;
};