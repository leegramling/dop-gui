# Testing

This file is the current demo walkthrough for building, running, and explaining the testable UI pattern in `dop-gui`.

## Build

Linux/macOS:

```bash
cmake -S . -B build/dop-gui -DCMAKE_BUILD_TYPE=Release
cmake --build build/dop-gui -j 8
```

Windows:

```bat
build.bat
```

Environment overrides for `build.bat`:

- `BUILD_DIR`
- `BUILD_TYPE`
- `BUILD_JOBS`

Example:

```bat
set BUILD_TYPE=Debug
set BUILD_JOBS=4
build.bat
```

## How UI Testing Works

The important idea is that our widgets are wrapped. We do not call raw ImGui directly from most panels. Instead, panel code goes through helpers in [Widgets.cpp](/home/lgramling/dev/dop-gui/src/Widgets.cpp).

Those wrappers do three jobs:

1. register stable widget ids in `UiState.registry`
2. consume scripted test actions in headless mode
3. call real ImGui in desktop mode

The layout and Yoga code is useful for panel placement, but it is not the core testing idea. The testing pattern is much simpler than the full file makes it look.

## Simplified Widget Examples

Below are stripped-down examples of the pattern behind `Button`, `Checkbox`, and `Input`.

### Button

```cpp
bool Button(UiState& uiState, const char* id)
{
    // Ensure the widget exists in the registry so tests and queries can find it.
    auto& widget = ensureWidget(uiState, id, "button");

    // In headless tests, scripted commands can simulate a click.
    const bool clicked = consumeClick(uiState, id);
    widget.boolValue = clicked;

    // Headless path: do not call ImGui, just return the scripted result.
    if (uiState.testMode)
    {
        return clicked;
    }

    // Desktop path: ask ImGui if the user clicked the real button this frame.
    const bool result = clicked || ImGui::Button(id);
    return result;
}
```

What matters:

- the widget is always registered
- headless mode returns a value from scripted actions
- desktop mode returns a value from real ImGui interaction

### Checkbox

```cpp
bool Checkbox(UiState& uiState, const char* id, bool& value)
{
    auto& widget = ensureWidget(uiState, id, "checkbox");

    // In tests, a script can inject a boolean change.
    const bool simulatedChange = consumeBool(uiState, id, value);
    widget.boolValue = value;

    // Headless path: update the bound value and return whether it changed.
    if (uiState.testMode)
    {
        return simulatedChange;
    }

    // Desktop path: let ImGui render and mutate the bound bool.
    const bool changed = ImGui::Checkbox(id, &value);
    widget.boolValue = value;
    return simulatedChange || changed;
}
```

What matters:

- headless mode mutates the bound value without a window
- desktop mode mutates the same bound value through ImGui
- queries can later read the current checkbox state from `widget.boolValue`

### Input

```cpp
std::string Input(UiState& uiState, const char* id, const std::string& value)
{
    auto& widget = ensureWidget(uiState, id, "input");

    // Start from the model value owned by the panel/app state.
    std::string currentValue = value;

    // In tests, a script can inject replacement text.
    consumeText(uiState, id, currentValue);
    widget.textValue = currentValue;

    // Headless path: return the injected text directly.
    if (uiState.testMode)
    {
        return currentValue;
    }

    // Desktop path: copy to an ImGui buffer and render a real text field.
    std::array<char, 256> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", currentValue.c_str());
    ImGui::InputText(id, buffer.data(), buffer.size());

    // Store the final visible value back into the registry.
    widget.textValue = buffer.data();
    return widget.textValue;
}
```

What matters:

- the same wrapper works for both automated tests and live UI
- headless tests bypass ImGui entirely
- the return value is what the panel should store back into app state

## Headless Path

Headless mode is enabled with:

```bash
./build/dop-gui/dop-gui --ui-test-mode
```

At startup, [App.cpp](/home/lgramling/dev/dop-gui/src/App.cpp) sets:

```cpp
_state.ui.testMode = uiTestMode;
if (_state.ui.testMode) _uiManager->evaluate(_state);
```

That means:

- no desktop window is required
- panels are still evaluated
- widget wrappers still run
- widget state is still registered in `UiState.registry`

So a headless test can:

1. queue a fake action such as `set_text` or `click`
2. evaluate the UI tree
3. inspect widget state through a query

### Headless Example

This script drives the New Shape dialog entirely without a desktop:

```json5
{
  actions: [
    { command: "ui.test.click.menuitem-scene-create" },
    { command: "ui.test.panel.panel-new-shape.set_text.shape-kind=Sphere" },
    { command: "ui.test.panel.panel-new-shape.set_text.position-x=1.50 m" },
    { command: "ui.test.panel.panel-new-shape.click.create-shape" },
    { query: "ui.panel.panel-new-shape.widget.shape-kind" },
    { query: "data.scene.object.sphere_1" },
  ],
}
```

Run it with:

```bash
./build/dop-gui/dop-gui --ui-test-mode --script tests/ui_new_shape_cli.json5
```

