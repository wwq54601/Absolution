#include "model_runtime_manager.h"

#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

ModelRuntimeManager::ModelRuntimeManager(const ModelRuntimeConfig& config)
    : config_(config) {}

bool ModelRuntimeManager::ensure_running() {
    if (!config_.enabled) {
        return false;
    }

    if (port_open()) {
        return true;
    }

    if (!file_exists(config_.server_path)) {
        return false;
    }

    if (!file_exists(config_.model_path)) {
        return false;
    }

    return start_process();
}

bool ModelRuntimeManager::is_running() const {
    return port_open();
}

std::string ModelRuntimeManager::base_url() const {
    std::stringstream ss;
    ss << "http://" << config_.host << ":" << config_.port << "/v1";
    return ss.str();
}

bool ModelRuntimeManager::file_exists(const std::string& path) const {
    return std::filesystem::exists(path);
}

bool ModelRuntimeManager::port_open() const {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(config_.port));
    inet_pton(AF_INET, config_.host.c_str(), &addr.sin_addr);

    bool ok = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;

    closesocket(sock);
    WSACleanup();

    return ok;
#else
    return false;
#endif
}

bool ModelRuntimeManager::start_process() const {
#ifdef _WIN32
    std::string command = build_command();

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        nullptr,
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!ok) {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
#else
    return false;
#endif
}

std::string ModelRuntimeManager::build_command() const {
    std::stringstream ss;

    ss << "\"" << config_.server_path << "\"";
    ss << " -m \"" << config_.model_path << "\"";
    ss << " --host " << config_.host;
    ss << " --port " << config_.port;
    ss << " -c " << config_.context_size;

    if (config_.gpu_layers > 0) {
        ss << " -ngl " << config_.gpu_layers;
    }

    return ss.str();
}