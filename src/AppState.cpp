#include "AppState.h"

#include <fstream>
#include <iomanip>
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

std::optional<std::string> parseOptionalStringField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"__JSON5__(\s*:\s*(?:"([^"]*)"|'([^']*)'))__JSON5__");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;
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

std::optional<int> parseOptionalIntField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"(\s*:\s*([-+]?[0-9]+))");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;
    return std::stoi(match[1].str());
}

std::optional<bool> parseOptionalBoolField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"(\s*:\s*(true|false))");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;
    return match[1].str() == "true";
}

std::optional<vsg::dvec3> parseOptionalVec3Field(const std::string& block, const std::string& key)
{
    const std::regex pattern(
        key +
        R"(\s*:\s*\[\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*,\s*([-+]?[0-9]*\.?[0-9]+)\s*\])");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;

    return vsg::dvec3{
        std::stod(match[1].str()),
        std::stod(match[2].str()),
        std::stod(match[3].str()),
    };
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
    if (filename.find("cubes") != std::string::npos) state.scene.name = "cubes";
    else if (filename.find("shapes") != std::string::npos) state.scene.name = "shapes";
    else state.scene.name = "bootstrap";
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
            .rotation = parseOptionalVec3Field(objectBlock, "rotation").value_or(vsg::dvec3{0.0, 0.0, 0.0}),
            .scale = parseVec3Field(objectBlock, "scale"),
        });
    }

    if (!state.scene.objects.empty()) state.scene.selectedObjectId = state.scene.objects.front().id;

    return state;
}

std::vector<std::string> parseStringArray(const std::string& block)
{
    std::vector<std::string> values;
    const std::regex pattern(R"__JSON5__(["']([^"']+)["'])__JSON5__");
    for (auto i = std::sregex_iterator(block.begin(), block.end(), pattern); i != std::sregex_iterator(); ++i)
    {
        values.push_back((*i)[1].str());
    }
    return values;
}

std::vector<std::string> extractObjectEntriesForKey(const std::string& text, const std::string& key)
{
    return extractObjectEntries(extractArrayBlock(text, key));
}

UiMenuState parseMenuBlock(const std::string& block)
{
    UiMenuState menu;
    menu.label = parseStringField(block, "label");

    for (const auto& itemBlock : extractObjectEntriesForKey(block, "items"))
    {
        menu.items.push_back(UiMenuItemState{
            .label = parseStringField(itemBlock, "label"),
            .command = parseStringField(itemBlock, "command"),
        });
    }

    return menu;
}

UiPanelState parsePanelBlock(const std::string& block)
{
    UiPanelState panel{.label = parseStringField(block, "label")};
    panel.open = parseOptionalBoolField(block, "open").value_or(true);
    panel.closable = parseOptionalBoolField(block, "closable").value_or(true);
    if (block.find("flags") != std::string::npos) panel.flags = parseStringArray(extractArrayBlock(block, "flags"));

    if (block.find("widgets") != std::string::npos)
    {
        for (const auto& widgetBlock : extractObjectEntriesForKey(block, "widgets"))
        {
            UiWidgetSpecState widget;
            widget.id = parseStringField(widgetBlock, "id");
            widget.type = parseStringField(widgetBlock, "type");
            widget.label = parseStringField(widgetBlock, "label");
            widget.bind = parseOptionalStringField(widgetBlock, "bind").value_or("");
            widget.arg = parseOptionalStringField(widgetBlock, "arg").value_or("");
            widget.onClick = parseOptionalStringField(widgetBlock, "onClick").value_or("");
            widget.onChange = parseOptionalStringField(widgetBlock, "onChange").value_or("");
            widget.unit = parseOptionalStringField(widgetBlock, "unit").value_or("");
            widget.precision = parseOptionalIntField(widgetBlock, "precision").value_or(3);
            if (widgetBlock.find("options") != std::string::npos) widget.options = parseStringArray(extractArrayBlock(widgetBlock, "options"));
            panel.widgets.push_back(std::move(widget));
        }
    }

    return panel;
}

int hexDigitValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    return -1;
}

float parseHexByte(const std::string& text, std::size_t offset)
{
    const int high = hexDigitValue(text[offset]);
    const int low = hexDigitValue(text[offset + 1]);
    if (high < 0 || low < 0) throw std::runtime_error("Invalid hex color component.");
    return static_cast<float>((high * 16) + low) / 255.0f;
}
}

AppState loadAppStateFromSceneFile(const std::string& filename)
{
    auto state = loadSceneStateFromFile(filename);
    state.view.backgroundColorHex = colorHexFromVec4(state.view.backgroundColor);
    return state;
}

UiLayoutState loadUiLayoutFromFile(const std::string& filename)
{
    const auto text = stripComments(readFile(filename));
    UiLayoutState layout;

    for (const auto& menuBlock : extractObjectEntriesForKey(text, "menus"))
    {
        layout.menus.push_back(parseMenuBlock(menuBlock));
    }

    for (const auto& panelBlock : extractObjectEntriesForKey(text, "panels"))
    {
        layout.panels.push_back(parsePanelBlock(panelBlock));
    }

    return layout;
}

AppState createBootstrapAppState()
{
    auto state = loadSceneStateFromFile(std::string(DOP_GUI_SOURCE_DIR) + "/scenes/bootstrap_scene.json5");
    state.view.backgroundColorHex = colorHexFromVec4(state.view.backgroundColor);
    return state;
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

WidgetState* findWidget(UiState& ui, const std::string& label)
{
    for (auto& widget : ui.registry)
    {
        if (widget.label == label) return &widget;
    }
    return nullptr;
}

const WidgetState* findWidget(const UiState& ui, const std::string& label)
{
    for (const auto& widget : ui.registry)
    {
        if (widget.label == label) return &widget;
    }
    return nullptr;
}

UiTestAction* findPendingUiAction(UiState& ui, const std::string& label, const std::string& kind)
{
    for (auto& action : ui.pendingActions)
    {
        if (action.label == label && action.kind == kind) return &action;
    }
    return nullptr;
}

std::string colorHexFromVec4(const vsg::vec4& color)
{
    auto clampByte = [](float component) -> int
    {
        if (component < 0.0f) component = 0.0f;
        if (component > 1.0f) component = 1.0f;
        return static_cast<int>(component * 255.0f + 0.5f);
    };

    std::ostringstream out;
    out << "#"
        << std::uppercase << std::hex << std::setfill('0')
        << std::setw(2) << clampByte(color.r)
        << std::setw(2) << clampByte(color.g)
        << std::setw(2) << clampByte(color.b);
    return out.str();
}

bool tryParseHexColor(const std::string& text, vsg::vec4& color)
{
    if (text.size() != 7 || text[0] != '#') return false;

    try
    {
        color.r = parseHexByte(text, 1);
        color.g = parseHexByte(text, 3);
        color.b = parseHexByte(text, 5);
        color.a = 1.0f;
        return true;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}
