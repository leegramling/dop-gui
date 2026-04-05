#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

class App;

struct CommandByPath
{
    std::string commandName;
    std::string canonicalPath;
    std::vector<double> numericArgs;
};

using CommandRequest = std::variant<CommandByPath>;

struct CommandSuccess
{
    std::string commandName;
    bool continueRunning = false;
    int exitCode = 0;
    std::string jsonValue = "{}";
};

struct CommandError
{
    std::string commandName;
    bool continueRunning = false;
    int exitCode = 1;
    std::string message;
};

using CommandResult = std::variant<CommandSuccess, CommandError>;

std::optional<CommandRequest> parseCommandRequest(const std::string& name);
bool requiresInitializedApp(const CommandRequest& request);
CommandResult executeCommand(App& app, const CommandRequest& request);
std::string serializeCommandResult(const CommandResult& result);
std::vector<std::string> commandNames();
