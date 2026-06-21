#include "agent.h"
#include "planner.h"
#include "executor.h"

#include "../codebase/retriever.h"
#include "../editing/file_editor.h"
#include "../editing/change_set.h"
#include "../llm/openai_compatible_client.h"
#include "../tools/command_runner.h"
#include "../util/path_utils.h"
#include "../editing/patch_engine.h"
#include "../editing/unified_diff.h"
#include "../ast/ast_patch_engine.h"

Agent::Agent(ChangeSet* change_set, CodebaseIndex* codebase_index)
    : change_set_(change_set),
      codebase_index_(codebase_index) {}

struct PendingChangeRequest {
    std::string file_path;
    std::string old_content;
    std::string new_content;
    std::string explanation;
    std::string unified_diff;
};

static bool is_file_queued(
    const std::vector<PendingChangeRequest>& queued_changes,
    const std::string& file_path
) {
    for (const auto& queued : queued_changes) {
        if (queued.file_path == file_path) {
            return true;
        }
    }

    return false;
}

AgentResponse Agent::run(const AgentRequest& request) {
    AgentResponse response;
    std::vector<PendingChangeRequest> queued_changes;
    AgentMemory memory;

    OpenAICompatibleClient llm;
    Planner planner(&llm);
    Executor executor(&llm);
    Retriever retriever(codebase_index_);
    FileEditor editor;
    CommandRunner command_runner;
    PatchEngine patch_engine;

    AstPatchEngine ast_patch_engine;

    std::string plan;

if (request.mode == "ask" || request.mode == "auto") {
    plan = planner.create_plan(request);

    memory.remember_observation(
        "PLAN:\n" + plan
    );
}

    for (int i = 0; i < max_steps_; i++) {
        std::string context = retriever.retrieve(
            request.query,
            request.active_file,
            memory
        );

        AgentAction action = executor.next_action(
            request,
            plan,
            context,
            memory
        );

        AgentStepResult step;
        step.action = action;

                if (action.type == ActionType::SearchCode) {
            std::string results = retriever.search(action.query);
            memory.remember_observation(results);

            step.success = true;
            step.output = results;
        }

        else if (action.type == ActionType::SearchSymbol) {
            std::string results = retriever.search_symbols(action.query);
            memory.remember_observation(results);

            step.success = true;
            step.output = results;
        }

        else if (action.type == ActionType::SearchReferences) {
    std::string results = retriever.search_references(action.query);
    memory.remember_observation(results);

    step.success = true;
    step.output = results;
}

else if (action.type == ActionType::SearchCalls) {
    std::string results = retriever.search_calls(action.query);
    memory.remember_observation(results);

    step.success = true;
    step.output = results;
}

else if (action.type == ActionType::AnalyzeImpact) {
    std::string results = retriever.analyze_impact(action.query);
    memory.remember_observation(results);

    step.success = true;
    step.output = results;
}

else if (action.type == ActionType::SemanticSearch) {
    std::string results = retriever.semantic_search(action.query);
    memory.remember_observation(results);

    step.success = true;
    step.output = results;
}

        else if (action.type == ActionType::ReadFile) {
    action.file_path = normalize_path(action.file_path);

    if (memory.has_visited_file(action.file_path)) {
        memory.remember_observation(
            "SYSTEM: File already read. Do not call read_file again for: " +
            action.file_path +
            ". Next action must be apply_patch, apply_unified_diff, edit_file, or done."
        );

        step.success = true;
        step.output = "File already read. Content is available in READ FILE CONTENTS.";
        response.steps.push_back(step);
        continue;
    }

    std::string content = editor.read_file(action.file_path);

    if (content.empty() && !editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: File does not exist: " +
        action.file_path +
        ". Do not use apply_patch for this file. Use create_file instead."
    );

    step.success = false;
    step.output = "File does not exist. Use create_file instead.";
    response.steps.push_back(step);
    continue;
}

    memory.remember_file(action.file_path);
    memory.remember_observation(content);

    memory.remember_file_content(action.file_path, content);

    step.success = true;
    step.output = content;
}

else if (action.type == ActionType::CreateFile) {
    action.file_path = normalize_path(action.file_path);

    if (action.file_path.empty() || action.content.empty()) {
        step.success = false;
        step.output = "Blocked create_file because file_path or content is empty.";
        response.steps.push_back(step);
        continue;
    }

    if (editor.file_exists(action.file_path)) {
    std::string old_content = editor.read_file(action.file_path);

    PendingChangeRequest queued;
    queued.file_path = action.file_path;
    queued.old_content = old_content;
    queued.new_content = action.content;
    queued.explanation = action.explanation.empty()
        ? "Replace existing file content"
        : action.explanation;
    queued.unified_diff = "";

    if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

    queued_changes.push_back(queued);

    step.success = true;
    step.output =
        "Queued replacement for existing file: " + action.file_path;

    memory.remember_edit(action.file_path);
    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, action.content);
    memory.remember_observation(
        "Queued replacement for existing file: " + action.file_path
    );

    response.steps.push_back(step);
    continue;
}

    bool already_queued = false;

for (const auto& queued : queued_changes) {
    if (queued.file_path == action.file_path) {
        already_queued = true;
        break;
    }
}

if (already_queued) {
    step.success = true;
    step.output = "File already queued: " + action.file_path;
    response.steps.push_back(step);
    continue;
}

    PendingChangeRequest queued;
    queued.file_path = action.file_path;
    queued.old_content = "";
    queued.new_content = action.content;
    queued.explanation = action.explanation;
    queued.unified_diff = "";

    if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

    queued_changes.push_back(queued);

    step.success = true;
    step.output = "Queued new file for: " + action.file_path;

    memory.remember_edit(action.file_path);
    memory.remember_observation("Queued new file for: " + action.file_path);
    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, action.content);

    response.steps.push_back(step);
    continue;
}

    else if (action.type == ActionType::EditFile) {
    action.file_path = normalize_path(action.file_path);

    bool file_exists = editor.file_exists(action.file_path);

if (file_exists) {
    memory.remember_observation(
        "SYSTEM: edit_file was rejected because the file already exists. "
        "Existing files must be modified using apply_patch only."
    );

    step.success = false;
    step.output = "edit_file rejected. Existing files require apply_patch.";
    response.steps.push_back(step);
    continue;
}

    if (file_exists && !memory.has_visited_file(action.file_path)) {
    std::string current_content = editor.read_file(action.file_path);

    if (current_content.empty()) {
        memory.remember_observation(
            "SYSTEM: Cannot edit_file because file could not be read: " +
            action.file_path
        );

        step.success = false;
        step.output = "Blocked edit_file because file could not be read.";
        response.steps.push_back(step);
        continue;
    }

    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, current_content);
    memory.remember_observation(
        "SYSTEM: File was auto-read before edit_file: " +
        action.file_path
    );
}

    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        response.steps.push_back(step);
        return response;
    }

    std::string old_content = file_exists
        ? editor.read_file(action.file_path)
        : "";

    PendingChangeRequest queued;
