#include "UiLayoutUtils.h"

#include "Widgets.h"

void queueUiCommand(UiState& uiState, const std::string& commandName, const std::string& value)
{
    if (commandName.empty()) return;
    uiState.requestedCommands.push_back(value.empty() ? commandName : (commandName + "=" + value));
}

void registerLayoutSlot(UiState& uiState, const std::string& panelId, const std::string& slotId, const UiLayoutRectState& layout)
{
    for (auto& slot : uiState.layoutSlots)
    {
        if (slot.panelId == panelId && slot.slotId == slotId)
        {
            slot.layout = layout;
            return;
        }
    }

    uiState.layoutSlots.push_back(UiLayoutSlotState{
        .panelId = panelId,
        .slotId = slotId,
        .layout = layout,
    });
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string_view>& slotIds)
{
    for (const auto slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, std::string(slotId), layout.rect(slotId));
        }
    }
}

void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string>& slotIds)
{
    for (const auto& slotId : slotIds)
    {
        if (layout.has(slotId))
        {
            registerLayoutSlot(uiState, panelId, slotId, layout.rect(slotId));
        }
    }
}

void setNextWidgetLayoutIfPresent(UiState& uiState, const YogaLayout& layout, std::string_view slotId)
{
    if (layout.has(slotId))
    {
        setNextWidgetLayout(uiState, layout.rect(slotId));
    }
}

WidgetSlotBinding makeWidgetSlotBinding(
    std::string_view widgetId,
    const std::function<std::string(std::string_view)>& labelResolver)
{
    return WidgetSlotBinding{
        .valueSlotId = std::string(widgetId),
        .labelSlotId = labelResolver(widgetId),
    };
}
