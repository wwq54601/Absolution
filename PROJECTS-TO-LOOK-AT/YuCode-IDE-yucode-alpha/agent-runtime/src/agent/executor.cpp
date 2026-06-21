#include "executor.h"
#include "../llm/llm_client.h"

#include <sstream>
#include <string>

Executor::Executor(LLMClient* llm) : llm_(llm) {}

static std::string extract_value(const std::string& raw, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t key_pos = raw.find(pattern);
    if (key_pos == std::string::npos) return "";

    size_t colon = raw.find(":", key_pos);
    if (colon == std::string::npos) return "";

    size_t start = raw.find("\"", colon + 1);
    if (start == std::string::npos) return "";
    start++;

    std::string result;
    bool escaped = false;

    for (size_t i = start; i < raw.size(); i++) {
        char c = raw[i];

        if (escaped) {
            if (c == 'n') result += '\n';
            else if (c == 't') result += '\t';
            else if (c == 'r') result += '\r';
            else result += c;

            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') break;

        result += c;
    }

    return result;
}

static std::string build_executor_prompt(
    const AgentRequest& request,
    const std::string& plan,
    const std::string& context,
    const AgentMemory& memory
)
{
    std::stringstream prompt;

    prompt << R"(
You are YuCode Executor, an autonomous local coding agent.

You control a real codebase through tools.

Return ONLY valid JSON.
STRICT JSON FIELD RULES:
- Every response MUST be a single valid JSON object.
- Every response MUST contain an "action" field.
- Return ONLY JSON.
- Never return markdown.
- Never explain outside JSON.
- Never return custom schemas.
- Never return {"file":"...","changes":[...]}.
- Never return {"summary":"..."}.
- Never return {"type":"insert","position":123}.

SUPPORTED ACTIONS:
- search_code
- search_symbol
- search_references
- search_calls
- analyze_impact
- semantic_search
- read_file
- create_file
- edit_file
- apply_patch
- apply_unified_diff
- apply_ast_patch
- run_command
- done

CREATE FILE:
- Use create_file for new files.
- If the file already exists or appears in READ FILE CONTENTS, do not use create_file.
- For existing files, use apply_patch, apply_unified_diff, or apply_ast_patch.

Schema:
{
  "action": "create_file",
  "file_path": "absolute file path",
  "content": "FULL NEW FILE CONTENT",
  "explanation": "short explanation"
}

EDIT FILE:
Never use edit_file for new files.
Never use edit_file for existing files.

Schema:
{
  "action": "edit_file",
  "file_path": "absolute file path",
  "content": "FULL NEW FILE CONTENT",
  "explanation": "short explanation"
}

APPLY PATCH:
Use apply_patch for existing files.

Schema:
{
  "action": "apply_patch",
  "file_path": "absolute file path",
  "find": "exact old code block copied from READ FILE CONTENTS",
  "replace": "new code block",
  "explanation": "short explanation"
}

APPLY UNIFIED DIFF:
Use apply_unified_diff for complex multi-line edits.

Schema:
{
  "action": "apply_unified_diff",
  "file": "absolute file path",
  "diff": "--- a/file\n+++ b/file\n@@ ...",
  "explanation": "short explanation"
}

APPLY AST PATCH:
Use apply_ast_patch to replace an existing function by symbol name.
Prefer apply_ast_patch over apply_patch when replacing a whole function.

Schema:
{
  "action": "apply_ast_patch",
  "file_path": "absolute file path",
  "kind": "function",
  "symbol": "function_name",
  "replacement": "FULL REPLACEMENT FUNCTION CODE",
  "explanation": "short explanation"
}

READ RULES:
- Before modifying an existing file, read_file that exact file.
- If a file is already listed in READ FILE CONTENTS, do not read_file it again.
- Never read the same file more than once.
- After reading a file, the next action should normally be apply_patch, apply_unified_diff, create_file, run_command, or done.
- Do not loop on read_file.

PATCH RULES:
- For existing files, always use apply_patch or apply_unified_diff.
- find must not be empty.
- replace must not be empty.
- find must be copied exactly from READ FILE CONTENTS.
- find must be unique in the file.
- replace must include the original find text plus the inserted or modified code when inserting.
- Do not replace an entire existing file unless explicitly asked.
- If replacing a whole existing function, prefer apply_ast_patch.
- apply_ast_patch requires file_path, kind, symbol, replacement, and explanation.
- replacement must contain the full replacement function including signature and body.

MULTI-FILE RULES:
- If the task requires multiple files, complete all required files.
- Create all requested new files with create_file.
- Modify all requested existing files with apply_patch or apply_unified_diff.
- After all requested files are queued, return done.
- Do not keep reading files after creating or patching the requested files.
- For a task like creating math.h, math.cpp and updating main.cpp:
  1. create_file math.h
  2. create_file math.cpp
  3. read_file main.cpp
  4. apply_patch main.cpp
  5. done

DONE RULES:
- Use done only when the requested work is complete or impossible.
- If all file changes were queued, return:
{
  "action": "done",
  "explanation": "Completed requested changes."
}

MODE RULES:
- ask: answer only, do not edit files.
- edit: implement the requested change.
- fix: find likely bugs and produce minimal fixes.
- refactor: improve structure while preserving behavior.
- test: add or improve tests.
- auto: infer intent.
- In auto mode, if the user asks to add, create, implement, change, update, remove, rename, fix, refactor, or test code, produce code changes.
- In auto mode, if the user asks only for explanation, summary, or analysis, do not edit files.

BEHAVIOR RULES:
- Do not guess file paths.
- Use active file if it is relevant.
- Preserve existing style.
- Keep changes minimal.
- If a patch fails, inspect/read/search again once, then choose a safer action.
- Do not invent unrelated code.
- Do not directly write files. Always create pending changes through tool actions.
USER REQUEST:
)";

prompt << "\nMODE:\n" << request.mode << "\n\n";

    prompt << request.query << "\n\n";
    prompt << "PROJECT INFO:\n";
    if (!request.project_summary.empty()) {
    prompt << request.project_summary << "\n";

    if (!request.session_context.empty()) {
    prompt << request.session_context << "\n";
}
}
prompt << "type: " << request.project_type << "\n";

if (!request.build_command.empty()) {
    prompt << "build_command: " << request.build_command << "\n";
}

if (!request.test_command.empty()) {
    prompt << "test_command: " << request.test_command << "\n";
}

prompt << "\n";
    if (!request.active_file.empty()) {
    prompt << "ACTIVE FILE:\n" << request.active_file << "\n\n";
}

if (!request.selected_text.empty()) {
    prompt << "SELECTED TEXT FROM ACTIVE FILE:\n";
    prompt << request.selected_text.substr(0, 12000) << "\n\n";
}
    if (!plan.empty()) {
    prompt << "PLAN:\n" << plan << "\n\n";
}
    prompt << "CONTEXT:\n" << context << "\n\n";

prompt << "VISITED FILES:\n";
for (const auto& file : memory.visited_files) {
    prompt << "- " << file << "\n";
}

prompt << "\nREAD FILE CONTENTS:\n";

for (const auto& item : memory.file_contents) {
    prompt << "\nFILE: " << item.first << "\n";
    prompt << item.second.substr(0, 6000) << "\n";
}

prompt << "\nEDITED FILES:\n";
for (const auto& file : memory.edited_files) {
    prompt << "- " << file << "\n";
}

prompt << "\nMEMORY:\n";
for (const auto& obs : memory.observations) {
    prompt << "- " << obs.substr(0, 1000) << "\n";
}

    if (!request.extra_context.empty()) {
    prompt << "EXTRA CONTEXT:\n";
    prompt << request.extra_context.substr(0, 16000) << "\n\n";
}

    return prompt.str();
}

