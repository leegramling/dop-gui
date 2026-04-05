#pragma once

#include "AppState.h"

class Panel
{
public:
    Panel(UiState& uiState, const char* id, const char* title);
    ~Panel();

    bool begin();

private:
    UiState& _uiState;
    const char* _id;
    const char* _title;
    bool _opened = false;
};
