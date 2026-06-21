#include "tree_sitter_cpp_parser.h"

#include <tree_sitter/api.h>

#include <string>
#include <vector>
#include <sstream>

extern "C" const TSLanguage *tree_sitter_cpp();

static std::string node_type(TSNode node) {
    const char* type = ts_node_type(node);
    return type ? std::string(type) : "";
}

static bool is_interesting_node(const std::string& type) {
    return
        type == "function_definition" ||
        type == "class_specifier" ||
        type == "struct_specifier";
}

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

static std::string first_line(const std::string& text) {
    size_t pos = text.find('\n');
    if (pos == std::string::npos) return text;
    return text.substr(0, pos);
}

static std::string find_name_from_declarator(
    const std::string& source,
    TSNode node
) {
    std::string type = node_type(node);

    if (
        type == "identifier" ||
        type == "field_identifier" ||
        type == "type_identifier" ||
        type == "qualified_identifier" ||
        type == "operator_name" ||
        type == "destructor_name"
    ) {
        return text_for_node(source, node);
    }

    uint32_t count = ts_node_child_count(node);

    for (uint32_t i = 0; i < count; i++) {
        TSNode child = ts_node_child(node, i);
        std::string found = find_name_from_declarator(source, child);

        if (!found.empty()) {
            return found;
        }
    }

    return "";
}

static std::string node_text_name(
    const std::string& source,
    TSNode node
) {
    std::string type = node_type(node);

    if (type == "class_specifier" || type == "struct_specifier") {
        uint32_t count = ts_node_child_count(node);

        for (uint32_t i = 0; i < count; i++) {
            TSNode child = ts_node_child(node, i);
            std::string child_type = node_type(child);

            if (child_type == "type_identifier") {
                return text_for_node(source, child);
            }
        }
    }

    if (type == "function_definition") {
        TSNode declarator = ts_node_child_by_field_name(
            node,
            "declarator",
            10
        );

        if (!ts_node_is_null(declarator)) {
            return find_name_from_declarator(source, declarator);
        }
    }

    return find_name_from_declarator(source, node);
}

static std::string kind_for_type(const std::string& type) {
    if (type == "class_specifier") return "class";
    if (type == "struct_specifier") return "struct";
    if (type == "function_definition") return "function";
    return "unknown";
}

static void collect_nodes(
    const std::string& file_path,
    const std::string& source,
    TSNode node,
    std::vector<AstNode>& out
) {
    std::string type = node_type(node);

    if (is_interesting_node(type)) {
        std::string text = text_for_node(source, node);
        std::string name = node_text_name(source, node);

        if (!name.empty()) {
            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);

            AstNode ast;
            ast.kind = kind_for_type(type);

if (ast.kind == "unknown") {
    return;
}
            ast.name = name;
            ast.file_path = file_path;
            ast.line_start = static_cast<int>(start.row) + 1;
            ast.line_end = static_cast<int>(end.row) + 1;
            ast.signature = first_line(text);
            ast.text = text;

            out.push_back(ast);
        }
    }

    uint32_t count = ts_node_child_count(node);
    for (uint32_t i = 0; i < count; i++) {
        collect_nodes(
            file_path,
            source,
            ts_node_child(node, i),
            out
        );
    }
}

std::vector<AstNode> TreeSitterCppParser::parse_file(
    const std::string& file_path,
    const std::string& content
) {
    std::vector<AstNode> nodes;

    TSParser* parser = ts_parser_new();
    if (!parser) {
        return nodes;
    }

    const TSLanguage* language = tree_sitter_cpp();
    if (!language) {
        ts_parser_delete(parser);
        return nodes;
    }

    if (!ts_parser_set_language(parser, language)) {
        ts_parser_delete(parser);
        return nodes;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        content.c_str(),
        static_cast<uint32_t>(content.size())
    );

    if (!tree) {
        ts_parser_delete(parser);
        return nodes;
    }

    TSNode root = ts_tree_root_node(tree);

    collect_nodes(
        file_path,
        content,
        root,
        nodes
    );

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return nodes;
}