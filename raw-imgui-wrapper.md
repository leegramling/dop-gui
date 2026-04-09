# Raw ImGui Wrapper Guide

This note describes how to make panel code safe for headless use when the panel currently calls raw ImGui APIs directly.

The short version is:

- wrapped widget calls like `Button`, `Input`, and `Checkbox` are easy to support in headless mode
- raw ImGui context/layout/style calls are only safe if you either:
  - guard them with `if (!state.ui.testMode)`, or
  - provide wrapper functions that become no-ops or test-safe behavior in headless mode

## Why This Is Needed

In this project, desktop ImGui context setup happens through `vsgImGui::RenderImGui`, which creates and drives the real ImGui context during the live render path.

Headless UI evaluation works differently:

- `UiManager::evaluate(...)` sets `testMode = true`
- it calls the same panel `render(...)` methods directly
- there may be no live VSG viewer or window path active

That means unguarded raw ImGui calls like these can be unsafe in headless mode:

- `ImGui::GetIO()`
- `ImGui::PushStyleColor(...)`
- `ImGui::PushStyleVar(...)`
- `ImGui::GetCursorPos()`
- `ImGui::GetContentRegionAvail()`
- `ImGui::Begin(...)`
- `ImGui::BeginCombo(...)`
- `ImGui::BeginTable(...)`

If your panel render method uses these directly, the safest pattern is to route them through wrapper helpers.

## Recommended Strategy

For an app that wants both desktop and headless UI evaluation:

1. keep business widgets behind wrappers
   - `Button`
   - `Checkbox`
   - `Input`
   - `Combo`
   - `Popup`
   - `Table`

2. wrap raw ImGui context/layout/style calls too
   - `PushStyleColor`
   - `PushStyleVar`
   - `PopStyleColor`
   - `PopStyleVar`
   - `SetCursorPos`
   - `SameLine`
   - `GetCursorPos`
   - `GetContentRegionAvail`
   - `GetIO`

3. make the wrapper policy explicit
   - some wrappers should be no-ops in headless mode
   - some should return fallback values
   - some should still record useful metadata in your UI registry

4. keep raw ImGui usage out of most panel code
   - panels should ideally call your wrapper layer, not ImGui directly

## Wrapper Categories

### 1. Context and Global State

These functions depend on an active ImGui context and should not be called unguarded in headless mode.

Wrap these:

- `ImGui::GetIO()`
- `ImGui::GetStyle()`
- `ImGui::GetMainViewport()`
- `ImGui::GetCurrentContext()`
- `ImGui::GetPlatformIO()`

Suggested headless behavior:

- return a small app-owned proxy state if needed
- or avoid calling them entirely in headless mode

These are usually the first wrappers to add.

### 2. Style Stack

These are very good candidates for no-op wrappers in headless mode.

Wrap these:

- `ImGui::PushStyleColor(...)`
- `ImGui::PopStyleColor(...)`
- `ImGui::PushStyleVar(...)`
- `ImGui::PopStyleVar(...)`
- `ImGui::StyleColorsDark(...)`
- `ImGui::StyleColorsLight(...)`

Suggested headless behavior:

- no-op

Reason:

- style changes usually do not affect state-based testing
- they are mainly visual concerns

### 3. Layout and Cursor Positioning

These are common in custom panel code and are often unsafe or meaningless in headless mode unless you provide fallbacks.

Wrap these:

- `ImGui::GetCursorPos()`
- `ImGui::SetCursorPos(...)`
- `ImGui::GetContentRegionAvail()`
- `ImGui::SetNextItemWidth(...)`
- `ImGui::SetNextWindowPos(...)`
- `ImGui::SetNextWindowSize(...)`
- `ImGui::SetNextWindowSizeConstraints(...)`
- `ImGui::SameLine()`
- `ImGui::SetItemDefaultFocus()`

Suggested headless behavior:

- return fallback panel-local values such as `(0,0)` or authored layout size
- or no-op when the call only affects visual placement

If your app already has an authored/Yoga layout system, these wrappers should usually consume that layout state instead of asking ImGui for geometry.

### 4. Window and Container Lifetime

These should usually be centralized and not scattered through panel code.

Wrap these:

- `ImGui::Begin(...)`
- `ImGui::End()`
- `ImGui::BeginMainMenuBar()`
- `ImGui::EndMainMenuBar()`
- `ImGui::BeginMenu(...)`
- `ImGui::EndMenu()`
- `ImGui::DockSpaceOverViewport(...)`

