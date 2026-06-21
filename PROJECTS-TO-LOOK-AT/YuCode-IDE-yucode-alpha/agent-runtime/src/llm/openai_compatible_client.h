#pragma once
#include "llm_client.h"
#include "../config/yucode_config.h"

#include <string>
#include <functional>

class OpenAICompatibleClient : public LLMClient {
public:
    OpenAICompatibleClient();

    std::string generate(const std::string& prompt) override;
    void generate_stream(
    const std::string& prompt,
    std::function<void(const std::string&)> on_chunk
) override;

private:
    YuCodeConfig config_;

    std::string post_json(
        const std::string& url,
        const std::string& body,
        const std::string& api_key
    );

    void post_json_stream(
    const std::string& url,
    const std::string& body,
    const std::string& api_key,
    std::function<void(const std::string&)> on_chunk
);

    std::string escape_json(const std::string& text);
    std::string unescape_json(const std::string& text);
    std::string extract_content(const std::string& response);
};