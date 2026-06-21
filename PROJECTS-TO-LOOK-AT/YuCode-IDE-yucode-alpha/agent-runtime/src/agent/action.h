#pragma once
#include <string>

enum class ActionType {
    SearchCode,
    SearchSymbol,
    SearchReferences,
    SearchCalls,
    AnalyzeImpact,
    SemanticSearch,
    ReadFile,
    EditFile,
    CreateFile,
    ApplyPatch,
    ApplyUnifiedDiff,
    ApplyAstPatch,
    RunCommand,
    Done,
    Error
};

struct AgentAction {
    ActionType type = ActionType::Error;

    std::string query;
    std::string file_path;
    std::string content;
    std::string find_text;
    std::string replace_text;
    std::string command;
    std::string explanation;
    std::string unified_diff;
    std::string symbol;
    std::string kind;
    std::string replacement;
};

struct AgentStepResult {
    bool success = false;
    AgentAction action;
    std::string output;
};