Suggested headless behavior:

- avoid calling the real ImGui function
- instead:
  - register the menu/panel/container in your UI registry
  - decide container open/closed state from app state

This is how the current project handles much of the menu behavior in headless mode.

### 4a. Menus, Combos, and Tables Need Structure Wrappers

Menus, combos, and tables are a special case because they are not just simple widgets. They are container-style APIs with begin/end or row/column structure, and panel logic often depends on their internal flow.

That means a pure “just no-op it” strategy is usually not enough.

For these controls, the wrapper should do two jobs:

1. preserve the logical structure in desktop mode
2. provide a deterministic fake structure in headless mode so the panel logic still runs

#### Menus

For menus, the wrapper should:

- register the menu and menu items
- allow scripted clicks to target menu items by stable ids
- in headless mode, treat a menu as logically open when a matching pending click exists

That is effectively what this project already does in [UiManager.cpp](/home/lgramling/dev/dop-gui/src/UiManager.cpp).

#### Combos

For combos, do not let panel code call `ImGui::BeginCombo(...)` and `ImGui::Selectable(...)` directly.

Instead, wrap the whole control as one semantic operation, for example:

```cpp
std::string uiSelect(
    UiState& uiState,
    std::string_view id,
    std::string_view displayLabel,
    std::string currentValue,
    const std::vector<std::string>& options)
{
    auto& widget = ensureWidget(uiState, id, "combo");
    widget.textValue = currentValue;

    // Headless path: consume scripted text and return the selected value.
    if (consumeText(uiState, id, currentValue))
    {
        widget.textValue = currentValue;
    }
    if (uiState.testMode)
    {
        return currentValue;
    }

    // Desktop path: use real ImGui combo behavior.
    const auto label = imguiLabel(id, displayLabel);
    if (ImGui::BeginCombo(label.c_str(), currentValue.c_str()))
    {
        for (const auto& option : options)
        {
            const bool selected = (option == currentValue);
            if (ImGui::Selectable(option.c_str(), selected))
            {
                currentValue = option;
                widget.textValue = currentValue;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return currentValue;
}
```

Why this shape is better:

- the panel sees one semantic “select a value” function
- headless mode never needs to fake begin/end combo structure
- the wrapper owns all of the ImGui container details

If you still need a lower-level begin/end combo wrapper for a legacy panel, keep the headless behavior explicit and minimal:

```cpp
bool uiComboBegin(
    UiState& uiState,
    std::string_view id,
    std::string_view displayLabel,
    std::string_view selectedValue)
{
    auto& widget = ensureWidget(uiState, id, "combo");
    widget.textValue = std::string(selectedValue);

    if (uiState.testMode)
    {
        // Headless policy:
        // pretend the combo is logically available so panel code can continue,
        // but do not depend on a real ImGui popup/list context existing.
        return true;
    }

    const auto label = imguiLabel(id, displayLabel);
    return ImGui::BeginCombo(label.c_str(), std::string(selectedValue).c_str());
}
```

And the paired end wrapper:

```cpp
void uiComboEnd(UiState& uiState)
{
    if (uiState.testMode)
    {
        return;
    }

    ImGui::EndCombo();
}
```

Important caveat:

- `uiComboBegin` is useful as a migration wrapper
- but the higher-level semantic wrapper is still the better long-term design
- otherwise panel code may still become coupled to visual combo structure in ways that are awkward to simulate headlessly

#### Tables

Tables should also be wrapped at the semantic level instead of sprinkling raw `BeginTable`, `TableNextRow`, and `TableSetColumnIndex` calls through panel code.

A good wrapper pattern is:

1. one wrapper begins the table
2. one helper emits a row
3. one wrapper ends the table

Example:

```cpp
bool uiBeginTable(UiState& uiState, std::string_view id, int columnCount)
{
    auto& widget = ensureWidget(uiState, id, "table");
    widget.boolValue = true;

    if (uiState.testMode)
    {
        return true;
    }

    return ImGui::BeginTable(
        std::string(id).c_str(),
        columnCount,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
}

void uiTableRow(UiState& uiState, const std::vector<std::string>& cells)
{
    if (uiState.testMode)
    {
        return;
    }

    ImGui::TableNextRow();
    for (std::size_t i = 0; i < cells.size(); ++i)
    {
        ImGui::TableSetColumnIndex(static_cast<int>(i));
        ImGui::TextUnformatted(cells[i].c_str());
    }
}

void uiEndTable(UiState& uiState)
{
    if (uiState.testMode)
    {
        return;
    }

    ImGui::EndTable();
}
```

