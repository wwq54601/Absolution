#include "server.h"
#include "agent/agent.h"
#include "agent/action.h"

#include <iostream>
#include <sstream>
#include "editing/file_editor.h"
#include "util/path_utils.h"
#include "project/project_detector.h"
#include "tools/command_runner.h"
#include "codebase/project_summary.h"
#include "tools/error_parser.h"
#include "config/yucode_config.h"
#include "llm/openai_compatible_client.h"
#include "embeddings/embedding_provider.h"
#include "model_runtime/model_runtime_manager.h"
#include <filesystem>

YuCodeServer::YuCodeServer() {
    YuCodeConfig config = YuCodeConfig::load();

    if (config.embedded_runtime_enabled) {
        ModelRuntimeConfig runtime_config;
        runtime_config.enabled = true;
        runtime_config.server_path = config.embedded_server_path;
        runtime_config.model_path = config.embedded_model_path;
        runtime_config.port = config.embedded_runtime_port;
        runtime_config.context_size = config.embedded_context_size;
        runtime_config.gpu_layers = config.embedded_gpu_layers;

        ModelRuntimeManager runtime(runtime_config);

        if (runtime.ensure_running()) {
            config.llm_provider = "yucode-local";
            config.llm_base_url = runtime.base_url();

            YuCodeConfig::save(config);
        }
    }

    agent_ = std::make_unique<Agent>(&change_set_, nullptr);
    setup_routes();
}

YuCodeServer::~YuCodeServer() = default;

void YuCodeServer::setup_routes() {
    server_.Get("/api/status", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(
            R"({"status":"running","name":"YuCode Agent Runtime","version":"0.1.0"})",
            "application/json"
        );
    });

    server_.Post("/api/session/clear", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(clear_session_json(), "application/json");
});

    server_.Post("/api/agent", [this](const httplib::Request& req, httplib::Response& res) {
        std::string result = run_agent_json(req.body);
        res.set_content(result, "application/json");
    });

    server_.Post("/api/agent/stream", [this](const httplib::Request& req, httplib::Response& res) {
    std::string body = req.body;

    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");
    res.set_header("X-Accel-Buffering", "no");

    res.set_chunked_content_provider(
        "text/event-stream",
        [this, body](size_t, httplib::DataSink& sink) {
            run_agent_stream_sse(body, sink);
            sink.done();
            return true;
        }
    );
});

    server_.Get("/api/model/status", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(model_status_json(), "application/json");
});

    server_.Post("/api/index/file", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_content(update_index_file_json(req.body), "application/json");
});

server_.Post("/api/index/file/remove", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_content(remove_index_file_json(req.body), "application/json");
});

    server_.Post("/api/test-llm", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(test_llm_json(), "application/json");
});

server_.Post("/api/test-embedding", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(test_embedding_json(), "application/json");
});

server_.Get("/api/index/status", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(index_status_json(), "application/json");
});

server_.Get("/api/embedded-runtime/status", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(embedded_runtime_status_json(), "application/json");
});

    server_.Get("/api/config", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(get_config_json(), "application/json");
});

server_.Post("/api/config", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_content(update_config_json(req.body), "application/json");
});

    server_.Post("/api/index", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_content(reindex_json(req.body), "application/json");
});

    server_.Get("/api/changes", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content(list_changes_json(), "application/json");
});

server_.Post("/api/changes/apply", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_content(apply_change_json(req.body), "application/json");
});

server_.Post("/api/changes/reject", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_content(reject_change_json(req.body), "application/json");
});
}

void YuCodeServer::start(int port) {
    std::cout << "YuCode Agent Runtime listening on http://127.0.0.1:" << port << "\n";
    server_.listen("127.0.0.1", port);
}

static std::string json_value(const std::string& raw, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t key_pos = raw.find(pattern);
    if (key_pos == std::string::npos) return "";

    size_t colon = raw.find(":", key_pos);
    if (colon == std::string::npos) return "";

    size_t start = raw.find("\"", colon + 1);
    if (start == std::string::npos) return "";
    start++;

    std::string value;
    bool escaped = false;

    for (size_t i = start; i < raw.size(); i++) {
        char c = raw[i];

        if (escaped) {
            if (c == 'n') value += '\n';
            else if (c == 't') value += '\t';
            else if (c == 'r') value += '\r';
            else value += c;

            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') break;

        value += c;
    }

    return value;
}

static std::string escape_json(const std::string& text) {
    std::string out;

    for (char c : text) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }

    return out;
}

