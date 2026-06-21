#include "error_parser.h"

#include <sstream>
#include <regex>

std::vector<ParsedError> ErrorParser::parse(const std::string& output) const {
    std::vector<ParsedError> errors;

    std::istringstream stream(output);
    std::string line;

    std::regex msvc_regex(
        R"(([^:\r\n]+)\((\d+),(\d+)\):\s*(error|warning)\s+[A-Z0-9]+:\s*(.*))"
    );

    std::regex gcc_clang_regex(
        R"(([^:\r\n]+):(\d+):(\d+):\s*(error|warning):\s*(.*))"
    );

    while (std::getline(stream, line)) {
        std::smatch match;

        if (std::regex_search(line, match, msvc_regex)) {
            ParsedError err;
            err.file_path = match[1];
            err.line = std::stoi(match[2]);
            err.column = std::stoi(match[3]);
            err.message = match[5];
            err.raw_line = line;
            errors.push_back(err);
            continue;
        }

        if (std::regex_search(line, match, gcc_clang_regex)) {
            ParsedError err;
            err.file_path = match[1];
            err.line = std::stoi(match[2]);
            err.column = std::stoi(match[3]);
            err.message = match[5];
            err.raw_line = line;
            errors.push_back(err);
            continue;
        }
    }

    return errors;
}

std::string ErrorParser::to_json(const std::vector<ParsedError>& errors) const {
    std::stringstream ss;

    ss << "[";

    for (size_t i = 0; i < errors.size(); i++) {
        const auto& e = errors[i];

        if (i > 0) ss << ",";

        ss << "{";
        ss << "\"file_path\":\"" << escape_json(e.file_path) << "\",";
        ss << "\"line\":" << e.line << ",";
        ss << "\"column\":" << e.column << ",";
        ss << "\"message\":\"" << escape_json(e.message) << "\",";
        ss << "\"raw_line\":\"" << escape_json(e.raw_line) << "\"";
        ss << "}";
    }

    ss << "]";

    return ss.str();
}

std::string ErrorParser::escape_json(const std::string& text) const {
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