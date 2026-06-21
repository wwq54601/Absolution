#include "yucode_config.h"

#include <fstream>
#include <sstream>

static std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);

    if (!in.is_open()) {
        return "";
    }

    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static std::string json_value(
    const std::string& json,
    const std::string& key
) {
    std::string token = "\"" + key + "\"";

    size_t pos = json.find(token);

    if (pos == std::string::npos) {
        return "";
    }

    pos = json.find(':', pos);

    if (pos == std::string::npos) {
        return "";
    }

    pos++;

    while (pos < json.size() &&
           (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }

    if (pos >= json.size()) {
        return "";
    }

    if (json[pos] == '"') {
        pos++;

        size_t end = json.find('"', pos);

        if (end == std::string::npos) {
            return "";
        }

        return json.substr(pos, end - pos);
    }

    return "";
}

static bool json_bool(
    const std::string& json,
    const std::string& key,
    bool fallback
) {
    std::string token = "\"" + key + "\"";

    size_t pos = json.find(token);

    if (pos == std::string::npos) {
        return fallback;
    }

    pos = json.find(':', pos);

    if (pos == std::string::npos) {
        return fallback;
    }

    pos++;

    while (pos < json.size() &&
           (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }

    if (json.compare(pos, 4, "true") == 0) {
        return true;
    }

    if (json.compare(pos, 5, "false") == 0) {
        return false;
    }

    return fallback;
}

YuCodeConfig YuCodeConfig::load() {
    YuCodeConfig cfg;

    std::string json = read_file("yucode.config.json");

    if (json.empty()) {
        return cfg;
    }

    std::string value;

    value = json_value(json, "llm_provider");
    if (!value.empty()) cfg.llm_provider = value;

    value = json_value(json, "llm_base_url");
    if (!value.empty()) cfg.llm_base_url = value;

    value = json_value(json, "llm_model");
    if (!value.empty()) cfg.llm_model = value;

    value = json_value(json, "llm_api_key");
if (!value.empty()) cfg.llm_api_key = value;

    cfg.embedding_enabled =
        json_bool(json, "embedding_enabled", false);

    value = json_value(json, "embedding_provider");
    if (!value.empty()) cfg.embedding_provider = value;

    value = json_value(json, "embedding_base_url");
    if (!value.empty()) cfg.embedding_base_url = value;

    value = json_value(json, "embedding_model");
    if (!value.empty()) cfg.embedding_model = value;

    cfg.embedded_runtime_enabled =
    json_bool(json, "embedded_runtime_enabled", false);

value = json_value(json, "embedded_server_path");
if (!value.empty()) cfg.embedded_server_path = value;

value = json_value(json, "embedded_model_path");
if (!value.empty()) cfg.embedded_model_path = value;

value = json_value(json, "embedded_runtime_port");
if (!value.empty()) cfg.embedded_runtime_port = std::stoi(value);

value = json_value(json, "embedded_context_size");
if (!value.empty()) cfg.embedded_context_size = std::stoi(value);

value = json_value(json, "embedded_gpu_layers");
if (!value.empty()) cfg.embedded_gpu_layers = std::stoi(value);

    return cfg;
}

static std::string escape_json_config(const std::string& text) {
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

std::string YuCodeConfig::to_json() const {
    std::stringstream ss;

    ss << "{";
    ss << "\"llm_provider\":\"" << escape_json_config(llm_provider) << "\",";
    ss << "\"llm_base_url\":\"" << escape_json_config(llm_base_url) << "\",";
    ss << "\"llm_model\":\"" << escape_json_config(llm_model) << "\",";
    ss << "\"llm_api_key\":\"" << escape_json_config(llm_api_key) << "\",";
    ss << "\"embedding_enabled\":" << (embedding_enabled ? "true" : "false") << ",";
    ss << "\"embedding_provider\":\"" << escape_json_config(embedding_provider) << "\",";
    ss << "\"embedding_base_url\":\"" << escape_json_config(embedding_base_url) << "\",";
    ss << "\"embedding_model\":\"" << escape_json_config(embedding_model) << "\",";
    ss << "\"embedded_runtime_enabled\":" << (embedded_runtime_enabled ? "true" : "false") << ",";
    ss << "\"embedded_server_path\":\"" << escape_json_config(embedded_server_path) << "\",";
    ss << "\"embedded_model_path\":\"" << escape_json_config(embedded_model_path) << "\",";
    ss << "\"embedded_runtime_port\":\"" << embedded_runtime_port << "\",";
    ss << "\"embedded_context_size\":\"" << embedded_context_size << "\",";
    ss << "\"embedded_gpu_layers\":\"" << embedded_gpu_layers << "\"";
    ss << "}";

    return ss.str();
}

bool YuCodeConfig::save(const YuCodeConfig& config) {
    std::ofstream out("yucode.config.json", std::ios::binary | std::ios::trunc);

    if (!out.is_open()) {
        return false;
    }

    out << config.to_json();
    return true;
}