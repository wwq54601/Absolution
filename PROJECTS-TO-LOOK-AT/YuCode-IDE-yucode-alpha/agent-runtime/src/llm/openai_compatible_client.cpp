#include "openai_compatible_client.h"

#include <windows.h>
#include <winhttp.h>

#include <sstream>
#include <string>
#include <fstream>

#pragma comment(lib, "winhttp.lib")

OpenAICompatibleClient::OpenAICompatibleClient() {
    config_ = YuCodeConfig::load();
}

std::string OpenAICompatibleClient::generate(const std::string& prompt) {
    std::string model = config_.llm_model.empty()
        ? "local-model"
        : config_.llm_model;

    std::stringstream body;

    body
        << R"({"model":")" << escape_json(model) << R"(",)"
        << R"("messages":[)"
        << R"({"role":"system","content":"You are YuCode, an autonomous local coding agent. Always follow the requested response format exactly."},)"
        << R"({"role":"user","content":")"
        << escape_json(prompt)
        << R"("}],)"
        << R"("temperature":0.1,"max_tokens":256})";

    std::string url = config_.llm_base_url + "/chat/completions";

    std::string response = post_json(
    url,
    body.str(),
    config_.llm_api_key
);

if (response.empty()) {
    Sleep(1000);

    response = post_json(
        url,
        body.str(),
        config_.llm_api_key
    );
}

if (response.empty()) {
    Sleep(2000);

    response = post_json(
        url,
        body.str(),
        config_.llm_api_key
    );
}

    std::ofstream debug("llm_debug_response.log", std::ios::binary | std::ios::app);

debug << "\n\n===== LLM RESPONSE BEGIN =====\n";
debug << response;
debug << "\n===== LLM RESPONSE END =====\n";

debug.close();

    if (response.empty()) {
    std::ofstream debug("llm_debug_response.log", std::ios::binary | std::ios::app);
    debug << "\n\n===== EMPTY RESPONSE =====\n";
    debug.close();

    return R"({"action":"done","explanation":"LLM provider returned no response. Check yucode.config.json provider/base_url/model settings."})";
}

    std::string content = extract_content(response);

    if (content.empty()) {
        return R"({"action":"done","explanation":"Could not parse LLM provider response."})";
    }

    return content;
}

std::string OpenAICompatibleClient::post_json(
    const std::string& url,
    const std::string& body,
    const std::string& api_key
) {
    std::string host = "127.0.0.1";
    int port = 1234;
    std::string path = "/v1/chat/completions";
    bool https = false;

    size_t scheme = url.find("://");
    size_t host_start = scheme == std::string::npos ? 0 : scheme + 3;

    if (url.rfind("https://", 0) == 0) {
        https = true;
        port = 443;
    }

    if (url.rfind("http://", 0) == 0) {
        https = false;
        port = 80;
    }

    size_t path_pos = url.find("/", host_start);
    std::string host_port = path_pos == std::string::npos
        ? url.substr(host_start)
        : url.substr(host_start, path_pos - host_start);

    if (path_pos != std::string::npos) {
        path = url.substr(path_pos);
    }

    size_t colon = host_port.find(":");

    if (colon != std::string::npos) {
        host = host_port.substr(0, colon);
        port = std::stoi(host_port.substr(colon + 1));
    } else {
        host = host_port;
    }

    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    HINTERNET session = WinHttpOpen(
        L"YuCode-LLM/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session) return "";

     WinHttpSetTimeouts(
    session,
    30000,   // resolve
    30000,   // connect
    30000,   // send
    180000   // receive
);

    HINTERNET connect = WinHttpConnect(
        session,
        whost.c_str(),
        static_cast<INTERNET_PORT>(port),
        0
    );

    if (!connect) {
        WinHttpCloseHandle(session);
        return "";
    }

    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET req = WinHttpOpenRequest(
        connect,
        L"POST",
        wpath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!req) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::wstring headers = L"Content-Type: application/json\r\n";

    if (!api_key.empty()) {
        std::wstring wkey(api_key.begin(), api_key.end());
        headers += L"Authorization: Bearer " + wkey + L"\r\n";
    }

    std::ofstream reqdebug("request_debug.json");
reqdebug << body;
reqdebug.close();

    BOOL sent = WinHttpSendRequest(
        req,
        headers.c_str(),
        static_cast<DWORD>(headers.size()),
        (LPVOID)body.c_str(),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0
    );

    if (!sent) {
    DWORD err = GetLastError();

    std::ofstream errfile("http_error.txt", std::ios::binary | std::ios::trunc);
    errfile << err;
    errfile.close();

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return "";
}

    if (!WinHttpReceiveResponse(req, nullptr)) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        DWORD err = GetLastError();

std::ofstream errfile("receive_error.txt");
errfile << err;
errfile.close();
        return "";
    }

    std::string response;
    DWORD available = 0;

    do {
        available = 0;

        if (!WinHttpQueryDataAvailable(req, &available)) {
            break;
        }

        if (available == 0) {
            break;
        }

        std::string buffer;
        buffer.resize(available);

        DWORD read = 0;

        if (!WinHttpReadData(req, buffer.data(), available, &read)) {
            break;
        }

        buffer.resize(read);
        response += buffer;

    } while (available > 0);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    std::ofstream f("response_size.txt");
f << response.size();

    return response;
}

