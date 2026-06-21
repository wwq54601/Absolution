#pragma once
#include <string>

struct ModelRuntimeConfig {
    bool enabled = false;

    std::string server_path = "runtime/llama-server.exe";
    std::string model_path = "models/code-model.gguf";

    std::string host = "127.0.0.1";
    int port = 11435;

    int context_size = 32768;
    int gpu_layers = 0;
};

class ModelRuntimeManager {
public:
    explicit ModelRuntimeManager(const ModelRuntimeConfig& config);

    bool ensure_running();
    bool is_running() const;
    std::string base_url() const;

private:
    ModelRuntimeConfig config_;

    bool file_exists(const std::string& path) const;
    bool port_open() const;
    bool start_process() const;
    std::string build_command() const;
};