#include "Widgets.h"

#include <vsgImGui/imgui.h>

#include <array>
#include <cstdio>
#include <iomanip>
#include <optional>
#include <sstream>

namespace
{
WidgetState& ensureWidget(UiState& uiState, std::string_view label, std::string_view type)
{
    if (!uiState.currentPanelId.empty())
    {
        if (auto* existing = findWidget(uiState, uiState.currentPanelId, std::string(label)))
        {
            existing->label = std::string(label);
            existing->panelId = uiState.currentPanelId;
            existing->widgetId = std::string(label);
            existing->type = std::string(type);
            return *existing;
        }

        uiState.registry.push_back(WidgetState{
            .label = std::string(label),
            .panelId = uiState.currentPanelId,
            .widgetId = std::string(label),
            .type = std::string(type),
        });
        return uiState.registry.back();
    }

    if (auto* existing = findWidget(uiState, std::string(label)))
    {
        existing->panelId = uiState.currentPanelId;
        existing->widgetId = std::string(label);
        existing->type = std::string(type);
        return *existing;
    }

    uiState.registry.push_back(WidgetState{
        .label = std::string(label),
        .panelId = uiState.currentPanelId,
        .widgetId = std::string(label),
        .type = std::string(type),
    });
    return uiState.registry.back();
}

bool consumeClick(UiState& uiState, std::string_view label)
{
    if (auto* action = findPendingUiAction(uiState, uiState.currentPanelId, label, "click"))
    {
        action->kind.clear();
        return true;
    }
    return false;
}

bool consumeBool(UiState& uiState, std::string_view label, bool& value)
{
    if (auto* action = findPendingUiAction(uiState, uiState.currentPanelId, label, "set_bool"))
    {
        value = action->boolValue;
        action->kind.clear();
        return true;
    }
    return false;
}

bool consumeText(UiState& uiState, std::string_view label, std::string& value)
{
    if (auto* action = findPendingUiAction(uiState, uiState.currentPanelId, label, "set_text"))
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

std::string formatDouble(double value, int precision, std::string_view unitSuffix)
{
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(precision);
    out << value;
    if (!unitSuffix.empty()) out << " " << unitSuffix;
    return out.str();
}

std::optional<double> parseDoubleText(std::string text, std::string_view unitSuffix)
{
    if (!unitSuffix.empty())
    {
        const auto unitPos = text.find(unitSuffix);
        if (unitPos != std::string::npos)
        {
            text = text.substr(0, unitPos);
        }
    }

    while (!text.empty() && text.back() == ' ') text.pop_back();
    while (!text.empty() && text.front() == ' ') text.erase(text.begin());
    if (text.empty()) return std::nullopt;

    try
    {
        std::size_t consumed = 0;
        const double parsed = std::stod(text, &consumed);
        if (consumed != text.size()) return std::nullopt;
        return parsed;
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

std::optional<UiLayoutRectState> consumeNextLayout(UiState& uiState)
{
    auto layout = uiState.nextWidgetLayout;
    uiState.nextWidgetLayout.reset();
    return layout;
}

void applyLayoutRect(const std::optional<UiLayoutRectState>& layout)
{
    if (!layout || !layout->enabled) return;
    ImGui::SetCursorPos(ImVec2(static_cast<float>(layout->x), static_cast<float>(layout->y)));
    if (layout->width > 0.0) ImGui::SetNextItemWidth(static_cast<float>(layout->width));
}

void recordWidgetRect(WidgetState& widget, const std::optional<UiLayoutRectState>& requestedLayout)
{
    if (requestedLayout && requestedLayout->enabled)
    {
        widget.layout = *requestedLayout;
        return;
    }

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    widget.layout.enabled = true;
    widget.layout.x = static_cast<double>(min.x);
    widget.layout.y = static_cast<double>(min.y);
    widget.layout.width = static_cast<double>(max.x - min.x);
    widget.layout.height = static_cast<double>(max.y - min.y);
}
}

void registerWidget(UiState& uiState, std::string_view label, std::string_view type)
{
    (void)ensureWidget(uiState, label, type);
}

void setNextWidgetLayout(UiState& uiState, const UiLayoutRectState& layout)
{
    uiState.nextWidgetLayout = layout;
}

void Text(UiState& uiState, const char* label, const std::string& value)
{
    auto& widget = ensureWidget(uiState, label, "text");
    const auto requestedLayout = consumeNextLayout(uiState);
    widget.textValue = value;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return;
    }

    applyLayoutRect(requestedLayout);
    ImGui::Text("%s", value.c_str());
    recordWidgetRect(widget, requestedLayout);
}

bool Checkbox(UiState& uiState, const char* label, bool& value)
{
    auto& widget = ensureWidget(uiState, label, "checkbox");
    const auto requestedLayout = consumeNextLayout(uiState);
    const bool simulatedChange = consumeBool(uiState, label, value);
    widget.boolValue = value;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return simulatedChange;
    }

    applyLayoutRect(requestedLayout);
    const bool changed = ImGui::Checkbox(label, &value);
    widget.boolValue = value;
    recordWidgetRect(widget, requestedLayout);
    return simulatedChange || changed;
}

bool Checkbox(UiState& uiState, const char* id, const char* displayLabel, bool& value)
{
    auto& widget = ensureWidget(uiState, id, "checkbox");
    const auto requestedLayout = consumeNextLayout(uiState);
    const bool simulatedChange = consumeBool(uiState, id, value);
    widget.boolValue = value;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return simulatedChange;
    }