static std::string action_type_to_string(ActionType type) {
    switch (type) {
        case ActionType::SearchCode:
            return "search_code";
        case ActionType::SearchSymbol:
            return "search_symbol";
        case ActionType::SearchReferences:
            return "search_references";
        case ActionType::SearchCalls:
            return "search_calls";
        case ActionType::AnalyzeImpact:
            return "analyze_impact";
        case ActionType::SemanticSearch:
            return "semantic_search";
        case ActionType::ReadFile:
            return "read_file";
        case ActionType::EditFile:
            return "edit_file";
        case ActionType::CreateFile:
            return "create_file";
        case ActionType::ApplyPatch:
            return "apply_patch";
        case ActionType::ApplyUnifiedDiff:
            return "apply_unified_diff";
        case ActionType::ApplyAstPatch:
            return "apply_ast_patch";
        case ActionType::RunCommand:
            return "run_command";
        case ActionType::Done:
            return "done";
        default:
            return "error";
    }
}

static std::string pick_server_verify_command(
    const std::string& build_command,
    const std::string& test_command
) {
    if (!test_command.empty()) {
        return test_command;
    }

    if (!build_command.empty()) {
        return build_command;
    }

    return "";
}

std::string YuCodeServer::run_agent_json(const std::string& body) {
    AgentRequest request;
    request.query = json_value(body, "query");
    session_.add_user_message(request.query);
    request.session_context = session_.build_prompt_context();
    request.workspace_path = json_value(body, "workspace_path");
    request.active_file = json_value(body, "active_file");
    request.selected_text = json_value(body, "selected_text");
    request.extra_context = json_value(body, "extra_context");

    request.mode = json_value(body, "mode");

if (request.mode.empty()) {
    request.mode = "auto";
}

    if (request.query.empty()) {
        return R"({"success":false,"error":"query is required"})";
    }

    if (request.workspace_path.empty()) {
        request.workspace_path = ".";
    }

    ProjectDetector detector;
ProjectInfo project = detector.detect(request.workspace_path);

request.project_type = project.type;
request.build_command = project.build_command;
request.test_command = project.test_command;

ProjectSummaryBuilder summary_builder;
ProjectSummary summary = summary_builder.build(
    request.workspace_path,
    request.project_type,
    request.build_command,
    request.test_command
);

request.project_summary = summary.to_prompt();

current_workspace_path_ = request.workspace_path;
current_build_command_ = project.build_command;
current_test_command_ = project.test_command;

    if (!codebase_index_ || codebase_index_->workspace_path() != normalize_path(request.workspace_path)) {
    codebase_index_ = std::make_unique<CodebaseIndex>(request.workspace_path);
    codebase_index_->build();

    agent_ = std::make_unique<Agent>(&change_set_, codebase_index_.get());
}

    AgentResponse result = agent_->run(request);
    session_.add_assistant_message(result.final_message);

    std::stringstream ss;
    ss << "{";
    ss << "\"success\":" << (result.success ? "true" : "false") << ",";
    ss << "\"message\":\"" << escape_json(result.final_message) << "\",";
    ss << "\"pending_change_ids\":[";
for (size_t i = 0; i < result.pending_change_ids.size(); i++) {
    if (i > 0) ss << ",";
    ss << "\"" << escape_json(result.pending_change_ids[i]) << "\"";
}
ss << "],";
    ss << "\"steps\":[";

    for (size_t i = 0; i < result.steps.size(); i++) {
        const auto& step = result.steps[i];

        if (i > 0) ss << ",";

        ss << "{";
ss << "\"success\":" << (step.success ? "true" : "false") << ",";
ss << "\"action\":\"" << escape_json(action_type_to_string(step.action.type)) << "\",";
ss << "\"query\":\"" << escape_json(step.action.query) << "\",";
ss << "\"file\":\"" << escape_json(step.action.file_path) << "\",";
ss << "\"command\":\"" << escape_json(step.action.command) << "\",";
ss << "\"explanation\":\"" << escape_json(step.action.explanation) << "\",";
ss << "\"output\":\"" << escape_json(step.output.substr(0, 4000)) << "\"";
ss << "}";
    }

    ss << "]";
    ss << "}";

    return ss.str();
}

