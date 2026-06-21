#pragma once
#include <string>

struct YuCodeConfig {
    std::string llm_provider = "lmstudio";
    std::string llm_base_url = "http://127.0.0.1:1234/v1";
    std::string llm_model = "";
    std::string llm_api_key = "";

    bool embedding_enabled = false;

    std::string embedding_provider = "lmstudio";
    std::string embedding_base_url = "http://127.0.0.1:1234/v1";
    std::string embedding_model = "nomic-embed-text";

    std::string to_json() const;

    static YuCodeConfig load();
    static bool save(const YuCodeConfig& config);

    bool embedded_runtime_enabled = false;
std::string embedded_server_path = "runtime/llama-server.exe";
std::string embedded_model_path = "models/code-model.gguf";
int embedded_runtime_port = 11435;
int embedded_context_size = 32768;
int embedded_gpu_layers = 0;
};