queued.file_path = action.file_path;
queued.old_content = old_content;
queued.new_content = action.content;
queued.explanation = action.explanation;
queued.unified_diff = "";

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

queued_changes.push_back(queued);

step.success = true;
step.output = "Queued pending change for: " + action.file_path;

memory.remember_edit(action.file_path);
memory.remember_observation("Queued pending change for: " + action.file_path);

response.steps.push_back(step);
continue;
}

        else if (action.type == ActionType::ApplyPatch) {
            action.file_path = normalize_path(action.file_path);

            if (!memory.has_visited_file(action.file_path)) {
                memory.remember_observation(
                    "SYSTEM: Cannot apply_patch before reading file. Must read_file first: " +
                    action.file_path
                );

                step.success = false;
                step.output = "Blocked apply_patch because file was not read first.";
                response.steps.push_back(step);
                continue;
            }

            if (!change_set_) {
                response.success = false;
                response.final_message = "ChangeSet is not available.";
                response.steps.push_back(step);
                return response;
            }

            if (!editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: Cannot apply_patch because file does not exist: " +
        action.file_path +
        ". Use create_file with full file content instead."
    );

    step.success = false;
    step.output = "File does not exist. Use create_file instead.";
    response.steps.push_back(step);
    continue;
}

            std::string old_content = editor.read_file(action.file_path);

            PatchResult patch = patch_engine.apply_find_replace(
                old_content,
                action.find_text,
                action.replace_text
            );

            if (!patch.success) {
                memory.remember_observation(
                    "SYSTEM: apply_patch failed for " +
                    action.file_path +
                    ": " +
                    patch.error
                );

                step.success = false;
                step.output = "Patch failed: " + patch.error;
                response.steps.push_back(step);
                continue;
            }

            PendingChangeRequest queued;
