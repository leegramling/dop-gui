#pragma once

#include "AppState.h"

#include <functional>
#include <string_view>

/**
 * @brief Register a widget label and type in the UI registry.
 * @param uiState UI-local state containing the widget registry.
 * @param label Stable widget label.
 * @param type Widget type name recorded for queries.
 */
void registerWidget(UiState& uiState, std::string_view label, std::string_view type);
/**
 * @brief Queue an authored layout rectangle for the next wrapped widget evaluation.
 * @param uiState UI-local state containing pending layout placement.
 * @param layout Panel-local layout rectangle to apply to the next widget.
 */
void setNextWidgetLayout(UiState& uiState, const UiLayoutRectState& layout);
/**
 * @brief Render or evaluate a text widget.
 * @param uiState UI-local state containing the widget registry.
 * @param label Stable widget label.
 * @param value Text value to display or expose in test mode.
 */
void Text(UiState& uiState, const char* label, const std::string& value);
/**
 * @brief Render or evaluate a checkbox widget using the widget label as the ImGui label.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param label Stable widget label and visible ImGui label.
 * @param value Boolean value bound to the widget.
 * @return True when the widget toggles during this evaluation.
 */
bool Checkbox(UiState& uiState, const char* label, bool& value);
/**
 * @brief Render or evaluate a checkbox widget with a separate display label.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param value Boolean value bound to the widget.
 * @return True when the widget toggles during this evaluation.
 */
bool Checkbox(UiState& uiState, const char* id, const char* displayLabel, bool& value);
/**
 * @brief Render or evaluate a button widget using the widget label as the ImGui label.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param label Stable widget label and visible ImGui label.
 * @return True when the button is activated during this evaluation.
 */
bool Button(UiState& uiState, const char* label);
/**
 * @brief Render or evaluate a button widget with a separate display label.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @return True when the button is activated during this evaluation.
 */
bool Button(UiState& uiState, const char* id, const char* displayLabel);
/**
 * @brief Clearer alias for a wrapped action button.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @return True when the button is activated during this evaluation.
 */
inline bool ActionButton(UiState& uiState, const char* id, const char* displayLabel)
{
    return Button(uiState, id, displayLabel);
}
/**
 * @brief Render or evaluate an input widget using the widget label as the ImGui label.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param label Stable widget label and visible ImGui label.
 * @param value Current string value.
 * @return Final string value after this evaluation.
 */
std::string Input(UiState& uiState, const char* label, const std::string& value);
/**
 * @brief Render or evaluate an input widget with a separate display label.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param value Current string value.
 * @return Final string value after this evaluation.
 */
std::string Input(UiState& uiState, const char* id, const char* displayLabel, const std::string& value);
/**
 * @brief Clearer alias for a wrapped text input field.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param value Current string value.
 * @return Final string value after this evaluation.
 */
inline std::string TextField(UiState& uiState, const char* id, const char* displayLabel, const std::string& value)
{
    return Input(uiState, id, displayLabel, value);
}
/**
 * @brief Render or evaluate a floating-point input widget.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param value Current numeric value.
 * @param precision Number of fractional digits to preserve in widget text.
 * @param unitSuffix Optional unit suffix shown in the registry text and live UI.
 * @return Final numeric value after this evaluation.
 */
double InputDouble(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    double value,
    int precision = 3,
    const char* unitSuffix = nullptr);
/**
 * @brief Clearer alias for a wrapped numeric input field.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param value Current numeric value.
 * @param precision Number of fractional digits to preserve in widget text.
 * @param unitSuffix Optional unit suffix shown in the registry text and live UI.
 * @return Final numeric value after this evaluation.
 */
inline double NumericField(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    double value,
    int precision = 3,
    const char* unitSuffix = nullptr)
{
    return InputDouble(uiState, id, displayLabel, value, precision, unitSuffix);
}
/**
 * @brief Render or evaluate a radio button widget.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param selected Whether this radio option is currently selected.
 * @return True when this radio option is activated during the evaluation.
 */
bool RadioButton(UiState& uiState, const char* id, const char* displayLabel, bool selected);
/**
 * @brief Render or evaluate a combo box widget using a string selection.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param currentValue Current selected value.
 * @param options Ordered list of available option values.
 * @return Final selected option value after this evaluation.
 */
std::string ComboBox(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    const std::string& currentValue,
    const std::vector<std::string>& options);
/**
 * @brief Clearer alias for a wrapped single-selection field.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable widget identifier.
 * @param displayLabel Visible ImGui label.
 * @param currentValue Current selected value.
 * @param options Ordered list of available option values.
 * @return Final selected option value after this evaluation.
 */
inline std::string SelectField(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    const std::string& currentValue,
    const std::vector<std::string>& options)
{
    return ComboBox(uiState, id, displayLabel, currentValue, options);
}
/**
 * @brief Clearer alias for wrapped text emission.
 * @param uiState UI-local state containing the widget registry.
 * @param label Stable widget label.
 * @param value Text value to display or expose in test mode.
 */
inline void DisplayText(UiState& uiState, const char* label, const std::string& value)
{
    Text(uiState, label, value);
}
/**
 * @brief Render or evaluate a popup widget and its contents.
 * @param uiState UI-local state containing the widget registry and test actions.
 * @param id Stable popup identifier.
 * @param displayLabel Visible popup title.
 * @param requestOpen True when the popup should be opened during this evaluation.
 * @param emitContents Callback that emits popup contents through wrapped widgets.
 * @return True when the popup is open during this evaluation.
 */
bool Popup(
    UiState& uiState,
    const char* id,
    const char* displayLabel,
    bool requestOpen,
    const std::function<void()>& emitContents);
/**
 * @brief Render or evaluate a table widget and its contents.
 * @param uiState UI-local state containing the widget registry.
 * @param id Stable table identifier.
 * @param columnCount Number of table columns.
 * @param rowCount Current number of logical table rows.
 * @param emitRows Callback that emits table rows and cells through wrapped widgets.
 */
void Table(
    UiState& uiState,
    const char* id,
    int columnCount,
    std::size_t rowCount,
    const std::function<void()>& emitRows);