std::string YuCodeServer::list_changes_json() {
    auto changes = change_set_.list();

    std::stringstream ss;
    ss << "{";
    ss << "\"changes\":[";

    for (size_t i = 0; i < changes.size(); i++) {
        const auto& c = changes[i];

        if (i > 0) ss << ",";

        ss << "{";
        ss << "\"id\":\"" << escape_json(c.id) << "\",";
        ss << "\"file_path\":\"" << escape_json(c.file_path) << "\",";
        ss << "\"explanation\":\"" << escape_json(c.explanation) << "\",";
        ss << "\"old_content\":\"" << escape_json(c.old_content.substr(0, 20000)) << "\",";
        ss << "\"new_content\":\"" << escape_json(c.new_content.substr(0, 20000)) << "\",";
        ss << "\"unified_diff\":\"" << escape_json(c.unified_diff.substr(0, 20000)) << "\",";
        ss << "\"files\":[";

        for (size_t fi = 0; fi < c.files.size(); fi++) {
            const auto& f = c.files[fi];

            if (fi > 0) ss << ",";

            ss << "{";
            ss << "\"file_path\":\"" << escape_json(f.file_path) << "\",";
            ss << "\"old_content\":\"" << escape_json(f.old_content.substr(0, 20000)) << "\",";
            ss << "\"new_content\":\"" << escape_json(f.new_content.substr(0, 20000)) << "\",";
            ss << "\"unified_diff\":\"" << escape_json(f.unified_diff.substr(0, 20000)) << "\"";
            ss << "}";
        }

        ss << "]";
        ss << "}";
    }

    ss << "]";
    ss << "}";

    return ss.str();
}

std::string YuCodeServer::apply_change_json(const std::string& body) {
    std::string id = json_value(body, "id");

    PendingChange change;
    if (!change_set_.get(id, change)) {
        return R"({"success":false,"error":"change not found"})";
    }

    FileEditor editor;
bool ok = true;
std::string failed_file;

if (!change.files.empty()) {
    for (const auto& file : change.files) {
        if (!editor.rewrite_file(file.file_path, file.new_content)) {
            ok = false;
            failed_file = file.file_path;
            break;
        }
    }
} else {
    if (!editor.rewrite_file(change.file_path, change.new_content)) {
        ok = false;
        failed_file = change.file_path;
    }
}

if (!ok) {
    return std::string(R"({"success":false,"error":"failed to apply change","file_path":")") +
           escape_json(failed_file) +
           R"("})";
}

    change_set_.remove(id);

    std::string verify_command = pick_server_verify_command(
        current_build_command_,
        current_test_command_
    );

    std::string verify_output;

    if (!verify_command.empty() && !current_workspace_path_.empty()) {
        CommandRunner runner;
        verify_output = runner.run(
            verify_command,
            current_workspace_path_
        );
    }

    ErrorParser error_parser;
auto parsed_errors = error_parser.parse(verify_output);
std::string parsed_errors_json = error_parser.to_json(parsed_errors);

    std::stringstream ss;
    ss << "{";
    ss << "\"success\":true,";
    ss << "\"applied_change\":\"" << escape_json(id) << "\",";
    ss << "\"verify_command\":\"" << escape_json(verify_command) << "\",";
    ss << "\"verify_output\":\"" << escape_json(verify_output.substr(0, 8000)) << "\",";
    ss << "\"errors\":" << parsed_errors_json;
    ss << "}";

    return ss.str();
}

std::string YuCodeServer::reject_change_json(const std::string& body) {
    std::string id = json_value(body, "id");

    bool ok = change_set_.remove(id);

    if (!ok) {
        return R"({"success":false,"error":"change not found"})";
    }

    return R"({"success":true})";
}