queued.file_path = action.file_path;
queued.old_content = old_content;
queued.new_content = patch.new_content;
queued.explanation = action.explanation;
queued.unified_diff = patch.unified_diff;

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

queued_changes.push_back(queued);

step.success = true;
step.output = "Queued pending patch for: " + action.file_path;

memory.remember_edit(action.file_path);
memory.remember_observation("Queued pending patch for: " + action.file_path);

response.steps.push_back(step);
continue;
        }

        else if (action.type == ActionType::ApplyUnifiedDiff) {
    action.file_path = normalize_path(action.file_path);

    if (!memory.has_visited_file(action.file_path)) {
        memory.remember_observation(
            "SYSTEM: Cannot apply_unified_diff before reading file. Must read_file first: " +
            action.file_path
        );

        step.success = false;
        step.output = "Blocked apply_unified_diff because file was not read first.";
        response.steps.push_back(step);
        continue;
    }

    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        response.steps.push_back(step);
        return response;
    }

    if (!editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: Cannot apply_patch because file does not exist: " +
        action.file_path +
        ". Use create_file with full file content instead."
    );

    step.success = false;
    step.output = "File does not exist. Use create_file instead.";
    response.steps.push_back(step);
    continue;
}

    std::string old_content = editor.read_file(action.file_path);

    UnifiedDiffParser parser;
    UnifiedDiffApplier applier;

    UnifiedDiff diff = parser.parse(action.unified_diff);

    UnifiedPatchResult patch = applier.apply(
        old_content,
        diff
    );

    if (!patch.success) {
        memory.remember_observation(
            "SYSTEM: apply_unified_diff failed for " +
            action.file_path +
            ": " +
            patch.error
        );

        step.success = false;
        step.output = "Unified diff failed: " + patch.error;
        response.steps.push_back(step);
        continue;
    }

    PendingChangeRequest queued;
queued.file_path = action.file_path;
queued.old_content = old_content;
queued.new_content = patch.new_content;
queued.explanation = action.explanation;
queued.unified_diff = action.unified_diff;

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

queued_changes.push_back(queued);

step.success = true;
step.output = "Queued pending unified diff for: " + action.file_path;

memory.remember_edit(action.file_path);
memory.remember_observation("Queued pending unified diff for: " + action.file_path);

response.steps.push_back(step);
continue;
}