std::string OpenAICompatibleClient::escape_json(const std::string& text) {
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

std::string OpenAICompatibleClient::unescape_json(const std::string& text) {
    std::string out;
    bool escaped = false;

    for (char c : text) {
        if (escaped) {
            if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else if (c == '"') out += '"';
            else if (c == '\\') out += '\\';
            else out += c;

            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        out += c;
    }

    return out;
}

std::string OpenAICompatibleClient::extract_content(const std::string& response) {
    std::string key = R"("content")";
    size_t key_pos = response.find(key);

    if (key_pos == std::string::npos) {
        return "";
    }

    size_t colon = response.find(":", key_pos);

    if (colon == std::string::npos) {
        return "";
    }

    size_t start = response.find("\"", colon + 1);

    if (start == std::string::npos) {
        return "";
    }

    start++;

    std::string raw;
    bool escaped = false;

    for (size_t i = start; i < response.size(); i++) {
        char c = response[i];

        if (escaped) {
            raw += '\\';
            raw += c;
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            break;
        }

        raw += c;
    }

    return unescape_json(raw);
}

void OpenAICompatibleClient::generate_stream(
    const std::string& prompt,
    std::function<void(const std::string&)> on_chunk
) {
    std::string model = config_.llm_model.empty()
        ? "local-model"
        : config_.llm_model;

    std::stringstream body;

    body
        << R"({"model":")" << escape_json(model) << R"(",)"
        << R"("messages":[)"
        << R"({"role":"system","content":"You are YuCode, an autonomous local coding agent. Always follow the requested response format exactly."},)"
        << R"({"role":"user","content":")"
        << escape_json(prompt)
        << R"("}],)"
        << R"("temperature":0.1,"max_tokens":1024,"stream":true})";

    std::string url = config_.llm_base_url + "/chat/completions";

    post_json_stream(
        url,
        body.str(),
        config_.llm_api_key,
        on_chunk
    );
}

