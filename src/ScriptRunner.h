#pragma once

#include <string>
#include <vector>

class App;

struct ScriptRequest
{
    std::vector<std::string> commands;
    std::vector<std::string> queries;
};

class ScriptRunner
{
public:
    ScriptRequest parse(const std::string& filename) const;
    int execute(App& app, const ScriptRequest& request) const;

private:
    static std::string readFile(const std::string& filename);
    static std::string stripComments(const std::string& text);
    static std::vector<std::string> parseStringArray(const std::string& text, const std::string& key);
};
