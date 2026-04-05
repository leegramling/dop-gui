#pragma once

#include <cstdint>
#include <string>
#include <vector>

class App;

/**
 * @brief Scheduled action extracted from a script.
 */
struct TimedScriptAction
{
    /**
     * @brief Supported scheduled action kinds.
     */
    enum class Kind
    {
        command,
        query,
    };

    /**
     * @brief The action kind.
     */
    Kind kind;
    /**
     * @brief Command or query text payload.
     */
    std::string text;
    /**
     * @brief Offset in milliseconds from the start of playback.
     */
    std::uint64_t offsetMs = 0;
};

/**
 * @brief Parsed script request data.
 */
struct ScriptRequest
{
    std::vector<std::string> commands;
    std::vector<std::string> queries;
    std::vector<TimedScriptAction> actions;
};

/**
 * @brief Parse and execute JSON5-authored command/query scripts.
 */
class ScriptRunner
{
public:
    /**
     * @brief Parse a script file into request data.
     * @param filename Script file path.
     * @return Parsed script request.
     */
    ScriptRequest parse(const std::string& filename) const;
    /**
     * @brief Execute a script request immediately.
     * @param app Application to run the script against.
     * @param request Parsed script request.
     * @return Process-style exit code.
     */
    int execute(App& app, const ScriptRequest& request) const;
    /**
     * @brief Convert a script request into timed playback actions.
     * @param request Parsed script request.
     * @return Scheduled playback actions.
     */
    std::vector<TimedScriptAction> createTimedActions(const ScriptRequest& request) const;

private:
    static std::string readFile(const std::string& filename);
    static std::string stripComments(const std::string& text);
    static std::vector<std::string> parseStringArray(const std::string& text, const std::string& key);
};
