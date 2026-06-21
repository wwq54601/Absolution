#pragma once
#include <memory>
#include <string>
#include <httplib.h>
#include "codebase/codebase_index.h"

#include "editing/change_set.h"

#include "session/session_store.h"

class Agent;

class YuCodeServer {
public:
    YuCodeServer();
    ~YuCodeServer();

    void start(int port);

private:
    std::unique_ptr<Agent> agent_;
    std::unique_ptr<CodebaseIndex> codebase_index_;
    ChangeSet change_set_;
    httplib::Server server_;

    SessionStore session_;

    void setup_routes();
    std::string model_status_json();

    std::string update_index_file_json(const std::string& body);
std::string remove_index_file_json(const std::string& body);

    std::string run_agent_json(const std::string& body);
    std::string list_changes_json();
    std::string apply_change_json(const std::string& body);
    std::string reject_change_json(const std::string& body);
    std::string reindex_json(const std::string& body);

    std::string test_llm_json();
    std::string test_embedding_json();
    std::string index_status_json();

    std::string embedded_runtime_status_json();

    std::string get_config_json();
std::string update_config_json(const std::string& body);

    std::string current_workspace_path_;
std::string current_build_command_;
std::string current_test_command_;

std::string clear_session_json();
std::string run_agent_stream_json(const std::string& body);

void run_agent_stream_sse(
    const std::string& body,
    httplib::DataSink& sink
);
};