    const auto label = imguiLabel(id, displayLabel);
    applyLayoutRect(requestedLayout);
    const bool changed = ImGui::Checkbox(label.c_str(), &value);
    widget.boolValue = value;
    recordWidgetRect(widget, requestedLayout);
    return simulatedChange || changed;
}

bool Button(UiState& uiState, const char* label)
{
    auto& widget = ensureWidget(uiState, label, "button");
    const auto requestedLayout = consumeNextLayout(uiState);
    const bool clicked = consumeClick(uiState, label);
    widget.boolValue = clicked;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return clicked;
    }
    applyLayoutRect(requestedLayout);
    const bool result = clicked || ImGui::Button(label);
    recordWidgetRect(widget, requestedLayout);
    return result;
}

bool Button(UiState& uiState, const char* id, const char* displayLabel)
{
    auto& widget = ensureWidget(uiState, id, "button");
    const auto requestedLayout = consumeNextLayout(uiState);
    const bool clicked = consumeClick(uiState, id);
    widget.boolValue = clicked;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return clicked;
    }
    const auto label = imguiLabel(id, displayLabel);
    applyLayoutRect(requestedLayout);
    const bool result = clicked || ImGui::Button(label.c_str());
    recordWidgetRect(widget, requestedLayout);
    return result;
}

std::string Input(UiState& uiState, const char* label, const std::string& value)
{
    auto& widget = ensureWidget(uiState, label, "input");
    const auto requestedLayout = consumeNextLayout(uiState);
    std::string currentValue = value;
    const bool simulatedEdit = consumeText(uiState, label, currentValue);
    widget.textValue = currentValue;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return currentValue;
    }

    std::array<char, 256> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", currentValue.c_str());
    applyLayoutRect(requestedLayout);
    ImGui::InputText(label, buffer.data(), buffer.size());
    widget.textValue = buffer.data();
    recordWidgetRect(widget, requestedLayout);
    return widget.textValue;
}

std::string Input(UiState& uiState, const char* id, const char* displayLabel, const std::string& value)
{
    auto& widget = ensureWidget(uiState, id, "input");
    const auto requestedLayout = consumeNextLayout(uiState);
    std::string currentValue = value;
    consumeText(uiState, id, currentValue);
    widget.textValue = currentValue;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return currentValue;
    }

    std::array<char, 256> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", currentValue.c_str());
    const auto label = imguiLabel(id, displayLabel);
    applyLayoutRect(requestedLayout);
    ImGui::InputText(label.c_str(), buffer.data(), buffer.size());
    widget.textValue = buffer.data();
    recordWidgetRect(widget, requestedLayout);
    return widget.textValue;
}

double InputDouble(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    double value,
    int precision,
    const char* unitSuffix)
{
    auto& widget = ensureWidget(uiState, id, "input_double");
    const auto requestedLayout = consumeNextLayout(uiState);
    const std::string unit = unitSuffix ? unitSuffix : "";
    double currentValue = value;
    std::string formattedValue = formatDouble(currentValue, precision, unit);
    consumeText(uiState, id, formattedValue);
    if (auto parsed = parseDoubleText(formattedValue, unit)) currentValue = *parsed;
    widget.textValue = formatDouble(currentValue, precision, unit);

    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return currentValue;
    }

    const auto label = imguiLabel(id, displayLabel);
    applyLayoutRect(requestedLayout);
    if (!requestedLayout || requestedLayout->width <= 0.0) ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputDouble(label.c_str(), &currentValue, 0.0, 0.0, nullptr, ImGuiInputTextFlags_CharsScientific);
    if (!unit.empty())
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(unit.c_str());
    }
    widget.textValue = formatDouble(currentValue, precision, unit);
    recordWidgetRect(widget, requestedLayout);
    return currentValue;
}