If your tests care about table contents, do not rely only on the drawing path. Also register the row/column data in a queryable structure so headless mode can report:

- column names
- row count
- cell text

That way the table remains testable even when no pixels are rendered.

#### Practical rule

For menus, combos, and tables:

- wrap the whole semantic control, not just individual raw ImGui calls
- keep begin/end structure inside the wrapper layer
- expose stable ids and queryable state
- let headless mode operate on the semantic value, not on the visual container mechanics

### 5. Readback and Geometry Queries

These are often useful in desktop mode but should be treated carefully in headless mode.

Wrap these:

- `ImGui::GetItemRectMin()`
- `ImGui::GetItemRectMax()`
- `ImGui::GetWindowPos()`
- `ImGui::GetWindowSize()`

Suggested headless behavior:

- use authored or computed layout values instead
- or return empty/default rectangles

In this project, wrapped widgets record layout either from authored layout or from ImGui item rectangles in desktop mode.

### 6. Widget-Like Raw Calls

If your panel calls these directly instead of through app wrappers, they should also be wrapped.

Wrap these:

- `ImGui::Text(...)`
- `ImGui::TextUnformatted(...)`
- `ImGui::Button(...)`
- `ImGui::Checkbox(...)`
- `ImGui::InputText(...)`
- `ImGui::InputDouble(...)`
- `ImGui::RadioButton(...)`
- `ImGui::BeginCombo(...)`
- `ImGui::Selectable(...)`
- `ImGui::EndCombo()`
- `ImGui::OpenPopup(...)`
- `ImGui::BeginPopup(...)`
- `ImGui::EndPopup()`
- `ImGui::BeginTable(...)`
- `ImGui::TableSetupColumn(...)`
- `ImGui::TableHeadersRow()`
- `ImGui::TableNextRow()`
- `ImGui::TableSetColumnIndex(...)`
- `ImGui::EndTable()`

Suggested headless behavior:

- register the widget or structure
- consume queued test actions where relevant
- return a deterministic value to panel logic

This is the core of the current widget wrapper pattern.

### 7. Draw Lists and Custom Paint

Custom draw-list code is another important category to wrap.

Wrap these kinds of calls:

- `ImGui::GetWindowDrawList()`
- `ImGui::GetForegroundDrawList()`
- `ImGui::GetBackgroundDrawList()`
- `ImDrawList::AddLine(...)`
- `ImDrawList::AddRect(...)`
- `ImDrawList::AddRectFilled(...)`
- `ImDrawList::AddCircle(...)`
- `ImDrawList::AddCircleFilled(...)`
- `ImDrawList::AddText(...)`
- `ImDrawList::AddImage(...)`
- `ImDrawList::AddTriangle(...)`
- `ImDrawList::AddBezierCubic(...)`
- `ImDrawList::PathLineTo(...)`
- `ImDrawList::PathStroke(...)`
- `ImDrawList::PathFillConvex(...)`

Suggested headless behavior:

- do not issue real draw-list calls
- optionally record a lightweight paint description into a debug or test registry
- or simply no-op if the drawing is purely decorative

Why this matters:

- draw-list usage often assumes a live ImGui window and valid pixel coordinates
- it is usually not needed for logic-focused headless testing
- but if your tests care about diagram structure or overlay semantics, you may want a test-side representation of the draw commands

Good wrapper examples:

- `uiDrawLine(...)`
- `uiDrawRect(...)`
- `uiDrawTextOverlay(...)`
- `uiDrawIcon(...)`
- `uiBeginCanvas(...)`
- `uiEndCanvas(...)`

Good headless policies for draw wrappers:

- decorative only:
  - no-op in headless mode

- semantically meaningful drawing:
  - record a shape list or paint command list for queries

- visually testable overlays:
  - use the wrapper to support later snapshot testing

If your app has node editors, gizmos, graphs, rulers, overlays, or custom icon painting, draw-list wrappers are just as important as widget wrappers.

## Functions Seen In This Repo

These raw ImGui calls currently appear in `src/` and are the best starting point for wrapper coverage:

### High priority wrappers

- `ImGui::GetIO()`
- `ImGui::GetCursorPos()`
- `ImGui::GetContentRegionAvail()`
- `ImGui::SetCursorPos(...)`
- `ImGui::SetNextItemWidth(...)`
- `ImGui::GetItemRectMin()`
- `ImGui::GetItemRectMax()`
- `ImGui::GetStyle()`
- `ImGui::GetMainViewport()`
- `ImGui::GetCurrentContext()`
- `ImGui::GetPlatformIO()`