else if (action.type == ActionType::ApplyAstPatch) {
    action.file_path = normalize_path(action.file_path);

    if (action.file_path.empty() ||
        action.symbol.empty() ||
        action.replacement.empty()) {
        memory.remember_observation(
            "SYSTEM: apply_ast_patch was rejected because file_path, symbol, or replacement is empty."
        );

        step.success = false;
        step.output = "Blocked apply_ast_patch because file_path, symbol, or replacement is empty.";
        response.steps.push_back(step);
        continue;
    }

    if (!editor.file_exists(action.file_path)) {
        memory.remember_observation(
            "SYSTEM: Cannot apply_ast_patch because file does not exist: " +
            action.file_path
        );

        step.success = false;
        step.output = "File does not exist.";
        response.steps.push_back(step);
        continue;
    }

    std::string old_content = editor.read_file(action.file_path);

    AstPatchResult patch = ast_patch_engine.replace_function(
        action.file_path,
        old_content,
        action.symbol,
        action.replacement
    );

    if (!patch.success) {
        memory.remember_observation(
            "SYSTEM: apply_ast_patch failed for " +
            action.file_path +
            ": " +
            patch.error +
            ". Retry with apply_patch or apply_unified_diff."
        );

        step.success = false;
        step.output = "AST patch failed: " + patch.error;
        response.steps.push_back(step);
        continue;
    }

    PendingChangeRequest queued;
    queued.file_path = action.file_path;
    queued.old_content = old_content;
    queued.new_content = patch.new_content;
    queued.explanation = action.explanation;
    queued.unified_diff = patch.unified_diff;

    bool already_queued = false;

for (const auto& existing : queued_changes) {
    if (existing.file_path == action.file_path) {
        already_queued = true;
        break;
    }
}

if (already_queued) {
    step.success = true;
    step.output = "AST patch already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: AST patch already queued for " +
        action.file_path +
        ". Do not patch it again. Return done if no other files are needed."
    );

    response.steps.push_back(step);
    continue;
}

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

    queued_changes.push_back(queued);

    step.success = true;
    step.output = "Queued AST patch for: " + action.file_path + " symbol: " + action.symbol;

    memory.remember_edit(action.file_path);
    memory.remember_observation("Queued AST patch for: " + action.file_path);

    response.steps.push_back(step);
    continue;
}

        else if (action.type == ActionType::RunCommand) {
            std::string output = command_runner.run(
                action.command,
                request.workspace_path
            );

            memory.remember_observation("COMMAND: " + action.command + "\n" + output);

            step.success = true;
            step.output = output;
        }

        else if (action.type == ActionType::Done) {
    step.success = true;
    step.output = action.explanation;

    response.steps.push_back(step);

    if (!queued_changes.empty()) {
        if (!change_set_) {
            response.success = false;
            response.final_message = "ChangeSet is not available.";
            return response;
        }

        std::vector<PendingChangeFile> files;

for (const auto& queued : queued_changes) {
    PendingChangeFile file;
    file.file_path = queued.file_path;
    file.old_content = queued.old_content;
    file.new_content = queued.new_content;
    file.unified_diff = queued.unified_diff;

    files.push_back(file);
}

std::string change_id = change_set_->create_multi(
    files,
    action.explanation
);

response.pending_change_ids.push_back(change_id);
    }

    response.success = true;
    response.final_message = action.explanation;

    if (!queued_changes.empty()) {
        response.final_message +=
            " Created " +
            std::to_string(queued_changes.size()) +
            " pending change(s).";
    }

    return response;
}

        else {
            response.success = false;
            response.final_message = "Executor returned invalid action.";
            response.steps.push_back(step);
            return response;
        }

        response.steps.push_back(step);
    }

    if (!queued_changes.empty()) {
    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        return response;
    }

    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;

        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Partial changes created before max steps."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Stopped after max steps, but created partial pending change: " + change_id;

    return response;
}

response.success = false;
response.final_message = "Stopped after max steps.";
return response;
}

