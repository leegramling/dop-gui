#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

class App;

/**
 * @brief Path-based command request with optional numeric and raw arguments.
 */
struct CommandByPath
{
    std::string commandName;
    std::string canonicalPath;
    std::string rawArg;
    std::vector<double> numericArgs;
};

/**
 * @brief Tagged command request data.
 */
using CommandRequest = std::variant<CommandByPath>;

/**
 * @brief Successful command execution details.
 */
struct CommandSuccess
{
    std::string commandName;
    bool continueRunning = false;
    int exitCode = 0;
    std::string jsonValue = "{}";
};

/**
 * @brief Failed command execution details.
 */
struct CommandError
{
    std::string commandName;
    bool continueRunning = false;
    int exitCode = 1;
    std::string message;
};

/**
 * @brief Tagged command result data.
 */
using CommandResult = std::variant<CommandSuccess, CommandError>;

/**
 * @brief Parse a command name into typed command request data.
 * @param name Raw command string from CLI or script input.
 * @return Parsed command request when the input is non-empty.
 */
std::optional<CommandRequest> parseCommandRequest(const std::string& name);
/**
 * @brief Report whether a command requires the application to be initialized first.
 * @param request Parsed command request.
 * @return True when the command requires initialized runtime resources.
 */
bool requiresInitializedApp(const CommandRequest& request);
/**
 * @brief Execute a command request against the application.
 * @param app Application to mutate or query through command handlers.
 * @param request Parsed command request.
 * @return Tagged command result.
 */
CommandResult executeCommand(App& app, const CommandRequest& request);
/**
 * @brief Serialize a command result as JSON-compatible text.
 * @param result Tagged command result.
 * @return JSON-compatible serialized result text.
 */
std::string serializeCommandResult(const CommandResult& result);
/**
 * @brief Return the list of supported command names.
 * @return Supported command surface names.
 */
std::vector<std::string> commandNames();
