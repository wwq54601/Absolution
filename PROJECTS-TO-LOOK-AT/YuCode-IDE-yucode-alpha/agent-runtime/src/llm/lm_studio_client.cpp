#include "lm_studio_client.h"

#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

LMStudioClient::LMStudioClient()
    : host_("localhost"),
      port_(1234),
      path_("/v1/chat/completions") {}

std::string LMStudioClient::generate(const std::string& prompt) {
    std::stringstream body;

    body
        << R"({"model":"local-model","messages":[)"
        << R"({"role":"system","content":"You are YuCode, an autonomous local coding agent. Always follow the requested response format exactly."},)"
        << R"({"role":"user","content":")"
        << escape_json(prompt)
        << R"("}],)"
        << R"("temperature":0.1,"max_tokens":4096})";

    std::string response = post(body.str());

    if (response.empty()) {
        return R"({"action":"done","explanation":"LM Studio did not return a response. Make sure the local server is running on port 1234."})";
    }

    std::string content = extract_content(response);

    if (content.empty()) {
        return R"({"action":"done","explanation":"Could not parse LM Studio response."})";
    }

    return content;
}

std::string LMStudioClient::post(const std::string& body) {
    HINTERNET session = WinHttpOpen(
        L"YuCode-Agent/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session) return "";

    HINTERNET connect = WinHttpConnect(
        session,
        L"localhost",
        1234,
        0
    );

    if (!connect) {
        WinHttpCloseHandle(session);
        return "";
    }

    HINTERNET request = WinHttpOpenRequest(
        connect,
        L"POST",
        L"/v1/chat/completions",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::wstring headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(
        request,
        headers.c_str(),
        static_cast<DWORD>(headers.size()),
        (LPVOID)body.c_str(),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0
    );

    if (!sent) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    BOOL received = WinHttpReceiveResponse(request, nullptr);

    if (!received) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::string response;
    DWORD available = 0;

    do {
        available = 0;

        if (!WinHttpQueryDataAvailable(request, &available)) {
            break;
        }

        if (available == 0) {
            break;
        }

        std::string buffer;
        buffer.resize(available);

        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &read)) {
            break;
        }

        buffer.resize(read);
        response += buffer;

    } while (available > 0);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return response;
}

std::string LMStudioClient::escape_json(const std::string& text) {
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

std::string LMStudioClient::unescape_json(const std::string& text) {
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

std::string LMStudioClient::extract_content(const std::string& response) {
    std::string key = R"("content")";
    size_t key_pos = response.find(key);

    if (key_pos == std::string::npos) return "";

    size_t colon = response.find(":", key_pos);
    if (colon == std::string::npos) return "";

    size_t start = response.find("\"", colon + 1);
    if (start == std::string::npos) return "";

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

        if (c == '"') break;

        raw += c;
    }

    return unescape_json(raw);
}

void LMStudioClient::generate_stream(
    const std::string& prompt,
    std::function<void(const std::string&)> on_chunk
) {
    std::string result = generate(prompt);

    if (on_chunk) {
        on_chunk(result);
    }
}