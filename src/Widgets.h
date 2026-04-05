#pragma once

#include "AppState.h"

#include <string_view>

void registerWidget(UiState& uiState, std::string_view label, std::string_view type);
void Text(UiState& uiState, const char* label, const std::string& value);
bool Checkbox(UiState& uiState, const char* label, bool& value);
bool Checkbox(UiState& uiState, const char* id, const char* displayLabel, bool& value);
bool Button(UiState& uiState, const char* label);
bool Button(UiState& uiState, const char* id, const char* displayLabel);
std::string Input(UiState& uiState, const char* label, const std::string& value);
std::string Input(UiState& uiState, const char* id, const char* displayLabel, const std::string& value);