AgentResponse Agent::run_stream(
    const AgentRequest& request,
    std::function<void(const std::string&)> on_chunk
) {
    AgentResponse response;
    std::vector<PendingChangeRequest> queued_changes;
    int repeated_read_count = 0;
    AgentMemory memory;

    OpenAICompatibleClient llm;
    Planner planner(&llm);
    Executor executor(&llm);
    Retriever retriever(codebase_index_);
    FileEditor editor;
    CommandRunner command_runner;
    PatchEngine patch_engine;

    AstPatchEngine ast_patch_engine;

    std::string plan;

    if (request.mode == "ask" || request.mode == "auto") {
        plan = planner.create_plan(request);

        memory.remember_observation(
            "PLAN:\n" + plan
        );
    }

    for (int i = 0; i < max_steps_; i++) {

        std::string context = retriever.retrieve(
            request.query,
            request.active_file,
            memory
        );

        AgentAction action =
            executor.next_action_stream(
                request,
                plan,
                context,
                memory,
                on_chunk
            );

        AgentStepResult step;
        step.action = action;

        if (action.type == ActionType::ReadFile) {
    action.file_path = normalize_path(action.file_path);

    if (memory.has_visited_file(action.file_path)) {
    repeated_read_count++;

    memory.remember_observation(
    "CRITICAL SYSTEM ERROR: You repeated read_file for " +
    action.file_path +
    ". The file content is already available in READ FILE CONTENTS. "
    "Your NEXT action MUST NOT be read_file. "
    "Your NEXT action MUST be one of: create_file, apply_patch, apply_unified_diff, or done. "
    "For this task, continue by creating math.h and math.cpp, then patch main.cpp."
);

    step.success = false;
    step.output =
        "Blocked repeated read_file. File is already available in READ FILE CONTENTS.";

    response.steps.push_back(step);

    if (!queued_changes.empty() && repeated_read_count >= 3) {
        std::vector<PendingChangeFile> files;

        for (const auto& queued : queued_changes) {
            PendingChangeFile file;
            file.file_path = queued.file_path;
            file.old_content = queued.old_content;
            file.new_content = queued.new_content;
            file.unified_diff = queued.unified_diff;
            files.push_back(file);
        }

        std::string change_id = change_set_->create_multi(
            files,
            "Created partial change because agent repeated read_file."
        );

        response.pending_change_ids.push_back(change_id);
        response.success = true;
        response.final_message =
            "Created partial pending change after repeated read_file: " + change_id;

        return response;
    }

    continue;
}

    std::string content = editor.read_file(action.file_path);

    if (content.empty() && !editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: File does not exist: " +
        action.file_path +
        ". Do not use apply_patch for this file. Use create_file instead."
    );

    step.success = false;
    step.output = "File does not exist. Use create_file instead.";
    response.steps.push_back(step);
    continue;
}

    memory.remember_file(action.file_path);
    memory.remember_observation(content);
    memory.remember_file_content(action.file_path, content);

    step.success = true;
    step.output = content;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::SearchCode) {
    std::string results = retriever.search(action.query);

    memory.remember_observation(results);

    step.success = true;
    step.output = results;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::SearchSymbol) {
    std::string results = retriever.search_symbols(action.query);

    memory.remember_observation(results);

    step.success = true;
    step.output = results;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::SearchReferences) {
    std::string results = retriever.search_references(action.query);

    memory.remember_observation(results);

    step.success = true;
    step.output = results;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::SearchCalls) {
    std::string results = retriever.search_calls(action.query);

    memory.remember_observation(results);

    step.success = true;
    step.output = results;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::AnalyzeImpact) {
    std::string results = retriever.analyze_impact(action.query);

    memory.remember_observation(results);

    step.success = true;
    step.output = results;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::SemanticSearch) {
    std::string results = retriever.semantic_search(action.query);

    memory.remember_observation(results);

    step.success = true;
    step.output = results;

    response.steps.push_back(step);
    continue;
}

if (action.type == ActionType::CreateFile) {
    action.file_path = normalize_path(action.file_path);

    if (action.file_path.empty() || action.content.empty()) {
        step.success = false;
        step.output = "Blocked create_file because file_path or content is empty.";
        response.steps.push_back(step);
        continue;
    }

    if (editor.file_exists(action.file_path)) {
    std::string old_content = editor.read_file(action.file_path);

    PendingChangeRequest queued;
    queued.file_path = action.file_path;
    queued.old_content = old_content;
    queued.new_content = action.content;
    queued.explanation = action.explanation.empty()
        ? "Replace existing file content"
        : action.explanation;
    queued.unified_diff = "";

    if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

    queued_changes.push_back(queued);
    repeated_read_count = 0;

    step.success = true;
    step.output =
        "Queued replacement for existing file: " + action.file_path;

    memory.remember_edit(action.file_path);
    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, action.content);
    memory.remember_observation(
        "Queued replacement for existing file: " + action.file_path
    );

    response.steps.push_back(step);

if (queued_changes.size() >= 3) {
    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;
        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Created multi-file pending change."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Created multi-file pending change: " + change_id;

    return response;
}

    continue;
}

    bool already_queued = false;

for (const auto& queued : queued_changes) {
    if (queued.file_path == action.file_path) {
        already_queued = true;
        break;
    }
}

if (already_queued) {
    step.success = true;
    step.output = "File already queued: " + action.file_path;
    response.steps.push_back(step);
    continue;
}

    PendingChangeRequest queued;
    queued.file_path = action.file_path;
    queued.old_content = "";
    queued.new_content = action.content;
    queued.explanation = action.explanation;
    queued.unified_diff = "";

    queued_changes.push_back(queued);
    repeated_read_count = 0;

    step.success = true;
    step.output = "Queued new file for: " + action.file_path;

    memory.remember_edit(action.file_path);
    memory.remember_observation("Queued new file for: " + action.file_path);
    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, action.content);

    response.steps.push_back(step);

if (queued_changes.size() >= 3) {
    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;
        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Created multi-file pending change."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Created multi-file pending change: " + change_id;

    return response;
}

    continue;
}

