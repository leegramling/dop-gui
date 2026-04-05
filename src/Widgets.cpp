#include "Widgets.h"

#include <vsgImGui/imgui.h>

#include <array>

namespace
{
WidgetState& ensureWidget(UiState& uiState, std::string_view label, std::string_view type)
{
    if (auto* existing = findWidget(uiState, std::string(label)))
    {
        existing->type = std::string(type);
        return *existing;
    }

    uiState.registry.push_back(WidgetState{
        .label = std::string(label),
        .type = std::string(type),
    });
    return uiState.registry.back();
}

bool consumeClick(UiState& uiState, std::string_view label)
{
    if (auto* action = findPendingUiAction(uiState, std::string(label), "click"))
    {
        action->kind.clear();
        return true;
    }
    return false;
}

bool consumeBool(UiState& uiState, std::string_view label, bool& value)
{
    if (auto* action = findPendingUiAction(uiState, std::string(label), "set_bool"))
    {
        value = action->boolValue;
        action->kind.clear();
        return true;
    }
    return false;
}

bool consumeText(UiState& uiState, std::string_view label, std::string& value)
{
    if (auto* action = findPendingUiAction(uiState, std::string(label), "set_text"))
    {
        value = action->textValue;
        action->kind.clear();
        return true;
    }
    return false;
}

std::string imguiLabel(std::string_view id, std::string_view displayLabel)
{
    return std::string(displayLabel) + "##" + std::string(id);
}
}

void registerWidget(UiState& uiState, std::string_view label, std::string_view type)
{
    (void)ensureWidget(uiState, label, type);
}

void Text(UiState& uiState, const char* label, const std::string& value)
{
    auto& widget = ensureWidget(uiState, label, "text");
    widget.textValue = value;
    if (!uiState.testMode) ImGui::Text("%s", value.c_str());
}

bool Checkbox(UiState& uiState, const char* label, bool& value)
{
    auto& widget = ensureWidget(uiState, label, "checkbox");
    const bool simulatedChange = consumeBool(uiState, label, value);
    widget.boolValue = value;
    if (uiState.testMode) return simulatedChange;

    const bool changed = ImGui::Checkbox(label, &value);
    widget.boolValue = value;
    return simulatedChange || changed;
}

bool Checkbox(UiState& uiState, const char* id, const char* displayLabel, bool& value)
{
    auto& widget = ensureWidget(uiState, id, "checkbox");
    const bool simulatedChange = consumeBool(uiState, id, value);
    widget.boolValue = value;
    if (uiState.testMode) return simulatedChange;

    const auto label = imguiLabel(id, displayLabel);
    const bool changed = ImGui::Checkbox(label.c_str(), &value);
    widget.boolValue = value;
    return simulatedChange || changed;
}

bool Button(UiState& uiState, const char* label)
{
    auto& widget = ensureWidget(uiState, label, "button");
    const bool clicked = consumeClick(uiState, label);
    widget.boolValue = clicked;
    if (uiState.testMode) return clicked;
    return clicked || ImGui::Button(label);
}

bool Button(UiState& uiState, const char* id, const char* displayLabel)
{
    auto& widget = ensureWidget(uiState, id, "button");
    const bool clicked = consumeClick(uiState, id);
    widget.boolValue = clicked;
    if (uiState.testMode) return clicked;
    const auto label = imguiLabel(id, displayLabel);
    return clicked || ImGui::Button(label.c_str());
}

std::string Input(UiState& uiState, const char* label, const std::string& value)
{
    auto& widget = ensureWidget(uiState, label, "input");
    std::string currentValue = value;
    const bool simulatedEdit = consumeText(uiState, label, currentValue);
    widget.textValue = currentValue;
    if (uiState.testMode) return currentValue;

    std::array<char, 256> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", currentValue.c_str());
    ImGui::InputText(label, buffer.data(), buffer.size());
    widget.textValue = buffer.data();
    return widget.textValue;
}

std::string Input(UiState& uiState, const char* id, const char* displayLabel, const std::string& value)
{
    auto& widget = ensureWidget(uiState, id, "input");
    std::string currentValue = value;
    consumeText(uiState, id, currentValue);
    widget.textValue = currentValue;
    if (uiState.testMode) return currentValue;

    std::array<char, 256> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", currentValue.c_str());
    const auto label = imguiLabel(id, displayLabel);
    ImGui::InputText(label.c_str(), buffer.data(), buffer.size());
    widget.textValue = buffer.data();
    return widget.textValue;
}
