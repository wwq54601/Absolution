#pragma once
#include "llm_client.h"
#include <string>
#include <functional>

class LMStudioClient : public LLMClient {
public:
    LMStudioClient();

    std::string generate(const std::string& prompt) override;
    void generate_stream(
    const std::string& prompt,
    std::function<void(const std::string&)> on_chunk
) override;

private:
    std::string host_;
    int port_;
    std::string path_;

    std::string post(const std::string& body);
    std::string escape_json(const std::string& text);
    std::string unescape_json(const std::string& text);
    std::string extract_content(const std::string& response);
};