if (action.type == ActionType::EditFile) {
    action.file_path = normalize_path(action.file_path);

    if (!action.file_path.empty() && editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: edit_file was rejected because the file already exists. "
        "Existing files must be modified using apply_patch only. "
        "Use apply_patch with exact find and replace text copied from READ FILE CONTENTS."
    );

    step.success = false;
    step.output =
        "edit_file rejected. Existing files require apply_patch.";

    response.steps.push_back(step);
    continue;
}

    if (action.file_path.empty() || action.content.empty()) {
    memory.remember_observation(
        "SYSTEM: edit_file was rejected because file_path or content is empty. "
        "For existing files, do NOT use edit_file. "
        "Use apply_patch with file_path, exact find, replace, and explanation. "
        "The find text must be copied exactly from READ FILE CONTENTS."
    );

    step.success = false;
    step.output =
        "Blocked edit_file because file_path or content is empty. "
        "Retry with apply_patch.";

    response.steps.push_back(step);
    continue;
}

    bool file_exists = editor.file_exists(action.file_path);

    if (file_exists && !memory.has_visited_file(action.file_path)) {
    std::string current_content = editor.read_file(action.file_path);

    if (current_content.empty()) {
        memory.remember_observation(
            "SYSTEM: Cannot edit_file because file could not be read: " +
            action.file_path
        );

        step.success = false;
        step.output = "Blocked edit_file because file could not be read.";
        response.steps.push_back(step);
        continue;
    }

    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, current_content);
    memory.remember_observation(
        "SYSTEM: File was auto-read before edit_file: " +
        action.file_path
    );
}

    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        response.steps.push_back(step);
        return response;
    }

    std::string old_content = file_exists
        ? editor.read_file(action.file_path)
        : "";

    PendingChangeRequest queued;
queued.file_path = action.file_path;
queued.old_content = old_content;
queued.new_content = action.content;
queued.explanation = action.explanation;
queued.unified_diff = "";

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

queued_changes.push_back(queued);
repeated_read_count = 0;

step.success = true;
step.output = "Queued pending change for: " + action.file_path;

memory.remember_edit(action.file_path);
memory.remember_observation("Queued pending change for: " + action.file_path);

response.steps.push_back(step);

if (queued_changes.size() >= 3) {
    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;
        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Created multi-file pending change."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Created multi-file pending change: " + change_id;

    return response;
}

continue;
}

if (action.type == ActionType::ApplyPatch) {
    action.file_path = normalize_path(action.file_path);

    if (action.file_path.empty() ||
    action.find_text.empty() ||
    action.replace_text.empty()) {
    memory.remember_observation(
        "SYSTEM: apply_patch was rejected because file_path, find, or replace is empty. "
        "You MUST retry with this exact schema: "
        "{\"action\":\"apply_patch\",\"file_path\":\"...\",\"find\":\"exact text from READ FILE CONTENTS\",\"replace\":\"new text\",\"explanation\":\"...\"}. "
        "Do not use edit_file."
    );

    step.success = false;
    step.output = "Blocked apply_patch because file_path, find, or replace is empty.";
    response.steps.push_back(step);
    continue;
}

    if (!memory.has_visited_file(action.file_path)) {
    std::string current_content = editor.read_file(action.file_path);

    if (current_content.empty()) {
        memory.remember_observation(
            "SYSTEM: Cannot apply_patch because file could not be read: " +
            action.file_path
        );

        step.success = false;
        step.output = "Blocked apply_patch because file could not be read.";
        response.steps.push_back(step);
        continue;
    }

    memory.remember_file(action.file_path);
    memory.remember_file_content(action.file_path, current_content);
    memory.remember_observation(
        "SYSTEM: File was auto-read before apply_patch: " +
        action.file_path
    );
}

    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        response.steps.push_back(step);
        return response;
    }

    if (!editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: Cannot apply_patch because file does not exist: " +
        action.file_path +
        ". Use create_file with full file content instead."
    );

    step.success = false;
    step.output = "File does not exist. Use create_file instead.";
    response.steps.push_back(step);
    continue;
}

    std::string old_content = editor.read_file(action.file_path);

    PatchResult patch = patch_engine.apply_find_replace(
        old_content,
        action.find_text,
        action.replace_text
    );

    if (!patch.success) {
        memory.remember_observation(
            "SYSTEM: apply_patch failed for " +
            action.file_path +
            ": " +
            patch.error
        );

        step.success = false;
        step.output = "Patch failed: " + patch.error;
        response.steps.push_back(step);
        continue;
    }

    PendingChangeRequest queued;