std::string YuCodeServer::reindex_json(const std::string& body) {
    std::string workspace_path = json_value(body, "workspace_path");

    if (workspace_path.empty()) {
        return R"({"success":false,"error":"workspace_path is required"})";
    }

    try {
        codebase_index_ = std::make_unique<CodebaseIndex>(workspace_path);
        codebase_index_->build();

        ProjectDetector detector;
ProjectInfo project = detector.detect(workspace_path);

current_workspace_path_ = workspace_path;
current_build_command_ = project.build_command;
current_test_command_ = project.test_command;

        agent_ = std::make_unique<Agent>(&change_set_, codebase_index_.get());

        std::stringstream ss;
        ss << "{";
        ss << "\"success\":true,";
        ss << "\"workspace_path\":\"" << escape_json(codebase_index_->workspace_path()) << "\",";
        ss << "\"files\":" << codebase_index_->files().size();
        ss << "}";

        return ss.str();
    } catch (const std::exception& e) {
        return std::string(R"({"success":false,"error":")") +
               escape_json(e.what()) +
               R"("})";
    }
}

std::string YuCodeServer::clear_session_json() {
    session_.clear();
    return R"({"success":true})";
}

std::string YuCodeServer::get_config_json() {
    YuCodeConfig config = YuCodeConfig::load();
    return config.to_json();
}

static bool json_bool_value_server(
    const std::string& json,
    const std::string& key,
    bool fallback
) {
    std::string token = "\"" + key + "\"";

    size_t pos = json.find(token);
    if (pos == std::string::npos) return fallback;

    pos = json.find(':', pos);
    if (pos == std::string::npos) return fallback;

    pos++;

    while (pos < json.size() &&
           (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }

    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;

    return fallback;
}

std::string YuCodeServer::update_config_json(const std::string& body) {
    YuCodeConfig config = YuCodeConfig::load();

    std::string value;

    value = json_value(body, "llm_provider");
    if (!value.empty()) config.llm_provider = value;

    value = json_value(body, "llm_base_url");
    if (!value.empty()) config.llm_base_url = value;

    value = json_value(body, "llm_model");
    if (!value.empty()) config.llm_model = value;

    value = json_value(body, "llm_api_key");
    config.llm_api_key = value;

    config.embedding_enabled =
        json_bool_value_server(body, "embedding_enabled", config.embedding_enabled);

    value = json_value(body, "embedding_provider");
    if (!value.empty()) config.embedding_provider = value;

    value = json_value(body, "embedding_base_url");
    if (!value.empty()) config.embedding_base_url = value;

    value = json_value(body, "embedding_model");
    if (!value.empty()) config.embedding_model = value;

    config.embedded_runtime_enabled =
    json_bool_value_server(body, "embedded_runtime_enabled", config.embedded_runtime_enabled);

value = json_value(body, "embedded_server_path");
if (!value.empty()) config.embedded_server_path = value;

value = json_value(body, "embedded_model_path");
if (!value.empty()) config.embedded_model_path = value;

value = json_value(body, "embedded_runtime_port");
if (!value.empty()) config.embedded_runtime_port = std::stoi(value);

value = json_value(body, "embedded_context_size");
if (!value.empty()) config.embedded_context_size = std::stoi(value);

value = json_value(body, "embedded_gpu_layers");
if (!value.empty()) config.embedded_gpu_layers = std::stoi(value);

    bool ok = YuCodeConfig::save(config);

    if (!ok) {
        return R"({"success":false,"error":"failed to save config"})";
    }

    return std::string(R"({"success":true,"config":)") +
           config.to_json() +
           R"(})";
}

std::string YuCodeServer::test_llm_json() {
    try {
        OpenAICompatibleClient client;

        std::string response = client.generate(
            "Return ONLY this JSON: {\"action\":\"done\",\"explanation\":\"LLM test successful\"}"
        );

        std::stringstream ss;
        ss << "{";
        ss << "\"success\":true,";
        ss << "\"response\":\"" << escape_json(response.substr(0, 4000)) << "\"";
        ss << "}";

        return ss.str();
    } catch (const std::exception& e) {
        return std::string(R"({"success":false,"error":")") +
               escape_json(e.what()) +
               R"("})";
    }
}

std::string YuCodeServer::test_embedding_json() {
    try {
        YuCodeConfig config = YuCodeConfig::load();

        EmbeddingRequest request;
        request.provider = config.embedding_provider;
        request.base_url = config.embedding_base_url;
        request.model = config.embedding_model;

        OpenAICompatibleEmbeddingProvider openai_provider;
        OllamaEmbeddingProvider ollama_provider;

        EmbeddingProvider* provider = &openai_provider;

        if (request.provider == "ollama") {
            provider = &ollama_provider;
        }

        auto embedding = provider->embed(
            "YuCode embedding provider test",
            request
        );

        std::stringstream ss;
        ss << "{";
        ss << "\"success\":" << (!embedding.empty() ? "true" : "false") << ",";
        ss << "\"provider\":\"" << escape_json(request.provider) << "\",";
        ss << "\"model\":\"" << escape_json(request.model) << "\",";
        ss << "\"dimensions\":" << embedding.size();
        ss << "}";

        return ss.str();
    } catch (const std::exception& e) {
        return std::string(R"({"success":false,"error":")") +
               escape_json(e.what()) +
               R"("})";
    }
}

