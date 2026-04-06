#include "AppState.h"

#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace
{
std::string defaultObjectColorHex(std::string_view kind);

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

std::optional<std::string> extractOptionalObjectBlock(const std::string& text, const std::string& key)
{
    const auto keyPos = text.find(key);
    if (keyPos == std::string::npos) return std::nullopt;
    const auto openPos = text.find('{', keyPos);
    if (openPos == std::string::npos) return std::nullopt;
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

std::optional<double> parseOptionalDoubleField(const std::string& block, const std::string& key)
{
    const std::regex pattern(key + R"(\s*:\s*([-+]?[0-9]*\.?[0-9]+))");
    std::smatch match;
    if (!std::regex_search(block, match, pattern)) return std::nullopt;
    return std::stod(match[1].str());
}

std::optional<std::string> parseOptionalStringFieldTopLevel(const std::string& block, const std::string& key)
{
    int braceDepth = 0;
    int bracketDepth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (std::size_t i = 0; i < block.size(); ++i)
    {
        const char ch = block[i];
        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        else if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;
        if (inSingleQuote || inDoubleQuote) continue;
        if (ch == '{') ++braceDepth;
        else if (ch == '}') --braceDepth;
        else if (ch == '[') ++bracketDepth;
        else if (ch == ']') --bracketDepth;

        if (braceDepth == 1 && bracketDepth == 0 && std::isalpha(static_cast<unsigned char>(ch)))
        {
            std::size_t keyStart = i;
            while (i < block.size() &&
                   (std::isalnum(static_cast<unsigned char>(block[i])) || block[i] == '_' || block[i] == '-'))
            {
                ++i;
            }
            const auto parsedKey = block.substr(keyStart, i - keyStart);
            if (parsedKey != key) continue;
            while (i < block.size() && std::isspace(static_cast<unsigned char>(block[i]))) ++i;
            if (i >= block.size() || block[i] != ':') continue;
            ++i;
            while (i < block.size() && std::isspace(static_cast<unsigned char>(block[i]))) ++i;
            if (i >= block.size()) return std::nullopt;
            if (block[i] != '"' && block[i] != '\'') return std::nullopt;
            const char quote = block[i++];
            const auto valueStart = i;
            while (i < block.size() && block[i] != quote) ++i;
            if (i >= block.size()) return std::nullopt;
            return block.substr(valueStart, i - valueStart);
        }
    }

    return std::nullopt;
}

std::optional<double> parseOptionalDoubleFieldTopLevel(const std::string& block, const std::string& key)
{
    int braceDepth = 0;
    int bracketDepth = 0;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (std::size_t i = 0; i < block.size(); ++i)
    {
        const char ch = block[i];
        if (ch == '\'' && !inDoubleQuote) inSingleQuote = !inSingleQuote;
        else if (ch == '"' && !inSingleQuote) inDoubleQuote = !inDoubleQuote;
        if (inSingleQuote || inDoubleQuote) continue;
        if (ch == '{') ++braceDepth;
        else if (ch == '}') --braceDepth;
        else if (ch == '[') ++bracketDepth;
        else if (ch == ']') --bracketDepth;

        if (braceDepth == 1 && bracketDepth == 0 && std::isalpha(static_cast<unsigned char>(ch)))
        {
            std::size_t keyStart = i;
            while (i < block.size() &&
                   (std::isalnum(static_cast<unsigned char>(block[i])) || block[i] == '_' || block[i] == '-'))
            {
                ++i;
            }
            const auto parsedKey = block.substr(keyStart, i - keyStart);
            if (parsedKey != key) continue;
            while (i < block.size() && std::isspace(static_cast<unsigned char>(block[i]))) ++i;
            if (i >= block.size() || block[i] != ':') continue;
            ++i;
            while (i < block.size() && std::isspace(static_cast<unsigned char>(block[i]))) ++i;
            const auto valueStart = i;
            while (i < block.size() &&
                   (std::isdigit(static_cast<unsigned char>(block[i])) || block[i] == '.' || block[i] == '-' || block[i] == '+'))
            {
                ++i;
            }
            if (i == valueStart) return std::nullopt;
            return std::stod(block.substr(valueStart, i - valueStart));
        }
    }

    return std::nullopt;
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
        const auto kind = parseStringField(objectBlock, "kind");
        const auto colorHex = parseOptionalStringField(objectBlock, "color").value_or(defaultObjectColorHex(kind));
        state.scene.objects.push_back(SceneObjectState{
            .id = parseStringField(objectBlock, "id"),
            .kind = kind,
            .vertexCount = parseUIntField(objectBlock, "vertexCount"),
            .position = parseVec3Field(objectBlock, "position"),
            .rotation = parseOptionalVec3Field(objectBlock, "rotation").value_or(vsg::dvec3{0.0, 0.0, 0.0}),
            .scale = parseVec3Field(objectBlock, "scale"),
            .colorHex = colorHex,
        });
        auto& object = state.scene.objects.back();
        if (!tryParseHexColor(object.colorHex, object.color))
        {
            object.colorHex = defaultObjectColorHex(object.kind);
            tryParseHexColor(object.colorHex, object.color);
        }
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

UiLayoutRectState parseLayoutRectBlock(const std::string& block)
{
    UiLayoutRectState rect;
    rect.x = parseOptionalDoubleField(block, "x").value_or(0.0);
    rect.y = parseOptionalDoubleField(block, "y").value_or(0.0);
    rect.width = parseOptionalDoubleField(block, "w").value_or(0.0);
    rect.height = parseOptionalDoubleField(block, "h").value_or(0.0);
    rect.enabled = true;
    return rect;
}

UiFlexNodeState parseFlexNodeBlock(const std::string& block)
{
    UiFlexNodeState node;
    node.type = parseOptionalStringFieldTopLevel(block, "type").value_or("");
    node.slot = parseOptionalStringFieldTopLevel(block, "slot").value_or("");
    node.widget = parseOptionalStringFieldTopLevel(block, "widget").value_or("");
    node.labelFor = parseOptionalStringFieldTopLevel(block, "labelFor").value_or("");
    if (node.type.empty() && (!node.slot.empty() || !node.widget.empty() || !node.labelFor.empty())) node.type = "slot";
    node.gap = parseOptionalDoubleFieldTopLevel(block, "gap").value_or(0.0);
    node.width = parseOptionalDoubleFieldTopLevel(block, "width");
    node.height = parseOptionalDoubleFieldTopLevel(block, "height");
    node.flex = parseOptionalDoubleFieldTopLevel(block, "flex");

    if (block.find("children") != std::string::npos)
    {
        for (const auto& childBlock : extractObjectEntriesForKey(block, "children"))
        {
            node.children.push_back(parseFlexNodeBlock(childBlock));
        }
    }

    return node;
}

UiPanelState parsePanelBlock(const std::string& block)
{
    UiPanelState panel{.label = parseStringField(block, "label")};
    panel.open = parseOptionalBoolField(block, "open").value_or(true);
    panel.closable = parseOptionalBoolField(block, "closable").value_or(true);
    if (block.find("flags") != std::string::npos) panel.flags = parseStringArray(extractArrayBlock(block, "flags"));
    if (auto layoutBlock = extractOptionalObjectBlock(block, "layout")) panel.layout = parseLayoutRectBlock(*layoutBlock);
    if (auto flexLayoutBlock = extractOptionalObjectBlock(block, "flexLayout")) panel.flexLayout = parseFlexNodeBlock(*flexLayoutBlock);

    if (block.find("widgets") != std::string::npos)
    {
        for (const auto& widgetBlock : extractObjectEntriesForKey(block, "widgets"))
        {
            UiWidgetSpecState widget;
            widget.id = parseStringField(widgetBlock, "id");
            widget.slotId = parseOptionalStringField(widgetBlock, "slotId").value_or("");
            widget.labelSlot = parseOptionalStringField(widgetBlock, "labelSlot").value_or("");
            widget.type = parseStringField(widgetBlock, "type");
            widget.label = parseStringField(widgetBlock, "label");
            widget.bind = parseOptionalStringField(widgetBlock, "bind").value_or("");
            widget.arg = parseOptionalStringField(widgetBlock, "arg").value_or("");
            widget.onClick = parseOptionalStringField(widgetBlock, "onClick").value_or("");
            widget.onChange = parseOptionalStringField(widgetBlock, "onChange").value_or("");
            widget.unit = parseOptionalStringField(widgetBlock, "unit").value_or("");
            widget.precision = parseOptionalIntField(widgetBlock, "precision").value_or(3);
            if (widgetBlock.find("options") != std::string::npos) widget.options = parseStringArray(extractArrayBlock(widgetBlock, "options"));
            if (widgetBlock.find("columns") != std::string::npos) widget.columns = parseStringArray(extractArrayBlock(widgetBlock, "columns"));
            if (auto layoutBlock = extractOptionalObjectBlock(widgetBlock, "layout")) widget.layout = parseLayoutRectBlock(*layoutBlock);
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
std::string defaultObjectColorHex(std::string_view kind)
{
    if (kind == "triangle") return "#F24D40";
    if (kind == "rectangle") return "#33B2F2";
    if (kind == "tristrip") return "#33D973";
    if (kind == "cube") return "#F2CC40";
    if (kind == "sphere") return "#4DD9A6";
    if (kind == "torus") return "#CC66F2";
    if (kind == "pyramid") return "#F28C33";
    return "#D9D9D9";
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

UiPanelState* findPanel(UiState& ui, const std::string& panelId)
{
    for (auto& panel : ui.layout.panels)
    {
        std::string candidateId = panel.label;
        for (auto& ch : candidateId)
        {
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
            else if (ch == ' ' || ch == '.') ch = '-';
        }
        candidateId = "panel-" + candidateId;
        if (candidateId == panelId) return &panel;
    }
    return nullptr;
}

const UiPanelState* findPanel(const UiState& ui, const std::string& panelId)
{
    for (const auto& panel : ui.layout.panels)
    {
        std::string candidateId = panel.label;
        for (auto& ch : candidateId)
        {
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
            else if (ch == ' ' || ch == '.') ch = '-';
        }
        candidateId = "panel-" + candidateId;
        if (candidateId == panelId) return &panel;
    }
    return nullptr;
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

WidgetState* findWidget(UiState& ui, const std::string& panelId, const std::string& widgetId)
{
    for (auto& widget : ui.registry)
    {
        if (widget.panelId == panelId && widget.widgetId == widgetId) return &widget;
    }
    return nullptr;
}

const WidgetState* findWidget(const UiState& ui, const std::string& panelId, const std::string& widgetId)
{
    for (const auto& widget : ui.registry)
    {
        if (widget.panelId == panelId && widget.widgetId == widgetId) return &widget;
    }
    return nullptr;
}

std::optional<std::pair<std::string, std::string>> resolveLegacyWidgetAlias(std::string_view label)
{
    if (label == "panel-fps") return std::pair<std::string, std::string>{"panel-scene-info", "fps"};
    if (label == "panel-object-count") return std::pair<std::string, std::string>{"panel-scene-info", "object-count"};
    if (label == "panel-display-grid") return std::pair<std::string, std::string>{"panel-scene-info", "display-grid"};
    if (label == "panel-bgcolor") return std::pair<std::string, std::string>{"panel-scene-info", "background-color"};
    if (label == "panel-scene-select") return std::pair<std::string, std::string>{"panel-scene-info", "scene-select"};
    if (label == "panel-theme-dark") return std::pair<std::string, std::string>{"panel-scene-info", "theme-dark"};
    if (label == "panel-theme-light") return std::pair<std::string, std::string>{"panel-scene-info", "theme-light"};
    if (label == "panel-scene-summary-open") return std::pair<std::string, std::string>{"panel-scene-info", "scene-summary-open"};
    if (label == "panel-scene-selected-object") return std::pair<std::string, std::string>{"panel-scene-info", "selected-object"};
    if (label == "panel-scene-table") return std::pair<std::string, std::string>{"panel-scene-info", "scene-table"};

    if (label == "panel-selected-object") return std::pair<std::string, std::string>{"panel-properties", "selected-object"};
    if (label == "input-properties-position-x") return std::pair<std::string, std::string>{"panel-properties", "position-x"};
    if (label == "input-properties-position-y") return std::pair<std::string, std::string>{"panel-properties", "position-y"};
    if (label == "input-properties-position-z") return std::pair<std::string, std::string>{"panel-properties", "position-z"};
    if (label == "input-properties-rotation-x") return std::pair<std::string, std::string>{"panel-properties", "rotation-x"};
    if (label == "input-properties-rotation-y") return std::pair<std::string, std::string>{"panel-properties", "rotation-y"};
    if (label == "input-properties-rotation-z") return std::pair<std::string, std::string>{"panel-properties", "rotation-z"};
    if (label == "input-properties-scale-x") return std::pair<std::string, std::string>{"panel-properties", "scale-x"};
    if (label == "input-properties-scale-y") return std::pair<std::string, std::string>{"panel-properties", "scale-y"};
    if (label == "input-properties-scale-z") return std::pair<std::string, std::string>{"panel-properties", "scale-z"};

    return std::nullopt;
}

bool matchesWidgetReference(std::string_view panelId, std::string_view widgetId, std::string_view reference)
{
    if (reference == widgetId) return true;
    if (auto alias = resolveLegacyWidgetAlias(reference))
    {
        return alias->first == panelId && alias->second == widgetId;
    }
    return false;
}

const UiLayoutSlotState* findLayoutSlot(const UiState& ui, const std::string& panelId, const std::string& slotId)
{
    for (const auto& slot : ui.layoutSlots)
    {
        if (slot.panelId == panelId && slot.slotId == slotId) return &slot;
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

UiTestAction* findPendingUiAction(UiState& ui, std::string_view panelId, std::string_view widgetId, const std::string& kind)
{
    for (auto& action : ui.pendingActions)
    {
        if (action.kind != kind) continue;
        if (!action.panelId.empty() || !action.widgetId.empty())
        {
            if (action.panelId == panelId && action.widgetId == widgetId) return &action;
            continue;
        }
        if (matchesWidgetReference(panelId, widgetId, action.label))
        {
            return &action;
        }
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
