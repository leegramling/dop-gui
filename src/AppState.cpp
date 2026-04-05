#include "AppState.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace
{
std::string readFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Failed to open scene file: " + filename);

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string stripComments(const std::string& text)
{
    std::string result;
    result.reserve(text.size());

    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const char ch = text[i];
        const char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

        if (!inSingleQuote && !inDoubleQuote && ch == '/' && next == '/')
        {
            while (i < text.size() && text[i] != '\n') ++i;
            if (i < text.size()) result.push_back(text[i]);
            continue;
        }

        if (!inSingleQuote && !inDoubleQuote && ch == '/' && next == '*')
        {
            i += 2;
            while (i + 1 < text.size() && !(text[i] == '*' && text[i + 1] == '/')) ++i;
            ++i;
            continue;
        }

        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;

        result.push_back(ch);
    }

    return result;
}

std::size_t findKey(const std::string& text, const std::string& key)
{
    const auto position = text.find(key);
    if (position == std::string::npos) throw std::runtime_error("Missing scene key: " + key);
    return position;
}

std::string extractBalanced(const std::string& text, std::size_t start, char openChar, char closeChar)
{
    if (start >= text.size() || text[start] != openChar)
    {
        throw std::runtime_error("Invalid balanced block extraction.");
    }

    int depth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (std::size_t i = start; i < text.size(); ++i)
    {
        const char ch = text[i];

        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;
        if (inSingleQuote || inDoubleQuote) continue;

        if (ch == openChar) ++depth;
        if (ch == closeChar)
        {
            --depth;
            if (depth == 0) return text.substr(start, i - start + 1);
        }
    }

    throw std::runtime_error("Unbalanced scene block.");
}

std::string extractObjectBlock(const std::string& text, const std::string& key)
{
    const auto keyPos = findKey(text, key);
    const auto openPos = text.find('{', keyPos);
    if (openPos == std::string::npos) throw std::runtime_error("Missing object block for key: " + key);
    return extractBalanced(text, openPos, '{', '}');
}

std::string extractArrayBlock(const std::string& text, const std::string& key)
{
    const auto keyPos = findKey(text, key);
    const auto openPos = text.find('[', keyPos);
    if (openPos == std::string::npos) throw std::runtime_error("Missing array block for key: " + key);
    return extractBalanced(text, openPos, '[', ']');
}

std::string parseStringField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"__JSON5__(\s*:\s*(?:"([^"]*)"|'([^']*)'))__JSON5__");
    std::smatch match;
    if (!std::regex_search(block, match, pattern))
    {
        throw std::runtime_error("Missing string field: " + key);
    }
    return match[1].matched ? match[1].str() : match[2].str();
}

uint32_t parseUIntField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"(\s*:\s*([0-9]+))");
    std::smatch match;
    if (!std::regex_search(block, match, pattern))
    {
        throw std::runtime_error("Missing unsigned integer field: " + key);
    }
    return static_cast<uint32_t>(std::stoul(match[1].str()));
}

vsg::dvec3 parseVec3Field(const std::string& block, const std::string& key)
{
    const std::regex pattern(
        key +
        R"(\s*:\s*\[\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*\])");
    std::smatch match;
    if (!std::regex_search(block, match, pattern))
    {
        throw std::runtime_error("Missing vec3 field: " + key);
    }

    return {
        std::stod(match[1].str()),
        std::stod(match[2].str()),
        std::stod(match[3].str()),
    };
}

std::vector<std::string> extractObjectEntries(const std::string& arrayBlock)
{
    std::vector<std::string> objects;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    int depth = 0;
    std::size_t objectStart = std::string::npos;

    for (std::size_t i = 0; i < arrayBlock.size(); ++i)
    {
        const char ch = arrayBlock[i];
        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;
        if (inSingleQuote || inDoubleQuote) continue;

        if (ch == '{')
        {
            if (depth == 0) objectStart = i;
            ++depth;
        }
        else if (ch == '}')
        {
            --depth;
            if (depth == 0 && objectStart != std::string::npos)
            {
                objects.push_back(arrayBlock.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        }
    }

    return objects;
}

AppState loadSceneStateFromFile(const std::string& filename)
{
    const auto text = stripComments(readFile(filename));
    const auto cameraBlock = extractObjectBlock(text, "cameraPose");
    const auto objectsBlock = extractArrayBlock(text, "objects");

    AppState state;
    state.view.cameraPose.eye = parseVec3Field(cameraBlock, "eye");
    state.view.cameraPose.center = parseVec3Field(cameraBlock, "center");
    state.view.cameraPose.up = parseVec3Field(cameraBlock, "up");

    for (const auto& objectBlock : extractObjectEntries(objectsBlock))
    {
        state.scene.objects.push_back(SceneObjectState{
            .id = parseStringField(objectBlock, "id"),
            .kind = parseStringField(objectBlock, "kind"),
            .vertexCount = parseUIntField(objectBlock, "vertexCount"),
            .position = parseVec3Field(objectBlock, "position"),
            .scale = parseVec3Field(objectBlock, "scale"),
        });
    }

    return state;
}
}

AppState loadAppStateFromSceneFile(const std::string& filename)
{
    return loadSceneStateFromFile(filename);
}

AppState createBootstrapAppState()
{
    return loadSceneStateFromFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/bootstrap_scene.json5");
}

const SceneObjectState* findSceneObject(const SceneState& scene, const std::string& id)
{
    for (const auto& object : scene.objects)
    {
        if (object.id == id) return &object;
    }
    return nullptr;
}

SceneObjectState* findSceneObject(SceneState& scene, const std::string& id)
{
    for (auto& object : scene.objects)
    {
        if (object.id == id) return &object;
    }
    return nullptr;
}