std::string YuCodeServer::index_status_json() {
    YuCodeConfig config = YuCodeConfig::load();

    std::stringstream ss;

    ss << "{";
    ss << "\"workspace_path\":\"" << escape_json(current_workspace_path_) << "\",";
    ss << "\"has_index\":" << (codebase_index_ ? "true" : "false") << ",";
    ss << "\"embedding_enabled\":" << (config.embedding_enabled ? "true" : "false") << ",";

    if (codebase_index_) {
        ss << "\"files\":" << codebase_index_->files().size() << ",";
        ss << "\"symbols\":" << codebase_index_->symbols().size() << ",";
        ss << "\"references\":" << codebase_index_->references().size() << ",";
        ss << "\"calls\":" << codebase_index_->calls().size() << ",";
        ss << "\"embedding_chunks\":" << codebase_index_->vector_size() << ",";
        ss << "\"semantic_ready\":" << (codebase_index_->vector_size() > 0 ? "true" : "false");
    } else {
        ss << "\"files\":0,";
        ss << "\"symbols\":0,";
        ss << "\"references\":0,";
        ss << "\"calls\":0,";
        ss << "\"embedding_chunks\":0,";
        ss << "\"semantic_ready\":false";
    }

    ss << "}";

    return ss.str();
}

std::string YuCodeServer::embedded_runtime_status_json() {
    YuCodeConfig config = YuCodeConfig::load();

    ModelRuntimeConfig runtime_config;
    runtime_config.enabled = config.embedded_runtime_enabled;
    runtime_config.server_path = config.embedded_server_path;
    runtime_config.model_path = config.embedded_model_path;
    runtime_config.port = config.embedded_runtime_port;
    runtime_config.context_size = config.embedded_context_size;
    runtime_config.gpu_layers = config.embedded_gpu_layers;

    ModelRuntimeManager runtime(runtime_config);

    bool running_before = runtime.is_running();
    bool running_after = running_before;

    if (config.embedded_runtime_enabled && !running_before) {
        running_after = runtime.ensure_running();
    }

    std::stringstream ss;
    ss << "{";
    ss << "\"enabled\":" << (config.embedded_runtime_enabled ? "true" : "false") << ",";
    ss << "\"running\":" << (running_after ? "true" : "false") << ",";
    ss << "\"base_url\":\"" << escape_json(runtime.base_url()) << "\",";
    ss << "\"server_path\":\"" << escape_json(config.embedded_server_path) << "\",";
    ss << "\"model_path\":\"" << escape_json(config.embedded_model_path) << "\"";
    ss << "}";

    return ss.str();
}

std::string YuCodeServer::update_index_file_json(const std::string& body) {
    std::string file_path = json_value(body, "file_path");

    if (file_path.empty()) {
        return R"({"success":false,"error":"file_path is required"})";
    }

    if (!codebase_index_) {
        return R"({"success":false,"error":"index is not initialized"})";
    }

    codebase_index_->update_file(file_path);

    return R"({"success":true})";
}

std::string YuCodeServer::remove_index_file_json(const std::string& body) {
    std::string file_path = json_value(body, "file_path");

    if (file_path.empty()) {
        return R"({"success":false,"error":"file_path is required"})";
    }

    if (!codebase_index_) {
        return R"({"success":false,"error":"index is not initialized"})";
    }

    codebase_index_->remove_file(file_path);

    return R"({"success":true})";
}