AgentAction Executor::next_action(
    const AgentRequest& request,
    const std::string& plan,
    const std::string& context,
    const AgentMemory& memory
) {
    std::string prompt =
        build_executor_prompt(
            request,
            plan,
            context,
            memory
        );

    std::string raw = llm_->generate(prompt);

    return parse_action(raw, request);
}

static std::string extract_json_object(const std::string& text)
{
    size_t start = text.find('{');
    size_t end = text.rfind('}');

    if (start == std::string::npos ||
        end == std::string::npos ||
        end <= start)
    {
        return text;
    }

    return text.substr(start, end - start + 1);
}

static std::string extract_xml_attr(
    const std::string& raw,
    const std::string& attr
) {
    std::string pattern = attr + "=\"";
    size_t pos = raw.find(pattern);

    if (pos == std::string::npos) {
        return "";
    }

    pos += pattern.size();

    size_t end = raw.find("\"", pos);

    if (end == std::string::npos) {
        return "";
    }

    return raw.substr(pos, end - pos);
}

AgentAction Executor::parse_action(
    const std::string& raw,
    const AgentRequest& request
) {
    AgentAction action;

    std::string json = extract_json_object(raw);

    std::string type = extract_value(json, "action");

    if (type.empty() &&
    json.find("\"file\"") != std::string::npos &&
    json.find("\"changes\"") != std::string::npos) {

    action.type = ActionType::Error;
    action.explanation =
        "Model returned an unsupported custom changes schema. "
        "Expected an action JSON such as edit_file or apply_patch.";
    return action;
}

    if (type.empty()) {
    if (raw.find("<read_file") != std::string::npos) {
        action.type = ActionType::ReadFile;
        action.file_path = extract_xml_attr(raw, "file_path");
        return action;
    }

    if (raw.find("<search_code") != std::string::npos) {
        action.type = ActionType::SearchCode;
        action.query = extract_xml_attr(raw, "query");
        return action;
    }

    if (raw.find("<search_symbol") != std::string::npos) {
        action.type = ActionType::SearchSymbol;
        action.query = extract_xml_attr(raw, "query");
        return action;
    }
}

    if (type == "search_code" || type == "search_file") {
        action.type = ActionType::SearchCode;
        action.query = extract_value(raw, "query");
    }

    else if (type == "search_symbol") {
    action.type = ActionType::SearchSymbol;
    action.query = extract_value(raw, "query");
}

else if (type == "search_references") {
    action.type = ActionType::SearchReferences;
    action.query = extract_value(raw, "query");
}

else if (type == "search_calls") {
    action.type = ActionType::SearchCalls;
    action.query = extract_value(raw, "query");
}

else if (type == "analyze_impact") {
    action.type = ActionType::AnalyzeImpact;
    action.query = extract_value(raw, "query");
}

else if (type == "semantic_search") {
    action.type = ActionType::SemanticSearch;
    action.query = extract_value(raw, "query");
}

    else if (type == "read_file") {
        action.type = ActionType::ReadFile;
        action.file_path = extract_value(raw, "file_path");
    }

    else if (type == "create_file") {
    action.type = ActionType::CreateFile;

    action.file_path = extract_value(raw, "file_path");
    if (action.file_path.empty()) {
        action.file_path = extract_value(raw, "file");
    }
    if (action.file_path.empty()) {
        action.file_path = extract_value(raw, "path");
    }

    action.content = extract_value(raw, "content");
    if (action.content.empty()) {
        action.content = extract_value(raw, "new_content");
    }
    if (action.content.empty()) {
        action.content = extract_value(raw, "text");
    }

    action.explanation = extract_value(raw, "explanation");
    if (action.explanation.empty()) {
        action.explanation = "Create new file";
    }
}

    else if (type == "edit_file") {
    action.type = ActionType::EditFile;

    action.file_path = extract_value(raw, "file_path");
    if (action.file_path.empty()) {
        action.file_path = extract_value(raw, "file");
    }
    if (action.file_path.empty()) {
        action.file_path = extract_value(raw, "path");
    }

    action.content = extract_value(raw, "content");
    if (action.content.empty()) {
        action.content = extract_value(raw, "new_content");
    }
    if (action.content.empty()) {
        action.content = extract_value(raw, "text");
    }

    action.explanation = extract_value(raw, "explanation");
    if (action.explanation.empty()) {
        action.explanation = extract_value(raw, "summary");
    }
}

    else if (type == "apply_patch") {
    action.type = ActionType::ApplyPatch;

    action.file_path = extract_value(raw, "file_path");
    if (action.file_path.empty()) action.file_path = extract_value(raw, "file");
    if (action.file_path.empty()) action.file_path = extract_value(raw, "path");

    action.find_text = extract_value(raw, "find");
    if (action.find_text.empty()) action.find_text = extract_value(raw, "old");
    if (action.find_text.empty()) action.find_text = extract_value(raw, "old_text");
    if (action.find_text.empty()) action.find_text = extract_value(raw, "before");
    if (action.find_text.empty()) action.find_text = extract_value(raw, "search");

    action.replace_text = extract_value(raw, "replace");
    if (action.replace_text.empty()) action.replace_text = extract_value(raw, "new");
    if (action.replace_text.empty()) action.replace_text = extract_value(raw, "new_text");
    if (action.replace_text.empty()) action.replace_text = extract_value(raw, "after");
    if (action.replace_text.empty()) action.replace_text = extract_value(raw, "replacement");

    action.explanation = extract_value(raw, "explanation");
    if (action.explanation.empty()) action.explanation = extract_value(raw, "summary");
}

else if (type == "apply_unified_diff") {
    action.type = ActionType::ApplyUnifiedDiff;
    action.file_path = extract_value(raw, "file");
    action.unified_diff = extract_value(raw, "diff");
    action.explanation = extract_value(raw, "explanation");
}

else if (type == "apply_ast_patch") {
    action.type = ActionType::ApplyAstPatch;

    action.file_path = extract_value(raw, "file_path");
    if (action.file_path.empty()) action.file_path = extract_value(raw, "file");

    action.symbol = extract_value(raw, "symbol");
    if (action.symbol.empty()) action.symbol = extract_value(raw, "name");

    action.kind = extract_value(raw, "kind");
    if (action.kind.empty()) action.kind = "function";

    action.replacement = extract_value(raw, "replacement");
    if (action.replacement.empty()) action.replacement = extract_value(raw, "content");

    action.explanation = extract_value(raw, "explanation");
}

else if (type == "run_command") {
    action.type = ActionType::RunCommand;
    action.command = extract_value(raw, "command");
    action.explanation = extract_value(raw, "explanation");
}

    else if (type == "done") {
        action.type = ActionType::Done;
        action.explanation = extract_value(raw, "explanation");
    }

    else {
        const bool has_json_action =
        raw.find('{') != std::string::npos &&
        raw.find("\"action\"") != std::string::npos;

    if (!has_json_action && request.mode == "ask") {
        action.type = ActionType::Done;
        action.explanation = raw;
    } else {
        action.type = ActionType::Error;
        action.explanation =
            "Expected JSON tool action, but model returned invalid output.\nRAW:\n" +
            raw.substr(0, 3000);
    }
    }

    return action;
}

AgentAction Executor::next_action_stream(
    const AgentRequest& request,
    const std::string& plan,
    const std::string& context,
    const AgentMemory& memory,
    std::function<void(const std::string&)> on_chunk
) {
    std::string prompt = build_executor_prompt(
        request,
        plan,
        context,
        memory
    );

    std::string raw;

    llm_->generate_stream(
        prompt,
        [&](const std::string& chunk) {
            raw += chunk;

            if (on_chunk) {
                on_chunk(chunk);
            }
        }
    );

    std::string json = extract_json_object(raw);

    AgentAction parsed = parse_action(json, request);

    if (parsed.type == ActionType::Error && request.mode == "ask") {
        parsed.type = ActionType::Done;
        parsed.explanation = raw;
    }

    return parsed;
}