#include "command_runner.h"

#include <cstdio>
#include <array>
#include <string>
#include <algorithm>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

bool CommandRunner::is_safe_command(const std::string& command) const {
    std::string lower = command;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    const std::string blocked[] = {
        " del ",
        " rmdir ",
        " remove-item",
        " rm ",
        " format ",
        " shutdown",
        " taskkill",
        " reg delete",
        " powershell -enc",
        " curl ",
        " wget "
    };

    for (const auto& bad : blocked) {
        if (lower.find(bad) != std::string::npos) {
            return false;
        }
    }

    return true;
}

std::string CommandRunner::run(
    const std::string& command,
    const std::string& working_directory
) {
    if (command.empty()) {
        return "Command is empty.";
    }

    if (!is_safe_command(command)) {
        return "Blocked unsafe command: " + command;
    }

    std::string full_command;

#ifdef _WIN32
    full_command = "cd /d \"" + working_directory + "\" && " + command + " 2>&1";
#else
    full_command = "cd \"" + working_directory + "\" && " + command + " 2>&1";
#endif

    std::array<char, 512> buffer;
    std::string output;

    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) {
        return "Failed to run command.";
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();

        if (output.size() > 20000) {
            output += "\n[Output truncated]\n";
            break;
        }
    }

    int exit_code = pclose(pipe);

    output += "\n[exit_code=" + std::to_string(exit_code) + "]\n";
    return output;
}