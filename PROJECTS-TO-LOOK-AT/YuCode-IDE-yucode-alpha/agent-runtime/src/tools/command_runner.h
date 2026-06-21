#pragma once
#include <string>

class CommandRunner {
public:
    std::string run(
        const std::string& command,
        const std::string& working_directory
    );

private:
    bool is_safe_command(const std::string& command) const;
};