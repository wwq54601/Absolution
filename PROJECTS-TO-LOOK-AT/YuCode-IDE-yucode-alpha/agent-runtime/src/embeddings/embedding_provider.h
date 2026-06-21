#pragma once
#include <string>
#include <vector>

struct EmbeddingRequest {
    std::string provider = "lmstudio"; 
    std::string base_url = "http://127.0.0.1:1234/v1";
    std::string api_key = "";
    std::string model = "text-embedding-nomic-embed-text-v1.5";
};

class EmbeddingProvider {
public:
    virtual ~EmbeddingProvider() = default;

    virtual std::vector<float> embed(
        const std::string& text,
        const EmbeddingRequest& request
    ) = 0;
};

class OpenAICompatibleEmbeddingProvider : public EmbeddingProvider {
public:
    std::vector<float> embed(
        const std::string& text,
        const EmbeddingRequest& request
    ) override;

private:
    std::string post_json(
        const std::string& url,
        const std::string& body,
        const std::string& api_key
    );

    std::string escape_json(const std::string& text);
    std::vector<float> parse_embedding(const std::string& response);
};

class OllamaEmbeddingProvider : public EmbeddingProvider {
public:
    std::vector<float> embed(
        const std::string& text,
        const EmbeddingRequest& request
    ) override;

private:
    std::string post_json(
        const std::string& url,
        const std::string& body
    );

    std::string escape_json(const std::string& text);
    std::vector<float> parse_embedding(const std::string& response);
};