#pragma once
#include <string>
#include <vector>

struct ParsedError {
    std::string file_path;
    int line = 0;
    int column = 0;
    std::string message;
    std::string raw_line;
};

class ErrorParser {
public:
    std::vector<ParsedError> parse(const std::string& output) const;
    std::string to_json(const std::vector<ParsedError>& errors) const;

private:
    std::string escape_json(const std::string& text) const;
};