std::string YuCodeServer::model_status_json() {
    YuCodeConfig config = YuCodeConfig::load();

    ModelRuntimeConfig runtime_config;
    runtime_config.enabled = config.embedded_runtime_enabled;
    runtime_config.server_path = config.embedded_server_path;
    runtime_config.model_path = config.embedded_model_path;
    runtime_config.port = config.embedded_runtime_port;
    runtime_config.context_size = config.embedded_context_size;
    runtime_config.gpu_layers = config.embedded_gpu_layers;

    ModelRuntimeManager runtime(runtime_config);

    bool server_exists = std::filesystem::exists(config.embedded_server_path);
    bool model_exists = std::filesystem::exists(config.embedded_model_path);
    bool runtime_running = runtime.is_running();

    std::stringstream ss;

    ss << "{";
    ss << "\"embedded_runtime_enabled\":" << (config.embedded_runtime_enabled ? "true" : "false") << ",";
    ss << "\"server_path\":\"" << escape_json(config.embedded_server_path) << "\",";
    ss << "\"server_exists\":" << (server_exists ? "true" : "false") << ",";
    ss << "\"model_path\":\"" << escape_json(config.embedded_model_path) << "\",";
    ss << "\"model_exists\":" << (model_exists ? "true" : "false") << ",";
    ss << "\"runtime_running\":" << (runtime_running ? "true" : "false") << ",";
    ss << "\"base_url\":\"" << escape_json(runtime.base_url()) << "\",";
    ss << "\"port\":" << config.embedded_runtime_port;
    ss << "}";

    return ss.str();
}

std::string YuCodeServer::run_agent_stream_json(const std::string& body) {
    AgentRequest request;

    request.query = json_value(body, "query");
    session_.add_user_message(request.query);
    request.session_context = session_.build_prompt_context();

    request.workspace_path = json_value(body, "workspace_path");
    request.active_file = json_value(body, "active_file");
    request.selected_text = json_value(body, "selected_text");
    request.extra_context = json_value(body, "extra_context");
    request.mode = json_value(body, "mode");

    if (request.mode.empty()) {
        request.mode = "edit";
    }

    if (request.query.empty()) {
        return R"({"success":false,"error":"query is required"})";
    }

    if (request.workspace_path.empty()) {
        request.workspace_path = ".";
    }

    ProjectDetector detector;
    ProjectInfo project = detector.detect(request.workspace_path);

    request.project_type = project.type;
    request.build_command = project.build_command;
    request.test_command = project.test_command;

    ProjectSummaryBuilder summary_builder;
    ProjectSummary summary = summary_builder.build(
        request.workspace_path,
        request.project_type,
        request.build_command,
        request.test_command
    );

    request.project_summary = summary.to_prompt();

    current_workspace_path_ = request.workspace_path;
    current_build_command_ = project.build_command;
    current_test_command_ = project.test_command;

    if (!codebase_index_ ||
        codebase_index_->workspace_path() != normalize_path(request.workspace_path)) {

        codebase_index_ = std::make_unique<CodebaseIndex>(request.workspace_path);
        codebase_index_->build();

        agent_ = std::make_unique<Agent>(&change_set_, codebase_index_.get());
    }

    std::string streamed_text;

    AgentResponse result = agent_->run_stream(
        request,
        [&](const std::string& chunk) {
            streamed_text += chunk;
        }
    );

    session_.add_assistant_message(result.final_message);

    std::stringstream ss;

    ss << "{";
    ss << "\"success\":" << (result.success ? "true" : "false") << ",";
    ss << "\"message\":\"" << escape_json(result.final_message) << "\",";
    ss << "\"streamed_text\":\"" << escape_json(streamed_text.substr(0, 12000)) << "\",";
    ss << "\"pending_change_ids\":[";

    for (size_t i = 0; i < result.pending_change_ids.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\"" << escape_json(result.pending_change_ids[i]) << "\"";
    }

    ss << "],";
    ss << "\"steps\":[";

    for (size_t i = 0; i < result.steps.size(); i++) {
        const auto& step = result.steps[i];

        if (i > 0) ss << ",";

        ss << "{";
        ss << "\"success\":" << (step.success ? "true" : "false") << ",";
        ss << "\"action\":\"" << escape_json(action_type_to_string(step.action.type)) << "\",";
        ss << "\"query\":\"" << escape_json(step.action.query) << "\",";
        ss << "\"file\":\"" << escape_json(step.action.file_path) << "\",";
        ss << "\"command\":\"" << escape_json(step.action.command) << "\",";
        ss << "\"explanation\":\"" << escape_json(step.action.explanation) << "\",";
        ss << "\"output\":\"" << escape_json(step.output.substr(0, 4000)) << "\"";
        ss << "}";
    }

    ss << "]";
    ss << "}";

    return ss.str();
}

