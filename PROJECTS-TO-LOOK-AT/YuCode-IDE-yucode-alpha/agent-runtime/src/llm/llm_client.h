#pragma once

#include <string>
#include <functional>

class LLMClient {
public:
    virtual ~LLMClient() = default;

    virtual std::string generate(const std::string& prompt) = 0;

    virtual void generate_stream(
        const std::string& prompt,
        std::function<void(const std::string&)> on_chunk
    ) = 0;
};