### Visual/style wrappers

- `ImGui::StyleColorsDark(...)`
- `ImGui::StyleColorsLight(...)`
- `ImGui::SameLine()`
- `ImGui::SetItemDefaultFocus()`

### Container wrappers

- `ImGui::Begin(...)`
- `ImGui::End()`
- `ImGui::BeginMainMenuBar()`
- `ImGui::EndMainMenuBar()`
- `ImGui::BeginMenu(...)`
- `ImGui::EndMenu()`
- `ImGui::DockSpaceOverViewport(...)`
- `ImGui::BeginPopup(...)`
- `ImGui::EndPopup()`
- `ImGui::BeginCombo(...)`
- `ImGui::EndCombo()`
- `ImGui::BeginTable(...)`
- `ImGui::EndTable()`

### Widget wrappers

- `ImGui::Text(...)`
- `ImGui::TextUnformatted(...)`
- `ImGui::Button(...)`
- `ImGui::Checkbox(...)`
- `ImGui::InputText(...)`
- `ImGui::InputDouble(...)`
- `ImGui::RadioButton(...)`
- `ImGui::Selectable(...)`
- `ImGui::OpenPopup(...)`
- `ImGui::TableSetupColumn(...)`
- `ImGui::TableHeadersRow()`
- `ImGui::TableNextRow()`
- `ImGui::TableSetColumnIndex(...)`

### Draw-list wrappers to plan for in a larger app

These are not heavily used in this repo today, but they are common in richer UI applications and should be part of a wrapper plan:

- `ImGui::GetWindowDrawList()`
- `ImGui::GetForegroundDrawList()`
- `ImGui::GetBackgroundDrawList()`
- `ImDrawList::AddLine(...)`
- `ImDrawList::AddRect(...)`
- `ImDrawList::AddRectFilled(...)`
- `ImDrawList::AddCircle(...)`
- `ImDrawList::AddCircleFilled(...)`
- `ImDrawList::AddText(...)`
- `ImDrawList::AddImage(...)`
- `ImDrawList::PathLineTo(...)`
- `ImDrawList::PathStroke(...)`
- `ImDrawList::PathFillConvex(...)`

## A Good Minimal Wrapper Layer

If you do not want to wrap all of ImGui, start with these app-level helpers:

- `uiGetIO(...)`
- `uiPushStyleColor(...)`
- `uiPopStyleColor(...)`
- `uiPushStyleVar(...)`
- `uiPopStyleVar(...)`
- `uiGetCursorPos(...)`
- `uiGetContentRegionAvail(...)`
- `uiSetCursorPos(...)`
- `uiSameLine(...)`
- `uiBeginPanel(...)`
- `uiEndPanel(...)`
- `uiBeginMenuBar(...)`
- `uiEndMenuBar(...)`
- `uiBeginMenu(...)`
- `uiEndMenu(...)`
- `uiGetWindowDrawList(...)`
- `uiDrawLine(...)`
- `uiDrawRect(...)`
- `uiDrawText(...)`
- `uiDrawImage(...)`

And keep all user-facing controls behind wrappers like:

- `DisplayText(...)`
- `ActionButton(...)`
- `TextField(...)`
- `NumericField(...)`
- `CheckboxField(...)`
- `SelectField(...)`

That gives you one place to centralize headless policy.

## Recommended Headless Policy

For most apps, the cleanest rules are:

- style wrappers:
  - no-op in headless mode

- cursor/layout wrappers:
  - use authored or computed layout data
  - otherwise return conservative defaults

- container wrappers:
  - treat panel/menu open state as app state
  - do not depend on actual ImGui begin/end semantics in headless mode

- widget wrappers:
  - register stable ids
  - consume scripted actions
  - update registry/query state
  - skip real ImGui calls

## Practical Rule of Thumb

If a call:

- changes how something looks
  - wrap it and no-op it in headless mode

- asks ImGui for live geometry or context state
  - wrap it and provide a fallback

- affects business logic through a user interaction
  - wrap it and make it test-driven

## Best Outcome

The best end state is:

- panel code is mostly free of raw `ImGui::...` calls
- the wrapper layer owns headless behavior
- desktop and headless paths both execute the same panel logic
- only the rendering/integration details change between modes

That is what makes a large app maintainable once headless UI testing matters.