### How `Input` Behaves in Headless Mode

Using the real code in [Widgets.cpp](/home/lgramling/dev/dop-gui/src/Widgets.cpp):

```cpp
std::string currentValue = value;
consumeText(uiState, id, currentValue);
widget.textValue = currentValue;
if (uiState.testMode)
{
    return currentValue;
}
```

This means:

- the incoming model value is copied into `currentValue`
- a scripted `set_text` command can replace it
- the registry stores the result in `widget.textValue`
- the wrapper returns that text to the panel immediately

So if the script says:

```text
ui.test.panel.panel-new-shape.set_text.position-x=1.50 m
```

then the next `Input(...)` evaluation returns `"1.50 m"` even though no ImGui widget was rendered.

## Desktop Path

Desktop mode is the normal app run:

```bash
./build/dop-gui/dop-gui
```

In this path:

- a real VSG window is created
- ImGui is initialized
- the same widget wrappers call real ImGui functions

For example, the desktop half of `Input()` is:

```cpp
std::array<char, 256> buffer{};
std::snprintf(buffer.data(), buffer.size(), "%s", currentValue.c_str());
ImGui::InputText(label.c_str(), buffer.data(), buffer.size());
widget.textValue = buffer.data();
return widget.textValue;
```

That means:

- the panel passes in the current model value
- ImGui lets the user edit it with the keyboard
- the wrapper stores the visible result in the widget registry
- the wrapper returns the edited value to the panel

So headless and desktop both follow the same contract:

- input comes in from app state
- wrapper evaluates the widget
- final value comes back out
- widget registry keeps a queryable copy

The only difference is where the edit came from:

- headless: scripted action such as `set_text`
- desktop: real ImGui input from the user

## Query Surface

Queries read the widget registry out of [Query.cpp](/home/lgramling/dev/dop-gui/src/Query.cpp).

Useful query forms:

- `ui.widgets`
- `ui.widget.<widget-id>`
- `ui.panel.<panel-id>.widget.<widget-id>`

Examples:

```bash
./build/dop-gui/dop-gui --ui-test-mode --query ui.widgets
./build/dop-gui/dop-gui --ui-test-mode --query ui.widget.panel-display-grid
./build/dop-gui/dop-gui --ui-test-mode --query ui.panel.panel-new-shape.widget.shape-kind
```

The important part is that queries do not care whether the widget was evaluated in headless mode or desktop mode. They read the same `WidgetState` data:

- `type`
- `boolValue`
- `textValue`
- `layout`
- `panelId`

### `Input` Query Example

If a panel contains:

```cpp
shapeKind = Input(state.ui, "shape-kind", "Shape Kind", shapeKind);
```

then after a headless script sets:

```text
ui.test.panel.panel-new-shape.set_text.shape-kind=Sphere
```

the query:

```bash
./build/dop-gui/dop-gui --ui-test-mode --script tests/ui_new_shape_cli.json5
```

will include a result for:

```text
ui.panel.panel-new-shape.widget.shape-kind
```

and that widget state will report the current text value as `Sphere`.

## Headless Test Demo

Run the focused automated suite:

```bash
ctest --test-dir build/dop-gui --output-on-failure
```

Useful focused runs:

```bash
ctest --test-dir build/dop-gui --output-on-failure -R "dop_gui_ui_registry|dop_gui_ui_background_input|dop_gui_ui_grid_toggle"
ctest --test-dir build/dop-gui --output-on-failure -R "dop_gui_ui_new_shape_panel|dop_gui_ui_scene_create"
```

You can also run individual scripted headless flows directly:

```bash
./build/dop-gui/dop-gui --ui-test-mode --script tests/ui_new_shape_cli.json5
./build/dop-gui/dop-gui --ui-test-mode --script tests/regression_cli.json5
./build/dop-gui/dop-gui --ui-test-mode --query ui.widgets
```

## Live GUI Demo

Desktop helper flows are provided through `test_run.sh`:

```bash
./test_run.sh live-bg
./test_run.sh live-grid-off
./test_run.sh live-scene-cubes
./test_run.sh live-scene-create
./test_run.sh live-regression
```

The most useful end-to-end demo checks are:

1. `./test_run.sh live-regression`
2. `./test_run.sh live-scene-create`

Expected `live-regression` behavior:

1. app opens
2. scene switches to cubes
3. grid hides
4. background turns blue
5. app exits

Expected `live-scene-create` behavior:

1. app opens
2. `Scene -> Create` opens `New Shape`
3. one shape is created
4. dialog is reopened
5. `Cancel` closes it
6. app exits

## Demo Notes

- Headless `ctest` coverage is the authoritative automated safety net.
- Live GUI scripts require a real desktop session; they are not expected to pass in a headless sandbox.
- `imgui.ini` is intentionally local-only state and should not be used as a demo artifact.
- For adding new command/query/UI-test surfaces, use [HowToAddTestCommands.md](/home/lgramling/dev/dop-gui/HowToAddTestCommands.md).
