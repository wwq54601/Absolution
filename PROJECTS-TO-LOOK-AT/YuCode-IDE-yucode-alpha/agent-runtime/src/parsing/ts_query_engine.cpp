#include "ts_query_engine.h"

#include <tree_sitter/api.h>

#include <sstream>
#include <string>
#include <vector>

extern "C" const TSLanguage *tree_sitter_cpp();

static std::string text_for_node(
    const std::string& source,
    TSNode node
) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);

    if (start >= source.size() || end > source.size() || end <= start) {
        return "";
    }

    return source.substr(start, end - start);
}

static std::string first_identifier_inside(
    const std::string& source,
    TSNode node
) {
    const char* type_c = ts_node_type(node);
    std::string type = type_c ? type_c : "";

    if (
        type == "identifier" ||
        type == "field_identifier" ||
        type == "type_identifier" ||
        type == "qualified_identifier"
    ) {
        return text_for_node(source, node);
    }

    uint32_t count = ts_node_child_count(node);

    for (uint32_t i = 0; i < count; i++) {
        std::string found = first_identifier_inside(
            source,
            ts_node_child(node, i)
        );

        if (!found.empty()) {
            return found;
        }
    }

    return "";
}

std::vector<QueryMatch> TSQueryEngine::find_functions(
    const std::string& file_path,
    const std::string& source
) {
    return run_query(
        file_path,
        source,
        "(function_definition) @match",
        "function"
    );
}

std::vector<QueryMatch> TSQueryEngine::find_classes(
    const std::string& file_path,
    const std::string& source
) {
    std::vector<QueryMatch> results;

    auto classes = run_query(
        file_path,
        source,
        "(class_specifier) @match",
        "class"
    );

    auto structs = run_query(
        file_path,
        source,
        "(struct_specifier) @match",
        "struct"
    );

    results.insert(results.end(), classes.begin(), classes.end());
    results.insert(results.end(), structs.begin(), structs.end());

    return results;
}

std::vector<QueryMatch> TSQueryEngine::find_calls(
    const std::string& file_path,
    const std::string& source
) {
    return run_query(
        file_path,
        source,
        "(call_expression) @match",
        "call"
    );
}

std::vector<QueryMatch> TSQueryEngine::find_includes(
    const std::string& file_path,
    const std::string& source
) {
    return run_query(
        file_path,
        source,
        "(preproc_include) @match",
        "include"
    );
}

std::vector<QueryMatch> TSQueryEngine::run_query(
    const std::string& file_path,
    const std::string& source,
    const std::string& query_text,
    const std::string& kind
) {
    std::vector<QueryMatch> results;

    TSParser* parser = ts_parser_new();
    if (!parser) return results;

    const TSLanguage* language = tree_sitter_cpp();
    if (!language) {
        ts_parser_delete(parser);
        return results;
    }

    if (!ts_parser_set_language(parser, language)) {
        ts_parser_delete(parser);
        return results;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source.c_str(),
        static_cast<uint32_t>(source.size())
    );

    if (!tree) {
        ts_parser_delete(parser);
        return results;
    }

    uint32_t error_offset = 0;
    TSQueryError error_type;

    TSQuery* query = ts_query_new(
        language,
        query_text.c_str(),
        static_cast<uint32_t>(query_text.size()),
        &error_offset,
        &error_type
    );

    if (!query) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return results;
    }

    TSQueryCursor* cursor = ts_query_cursor_new();
    TSNode root = ts_tree_root_node(tree);

    ts_query_cursor_exec(cursor, query, root);

    TSQueryMatch match;

    while (ts_query_cursor_next_match(cursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;

            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);

            QueryMatch item;
            item.kind = kind;
            item.file_path = file_path;
            item.line_start = static_cast<int>(start.row) + 1;
            item.line_end = static_cast<int>(end.row) + 1;
            item.text = text_for_node(source, node);
            item.name = first_identifier_inside(source, node);

            if (!item.name.empty()) {
                results.push_back(item);
            }
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return results;
}