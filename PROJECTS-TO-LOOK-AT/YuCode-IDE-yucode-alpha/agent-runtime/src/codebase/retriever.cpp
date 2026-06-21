#include "retriever.h"
#include "../util/path_utils.h"

#include <sstream>
#include <algorithm>

Retriever::Retriever(CodebaseIndex* index)
    : index_(index) {}

std::string Retriever::retrieve(
    const std::string& query,
    const std::string& active_file,
    const AgentMemory& memory
) {
    std::stringstream ss;

    ss << "RELEVANT CODE CONTEXT\n\n";

    if (!index_) {
        ss << "No codebase index available.\n";
        return ss.str();
    }

    ss << index_->symbols().build_context(query, 20);
    ss << "\n";

    ss << index_->references().build_context(query, 10);
ss << "\n";

ss << index_->calls().build_context(query, 10);
ss << "\n";

    if (!active_file.empty()) {
        std::string content = index_->get_file_content(active_file);

        if (!content.empty()) {
            ss << "ACTIVE FILE:\n";
            ss << "[file: " << normalize_path(active_file) << "]\n";
            ss << content.substr(0, 16000) << "\n\n";
        }
    }

    ss << "SEMANTIC SEARCH RESULTS\n";
ss << semantic_search(query);
ss << "\n";

    ss << "TEXT SEARCH RESULTS\n";
    ss << search(query);

    return ss.str();
}

std::string Retriever::search_references(const std::string& query) {
    if (!index_) {
        return "No codebase index available.\n";
    }

    return index_->references().build_context(query, 30);
}

std::string Retriever::search(const std::string& query) {
    std::stringstream ss;

    if (!index_) {
        return "No codebase index available.\n";
    }

    auto results = index_->search_files(query, 5);

    for (const auto& file : results) {
        ss << "\n[file: " << normalize_path(file.path) << "]\n";
        ss << file.content.substr(0, 12000) << "\n";
    }

    return ss.str();
}

std::string Retriever::search_symbols(const std::string& query) {
    if (!index_) {
        return "No codebase index available.\n";
    }

    return index_->symbols().build_context(query, 30);
}

bool Retriever::contains_case_insensitive(
    const std::string& text,
    const std::string& query
) const {
    std::string a = text;
    std::string b = query;

    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);

    return a.find(b) != std::string::npos;
}

std::string Retriever::search_calls(const std::string& query) {
    if (!index_) {
        return "No codebase index available.\n";
    }

    return index_->calls().build_context(query, 30);
}

std::string Retriever::analyze_impact(const std::string& query) {
    std::stringstream ss;

    if (!index_) {
        return "No codebase index available.\n";
    }

    ss << "IMPACT ANALYSIS\n\n";

    ss << index_->symbols().build_context(query, 10);
    ss << "\n";

    ss << index_->references().build_context(query, 20);
    ss << "\n";

    ss << index_->calls().build_context(query, 20);
    ss << "\n";

    ss << "RELATED TEXT SEARCH\n";
    ss << search(query);

    return ss.str();
}

std::string Retriever::semantic_search(const std::string& query) {
    std::stringstream ss;

    if (!index_) {
        return "No codebase index available.\n";
    }

    auto results = index_->semantic_search(query, 5);

    if (results.empty()) {
        ss << "No semantic results available.\n";
        return ss.str();
    }

    for (const auto& result : results) {
        ss << "\n[file: " << result.file_path << "] ";
        ss << "score=" << result.score << "\n";
        ss << result.content.substr(0, 6000) << "\n";
    }

    return ss.str();
}