static void write_sse_event(
    httplib::DataSink& sink,
    const std::string& event,
    const std::string& json_data
) {
    std::string payload;

    payload += "event: ";
    payload += event;
    payload += "\n";

    payload += "data: ";
    payload += json_data;
    payload += "\n\n";

    sink.write(payload.c_str(), payload.size());
}

std::string sse_json_message(
    const std::string& key,
    const std::string& value
) {
    std::stringstream ss;

    ss << "{";
    ss << "\"" << key << "\":\"" << escape_json(value) << "\"";
    ss << "}";

    return ss.str();
}

void YuCodeServer::run_agent_stream_sse(
    const std::string& body,
    httplib::DataSink& sink
) {
    AgentRequest request;

    request.query = json_value(body, "query");
    session_.add_user_message(request.query);
    request.session_context = session_.build_prompt_context();

    request.workspace_path = json_value(body, "workspace_path");
    request.active_file = json_value(body, "active_file");
    request.selected_text = json_value(body, "selected_text");
    request.extra_context = json_value(body, "extra_context");
    request.mode = json_value(body, "mode");

    if (request.mode.empty()) {
        request.mode = "edit";
    }

    if (request.query.empty()) {
        write_sse_event(
            sink,
            "error",
            sse_json_message("error", "query is required")
        );
        return;
    }

    if (request.workspace_path.empty()) {
        request.workspace_path = ".";
    }

    write_sse_event(
        sink,
        "status",
        sse_json_message("message", "Preparing project context...")
    );

    ProjectDetector detector;
    ProjectInfo project = detector.detect(request.workspace_path);

    request.project_type = project.type;
    request.build_command = project.build_command;
    request.test_command = project.test_command;

    ProjectSummaryBuilder summary_builder;
    ProjectSummary summary = summary_builder.build(
        request.workspace_path,
        request.project_type,
        request.build_command,
        request.test_command
    );

    request.project_summary = summary.to_prompt();

    current_workspace_path_ = request.workspace_path;
    current_build_command_ = project.build_command;
    current_test_command_ = project.test_command;

    if (!codebase_index_ ||
        codebase_index_->workspace_path() != normalize_path(request.workspace_path)) {

        write_sse_event(
            sink,
            "status",
            sse_json_message("message", "Building codebase index...")
        );

        codebase_index_ = std::make_unique<CodebaseIndex>(request.workspace_path);
        codebase_index_->build();

        agent_ = std::make_unique<Agent>(&change_set_, codebase_index_.get());
    }

    write_sse_event(
        sink,
        "status",
        sse_json_message("message", "Running agent...")
    );

    AgentResponse result = agent_->run_stream(
        request,
        [&](const std::string& chunk) {
            write_sse_event(
                sink,
                "token",
                sse_json_message("chunk", chunk)
            );
        }
    );

    session_.add_assistant_message(result.final_message);

    std::stringstream done;

    done << "{";
    done << "\"success\":" << (result.success ? "true" : "false") << ",";
    done << "\"message\":\"" << escape_json(result.final_message) << "\",";
    done << "\"pending_change_ids\":[";

    for (size_t i = 0; i < result.pending_change_ids.size(); i++) {
        if (i > 0) done << ",";
        done << "\"" << escape_json(result.pending_change_ids[i]) << "\"";
    }

    done << "],";
    done << "\"steps\":[";

    for (size_t i = 0; i < result.steps.size(); i++) {
        const auto& step = result.steps[i];

        if (i > 0) done << ",";

        done << "{";
        done << "\"success\":" << (step.success ? "true" : "false") << ",";
        done << "\"action\":\"" << escape_json(action_type_to_string(step.action.type)) << "\",";
        done << "\"query\":\"" << escape_json(step.action.query) << "\",";
        done << "\"file\":\"" << escape_json(step.action.file_path) << "\",";
        done << "\"command\":\"" << escape_json(step.action.command) << "\",";
        done << "\"explanation\":\"" << escape_json(step.action.explanation) << "\",";
        done << "\"output\":\"" << escape_json(step.output.substr(0, 4000)) << "\"";
        done << "}";
    }

    done << "]";
    done << "}";

    write_sse_event(
        sink,
        "done",
        done.str()
    );
}