static std::string unescape_json_stream_local(
    const std::string& text
) {
    std::string out;
    bool escaped = false;

    for (char c : text) {
        if (escaped) {
            if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else if (c == '"') out += '"';
            else if (c == '\\') out += '\\';
            else out += c;

            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        out += c;
    }

    return out;
}

static std::string extract_stream_content_delta(
    const std::string& json
) {
    std::string key = R"("content")";

    size_t key_pos = json.find(key);
    if (key_pos == std::string::npos) {
        return "";
    }

    size_t colon = json.find(":", key_pos);
    if (colon == std::string::npos) {
        return "";
    }

    size_t start = json.find("\"", colon + 1);
    if (start == std::string::npos) {
        return "";
    }

    start++;

    std::string raw;
    bool escaped = false;

    for (size_t i = start; i < json.size(); i++) {
        char c = json[i];

        if (escaped) {
            raw += "\\";
            raw += c;
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            break;
        }

        raw += c;
    }

    return unescape_json_stream_local(raw);
}

void OpenAICompatibleClient::post_json_stream(
    const std::string& url,
    const std::string& body,
    const std::string& api_key,
    std::function<void(const std::string&)> on_chunk
) {
    std::string host = "127.0.0.1";
    int port = 1234;
    std::string path = "/v1/chat/completions";
    bool https = false;

    size_t scheme = url.find("://");
    size_t host_start = scheme == std::string::npos ? 0 : scheme + 3;

    if (url.rfind("https://", 0) == 0) {
        https = true;
        port = 443;
    }

    if (url.rfind("http://", 0) == 0) {
        https = false;
        port = 80;
    }

    size_t path_pos = url.find("/", host_start);

    std::string host_port = path_pos == std::string::npos
        ? url.substr(host_start)
        : url.substr(host_start, path_pos - host_start);

    if (path_pos != std::string::npos) {
        path = url.substr(path_pos);
    }

    size_t colon = host_port.find(":");

    if (colon != std::string::npos) {
        host = host_port.substr(0, colon);
        port = std::stoi(host_port.substr(colon + 1));
    } else {
        host = host_port;
    }

    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    HINTERNET session = WinHttpOpen(
        L"YuCode-LLM-Stream/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session) {
        return;
    }

    WinHttpSetTimeouts(
        session,
        30000,
        30000,
        30000,
        180000
    );

    HINTERNET connect = WinHttpConnect(
        session,
        whost.c_str(),
        static_cast<INTERNET_PORT>(port),
        0
    );

    if (!connect) {
        WinHttpCloseHandle(session);
        return;
    }

    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET req = WinHttpOpenRequest(
        connect,
        L"POST",
        wpath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!req) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return;
    }

    std::wstring headers =
        L"Content-Type: application/json\r\n"
        L"Accept: text/event-stream\r\n";

    if (!api_key.empty()) {
        std::wstring wkey(api_key.begin(), api_key.end());
        headers += L"Authorization: Bearer " + wkey + L"\r\n";
    }

    BOOL sent = WinHttpSendRequest(
        req,
        headers.c_str(),
        static_cast<DWORD>(headers.size()),
        (LPVOID)body.c_str(),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0
    );

    if (!sent) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return;
    }

    if (!WinHttpReceiveResponse(req, nullptr)) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return;
    }

    std::string pending;
    DWORD available = 0;

    while (true) {
        available = 0;

        if (!WinHttpQueryDataAvailable(req, &available)) {
            break;
        }

        if (available == 0) {
            break;
        }

        std::string buffer;
        buffer.resize(available);

        DWORD read = 0;

        if (!WinHttpReadData(req, buffer.data(), available, &read)) {
            break;
        }

        buffer.resize(read);
        pending += buffer;

        size_t pos = 0;

        while ((pos = pending.find("\n\n")) != std::string::npos) {
            std::string event = pending.substr(0, pos);
            pending.erase(0, pos + 2);

            std::stringstream lines(event);
            std::string line;

            while (std::getline(lines, line)) {
                if (line.rfind("data:", 0) != 0) {
                    continue;
                }

                std::string data = line.substr(5);

                while (!data.empty() &&
                       (data[0] == ' ' || data[0] == '\t')) {
                    data.erase(data.begin());
                }

                if (data == "[DONE]") {
                    WinHttpCloseHandle(req);
                    WinHttpCloseHandle(connect);
                    WinHttpCloseHandle(session);
                    return;
                }

                std::string delta =
                    extract_stream_content_delta(data);

                if (!delta.empty() && on_chunk) {
                    on_chunk(delta);
                }
            }
        }
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
}