bool RadioButton(UiState& uiState, const char* id, const char* displayLabel, bool selected)
{
    auto& widget = ensureWidget(uiState, id, "radio");
    const auto requestedLayout = consumeNextLayout(uiState);
    const bool clicked = consumeClick(uiState, id);
    widget.boolValue = selected || clicked;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return clicked;
    }

    const auto label = imguiLabel(id, displayLabel);
    applyLayoutRect(requestedLayout);
    const bool changed = ImGui::RadioButton(label.c_str(), selected);
    widget.boolValue = selected || changed;
    recordWidgetRect(widget, requestedLayout);
    return clicked || changed;
}

std::string ComboBox(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    const std::string& currentValue,
    const std::vector<std::string>& options)
{
    auto& widget = ensureWidget(uiState, id, "combo");
    const auto requestedLayout = consumeNextLayout(uiState);
    std::string selectedValue = currentValue;
    consumeText(uiState, id, selectedValue);
    widget.textValue = selectedValue;
    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        return selectedValue;
    }

    int selectedIndex = 0;
    for (std::size_t i = 0; i < options.size(); ++i)
    {
        if (options[i] == selectedValue)
        {
            selectedIndex = static_cast<int>(i);
            break;
        }
    }

    const auto label = imguiLabel(id, displayLabel);
    applyLayoutRect(requestedLayout);
    if (ImGui::BeginCombo(label.c_str(), selectedValue.c_str()))
    {
        for (std::size_t i = 0; i < options.size(); ++i)
        {
            const bool isSelected = static_cast<int>(i) == selectedIndex;
            if (ImGui::Selectable(options[i].c_str(), isSelected))
            {
                selectedValue = options[i];
                selectedIndex = static_cast<int>(i);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    widget.textValue = selectedValue;
    recordWidgetRect(widget, requestedLayout);
    return selectedValue;
}

bool Popup(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    bool requestOpen,
    const std::function<void()>& emitContents)
{
    auto& widget = ensureWidget(uiState, id, "popup");
    const auto requestedLayout = consumeNextLayout(uiState);
    const bool clicked = consumeClick(uiState, id);
    const bool shouldOpen = requestOpen || clicked;

    if (uiState.testMode)
    {
        widget.boolValue = shouldOpen;
        widget.textValue = displayLabel;
        if (requestedLayout) widget.layout = *requestedLayout;
        if (shouldOpen) emitContents();
        return shouldOpen;
    }

    const auto label = imguiLabel(id, displayLabel);
    if (shouldOpen) ImGui::OpenPopup(label.c_str());
    const bool open = ImGui::BeginPopup(label.c_str());
    widget.boolValue = open;
    widget.textValue = displayLabel;
    if (open)
    {
        const auto popupPos = ImGui::GetWindowPos();
        const auto popupSize = ImGui::GetWindowSize();
        widget.layout.enabled = true;
        widget.layout.x = static_cast<double>(popupPos.x);
        widget.layout.y = static_cast<double>(popupPos.y);
        widget.layout.width = static_cast<double>(popupSize.x);
        widget.layout.height = static_cast<double>(popupSize.y);
        emitContents();
        ImGui::EndPopup();
    }
    else if (requestedLayout)
    {
        widget.layout = *requestedLayout;
    }
    return open;
}

void Table(
    UiState& uiState,
    const char* id,
    int columnCount,
    std::size_t rowCount,
    const std::function<void()>& emitRows)
{
    auto& widget = ensureWidget(uiState, id, "table");
    const auto requestedLayout = consumeNextLayout(uiState);
    widget.textValue = std::to_string(rowCount);
    widget.boolValue = rowCount > 0;

    if (uiState.testMode)
    {
        if (requestedLayout) widget.layout = *requestedLayout;
        emitRows();
        return;
    }

    applyLayoutRect(requestedLayout);
    if (ImGui::BeginTable(id, columnCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        emitRows();
        ImGui::EndTable();
        recordWidgetRect(widget, requestedLayout);
    }
}
