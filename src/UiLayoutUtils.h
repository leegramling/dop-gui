#pragma once

#include "AppState.h"
#include "YogaLayout.h"

#include <functional>
#include <string_view>

/**
 * @brief Stable mapping between a widget id and its layout slots.
 */
struct WidgetSlotBinding
{
    std::string valueSlotId;
    std::string labelSlotId;
};

/**
 * @brief Queue a UI-originated command request.
 * @param uiState UI state that owns the pending command queue.
 * @param commandName Command name to enqueue.
 * @param value Optional serialized argument payload.
 */
void queueUiCommand(UiState& uiState, const std::string& commandName, const std::string& value = {});
/**
 * @brief Register a single computed layout slot.
 * @param uiState UI state that owns the registered slot list.
 * @param panelId Stable panel identifier.
 * @param slotId Stable layout slot identifier.
 * @param layout Computed layout rectangle for the slot.
 */
void registerLayoutSlot(UiState& uiState, const std::string& panelId, const std::string& slotId, const UiLayoutRectState& layout);
/**
 * @brief Register multiple computed layout slots from a Yoga layout.
 * @param uiState UI state that owns the registered slot list.
 * @param panelId Stable panel identifier.
 * @param layout Yoga layout that provides computed slot rectangles.
 * @param slotIds Slot ids to record when present in the Yoga layout.
 */
void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string_view>& slotIds);
/**
 * @brief Register multiple computed layout slots from a Yoga layout.
 * @param uiState UI state that owns the registered slot list.
 * @param panelId Stable panel identifier.
 * @param layout Yoga layout that provides computed slot rectangles.
 * @param slotIds Slot ids to record when present in the Yoga layout.
 */
void registerLayoutSlots(UiState& uiState, const std::string& panelId, const YogaLayout& layout, const std::vector<std::string>& slotIds);
/**
 * @brief Queue the next widget layout if a Yoga slot is present.
 * @param uiState UI state that owns the pending widget layout.
 * @param layout Yoga layout that provides computed slot rectangles.
 * @param slotId Slot id to look up.
 */
void setNextWidgetLayoutIfPresent(UiState& uiState, const YogaLayout& layout, std::string_view slotId);
/**
 * @brief Build a widget-to-slot binding using a panel-specific label-slot resolver.
 * @param widgetId Stable widget identifier.
 * @param labelResolver Function that maps widget ids to label slot ids.
 * @return Resolved slot binding for the widget.
 */
WidgetSlotBinding makeWidgetSlotBinding(
    std::string_view widgetId,
    const std::function<std::string(std::string_view)>& labelResolver);