queued.file_path = action.file_path;
queued.old_content = old_content;
queued.new_content = patch.new_content;
queued.explanation = action.explanation;
queued.unified_diff = patch.unified_diff;

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

queued_changes.push_back(queued);
repeated_read_count = 0;

step.success = true;
step.output = "Queued pending patch for: " + action.file_path;

memory.remember_edit(action.file_path);
memory.remember_observation("Queued pending patch for: " + action.file_path);

response.steps.push_back(step);

if (queued_changes.size() >= 3) {
    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;
        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Created multi-file pending change."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Created multi-file pending change: " + change_id;

    return response;
}

continue;
}

if (action.type == ActionType::ApplyUnifiedDiff) {
    action.file_path = normalize_path(action.file_path);

    if (!memory.has_visited_file(action.file_path)) {
        memory.remember_observation(
            "SYSTEM: Cannot apply_unified_diff before reading file. Must read_file first: " +
            action.file_path
        );

        step.success = false;
        step.output = "Blocked apply_unified_diff because file was not read first.";
        response.steps.push_back(step);
        continue;
    }

    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        response.steps.push_back(step);
        return response;
    }

    if (!editor.file_exists(action.file_path)) {
    memory.remember_observation(
        "SYSTEM: Cannot apply_patch because file does not exist: " +
        action.file_path +
        ". Use create_file with full file content instead."
    );

    step.success = false;
    step.output = "File does not exist. Use create_file instead.";
    response.steps.push_back(step);
    continue;
}

    std::string old_content = editor.read_file(action.file_path);

    UnifiedDiffParser parser;
    UnifiedDiffApplier applier;

    UnifiedDiff diff = parser.parse(action.unified_diff);

    UnifiedPatchResult patch = applier.apply(
        old_content,
        diff
    );

    if (!patch.success) {
        memory.remember_observation(
            "SYSTEM: apply_unified_diff failed for " +
            action.file_path +
            ": " +
            patch.error
        );

        step.success = false;
        step.output = "Unified diff failed: " + patch.error;
        response.steps.push_back(step);
        continue;
    }

    PendingChangeRequest queued;
queued.file_path = action.file_path;
queued.old_content = old_content;
queued.new_content = patch.new_content;
queued.explanation = action.explanation;
queued.unified_diff = action.unified_diff;

if (is_file_queued(queued_changes, action.file_path)) {
    step.success = true;
    step.output = "Change already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: Change already queued for " +
        action.file_path +
        ". Do not modify this file again. Continue with other files or return done."
    );

    response.steps.push_back(step);
    continue;
}

queued_changes.push_back(queued);
repeated_read_count = 0;

step.success = true;
step.output = "Queued pending unified diff for: " + action.file_path;

memory.remember_edit(action.file_path);
memory.remember_observation("Queued pending unified diff for: " + action.file_path);

response.steps.push_back(step);

if (queued_changes.size() >= 3) {
    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;
        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Created multi-file pending change."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Created multi-file pending change: " + change_id;

    return response;
}

continue;
}

