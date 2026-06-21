#include "embedding_provider.h"

#include <windows.h>
#include <winhttp.h>

#include <sstream>
#include <regex>

#pragma comment(lib, "winhttp.lib")

std::vector<float> OpenAICompatibleEmbeddingProvider::embed(
    const std::string& text,
    const EmbeddingRequest& request
) {
    std::stringstream body;

    body << "{";
    body << "\"model\":\"" << escape_json(request.model) << "\",";
    body << "\"input\":\"" << escape_json(text.substr(0, 8000)) << "\"";
    body << "}";

    std::string url = request.base_url + "/embeddings";
    std::string response = post_json(url, body.str(), request.api_key);

    return parse_embedding(response);
}

std::string OpenAICompatibleEmbeddingProvider::post_json(
    const std::string& url,
    const std::string& body,
    const std::string& api_key
) {
    // MVP: localhost only parser.
    // Supported:
    // http://127.0.0.1:1234/v1/embeddings
    // http://localhost:1234/v1/embeddings

    std::string host = "127.0.0.1";
    int port = 1234;
    std::string path = "/v1/embeddings";

    if (url.find("localhost") != std::string::npos) {
        host = "localhost";
    }

    size_t scheme = url.find("://");
    size_t host_start = scheme == std::string::npos ? 0 : scheme + 3;
    size_t port_pos = url.find(":", host_start);
    size_t path_pos = url.find("/", host_start);

    if (port_pos != std::string::npos && path_pos != std::string::npos) {
        host = url.substr(host_start, port_pos - host_start);
        port = std::stoi(url.substr(port_pos + 1, path_pos - port_pos - 1));
        path = url.substr(path_pos);
    }

    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    HINTERNET session = WinHttpOpen(
        L"YuCode-Embedding/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session) return "";

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

    HINTERNET req = WinHttpOpenRequest(
        connect,
        L"POST",
        wpath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
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
        return "";
    }

    if (!WinHttpReceiveResponse(req, nullptr)) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::string response;
    DWORD available = 0;

    do {
        available = 0;

        if (!WinHttpQueryDataAvailable(req, &available)) break;
        if (available == 0) break;

        std::string buffer;
        buffer.resize(available);

        DWORD read = 0;
        if (!WinHttpReadData(req, buffer.data(), available, &read)) break;

        buffer.resize(read);
        response += buffer;

    } while (available > 0);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return response;
}

std::string OpenAICompatibleEmbeddingProvider::escape_json(const std::string& text) {
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

std::vector<float> OpenAICompatibleEmbeddingProvider::parse_embedding(
    const std::string& response
) {
    std::vector<float> values;

    size_t emb = response.find("\"embedding\"");
    if (emb == std::string::npos) return values;

    size_t start = response.find("[", emb);
    size_t end = response.find("]", start);

    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return values;
    }

    std::string arr = response.substr(start + 1, end - start - 1);
    std::stringstream ss(arr);
    std::string item;

    while (std::getline(ss, item, ',')) {
        try {
            values.push_back(std::stof(item));
        } catch (...) {
        }
    }

    return values;
}

std::vector<float> OllamaEmbeddingProvider::embed(
    const std::string& text,
    const EmbeddingRequest& request
) {
    std::stringstream body;

    body << "{";
    body << "\"model\":\"" << escape_json(request.model) << "\",";
    body << "\"prompt\":\"" << escape_json(text.substr(0, 8000)) << "\"";
    body << "}";

    std::string base = request.base_url.empty()
        ? "http://127.0.0.1:11434"
        : request.base_url;

    std::string response = post_json(base + "/api/embeddings", body.str());

    return parse_embedding(response);
}

std::string OllamaEmbeddingProvider::post_json(
    const std::string& url,
    const std::string& body
) {
    std::string host = "127.0.0.1";
    int port = 11434;
    std::string path = "/api/embeddings";

    size_t scheme = url.find("://");
    size_t host_start = scheme == std::string::npos ? 0 : scheme + 3;
    size_t port_pos = url.find(":", host_start);
    size_t path_pos = url.find("/", host_start);

    if (port_pos != std::string::npos && path_pos != std::string::npos) {
        host = url.substr(host_start, port_pos - host_start);
        port = std::stoi(url.substr(port_pos + 1, path_pos - port_pos - 1));
        path = url.substr(path_pos);
    }

    std::wstring whost(host.begin(), host.end());
    std::wstring wpath(path.begin(), path.end());

    HINTERNET session = WinHttpOpen(
        L"YuCode-Ollama-Embedding/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session) return "";

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

    HINTERNET req = WinHttpOpenRequest(
        connect,
        L"POST",
        wpath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!req) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::wstring headers = L"Content-Type: application/json\r\n";

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
        return "";
    }

    if (!WinHttpReceiveResponse(req, nullptr)) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }

    std::string response;
    DWORD available = 0;

    do {
        available = 0;

        if (!WinHttpQueryDataAvailable(req, &available)) break;
        if (available == 0) break;

        std::string buffer;
        buffer.resize(available);

        DWORD read = 0;
        if (!WinHttpReadData(req, buffer.data(), available, &read)) break;

        buffer.resize(read);
        response += buffer;

    } while (available > 0);

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return response;
}

std::string OllamaEmbeddingProvider::escape_json(const std::string& text) {
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

std::vector<float> OllamaEmbeddingProvider::parse_embedding(
    const std::string& response
) {
    std::vector<float> values;

    size_t emb = response.find("\"embedding\"");
    if (emb == std::string::npos) return values;

    size_t start = response.find("[", emb);
    size_t end = response.find("]", start);

    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return values;
    }

    std::string arr = response.substr(start + 1, end - start - 1);
    std::stringstream ss(arr);
    std::string item;

    while (std::getline(ss, item, ',')) {
        try {
            values.push_back(std::stof(item));
        } catch (...) {
        }
    }

    return values;
}