if (action.type == ActionType::ApplyAstPatch) {
    action.file_path = normalize_path(action.file_path);

    if (action.file_path.empty() ||
        action.symbol.empty() ||
        action.replacement.empty()) {
        memory.remember_observation(
            "SYSTEM: apply_ast_patch was rejected because file_path, symbol, or replacement is empty."
        );

        step.success = false;
        step.output = "Blocked apply_ast_patch because file_path, symbol, or replacement is empty.";
        response.steps.push_back(step);
        continue;
    }

    if (!editor.file_exists(action.file_path)) {
        memory.remember_observation(
            "SYSTEM: Cannot apply_ast_patch because file does not exist: " +
            action.file_path
        );

        step.success = false;
        step.output = "File does not exist.";
        response.steps.push_back(step);
        continue;
    }

    std::string old_content = editor.read_file(action.file_path);

    AstPatchResult patch = ast_patch_engine.replace_function(
        action.file_path,
        old_content,
        action.symbol,
        action.replacement
    );

    if (!patch.success) {
        memory.remember_observation(
            "SYSTEM: apply_ast_patch failed for " +
            action.file_path +
            ": " +
            patch.error +
            ". Retry with apply_patch or apply_unified_diff."
        );

        step.success = false;
        step.output = "AST patch failed: " + patch.error;
        response.steps.push_back(step);
        continue;
    }

    PendingChangeRequest queued;
    queued.file_path = action.file_path;
    queued.old_content = old_content;
    queued.new_content = patch.new_content;
    queued.explanation = action.explanation;
    queued.unified_diff = patch.unified_diff;

    bool already_queued = false;

for (const auto& existing : queued_changes) {
    if (existing.file_path == action.file_path) {
        already_queued = true;
        break;
    }
}

if (already_queued) {
    step.success = true;
    step.output = "AST patch already queued for: " + action.file_path;

    memory.remember_observation(
        "SYSTEM: AST patch already queued for " +
        action.file_path +
        ". Do not patch it again. Return done if no other files are needed."
    );

    response.steps.push_back(step);
    continue;
}

    queued_changes.push_back(queued);
    repeated_read_count = 0;

    step.success = true;
    step.output = "Queued AST patch for: " + action.file_path + " symbol: " + action.symbol;

    memory.remember_edit(action.file_path);
    memory.remember_observation("Queued AST patch for: " + action.file_path);

    response.steps.push_back(step);

    if (queued_changes.size() >= 3) {
        std::vector<PendingChangeFile> files;

        for (const auto& queued : queued_changes) {
            PendingChangeFile file;
            file.file_path = queued.file_path;
            file.old_content = queued.old_content;
            file.new_content = queued.new_content;
            file.unified_diff = queued.unified_diff;
            files.push_back(file);
        }

        std::string change_id = change_set_->create_multi(
            files,
            "Created multi-file pending change."
        );

        response.pending_change_ids.push_back(change_id);
        response.success = true;
        response.final_message =
            "Created multi-file pending change: " + change_id;

        return response;
    }

    continue;
}

if (action.type == ActionType::Done) {
    step.success = true;
    step.output = action.explanation;

    response.steps.push_back(step);

    if (!queued_changes.empty()) {
        if (!change_set_) {
            response.success = false;
            response.final_message = "ChangeSet is not available.";
            return response;
        }

        std::vector<PendingChangeFile> files;

for (const auto& queued : queued_changes) {
    PendingChangeFile file;
    file.file_path = queued.file_path;
    file.old_content = queued.old_content;
    file.new_content = queued.new_content;
    file.unified_diff = queued.unified_diff;

    files.push_back(file);
}

std::string change_id = change_set_->create_multi(
    files,
    action.explanation
);

response.pending_change_ids.push_back(change_id);
    }

    response.success = true;
    response.final_message = action.explanation;

    if (!queued_changes.empty()) {
        response.final_message +=
            " Created " +
            std::to_string(queued_changes.size()) +
            " pending change(s).";
    }

    return response;
}

if (action.type == ActionType::Error) {
    memory.remember_observation(
        "SYSTEM: Invalid tool JSON. You must return one of the supported actions. "
        "For edits, use edit_file with file_path/content/explanation or apply_patch with file_path/find/replace/explanation. "
        "Do not return custom schemas like file/changes/position."
    );

    step.success = false;
    step.output = action.explanation;

    response.steps.push_back(step);
    continue;
}

response.success = false;
response.final_message = "Unsupported streaming action.";

step.success = false;
step.output = "Streaming mode does not support this action yet.";

response.steps.push_back(step);

return response;
    }

    if (!queued_changes.empty()) {
    if (!change_set_) {
        response.success = false;
        response.final_message = "ChangeSet is not available.";
        return response;
    }

    std::vector<PendingChangeFile> files;

    for (const auto& queued : queued_changes) {
        PendingChangeFile file;
        file.file_path = queued.file_path;
        file.old_content = queued.old_content;
        file.new_content = queued.new_content;
        file.unified_diff = queued.unified_diff;

        files.push_back(file);
    }

    std::string change_id = change_set_->create_multi(
        files,
        "Partial changes created before max steps."
    );

    response.pending_change_ids.push_back(change_id);
    response.success = true;
    response.final_message =
        "Stopped after max steps, but created partial pending change: " + change_id;

    return response;
}

response.success = false;
response.final_message = "Stopped after max steps.";
return response;
}