# Declarative Object Properties UI

This document explains the two UI authoring paths in this repo:

1. declarative JSON5 UI
2. hand-coded panel UI

The current system supports menus, authored panels, Yoga-backed layout slots, wrapped widgets, command callbacks, and queryable widget registration.

## Overview

The declarative UI path starts in [ui/layout.json5](/home/lgramling/dev/dop-gui/ui/layout.json5).

At startup:

1. [App.cpp](/home/lgramling/dev/dop-gui/src/App.cpp) loads the JSON5 file into `UiLayoutState`
2. panel specs become `UiPanelState`, `UiWidgetSpecState`, and `UiFlexNodeState` in [AppState.h](/home/lgramling/dev/dop-gui/src/AppState.h)
3. runtime panel trees are built by [UiPanelTree.cpp](/home/lgramling/dev/dop-gui/src/UiPanelTree.cpp)
4. Yoga layout slots are built by [UiLayoutUtils.cpp](/home/lgramling/dev/dop-gui/src/UiLayoutUtils.cpp) and [YogaLayout.cpp](/home/lgramling/dev/dop-gui/src/YogaLayout.cpp)
5. wrapped widgets in [Widgets.cpp](/home/lgramling/dev/dop-gui/src/Widgets.cpp) render and register widget state

The hand-coded path skips the authored widget spec and writes layout/render code directly in the panel class. The current example is [NewShapePanel.cpp](/home/lgramling/dev/dop-gui/src/NewShapePanel.cpp).

## JSON5 UI Format

The top-level JSON5 file currently contains:

- `menus`
- `panels`

Example top-level shape:

```json5
{
  menus: [ ... ],
  panels: [ ... ],
}
```

### Menus

Each menu contains:

- `label`
- `items`

Each menu item contains:

- `label`
- `command`

Example:

```json5
{
  label: "Scene",
  items: [
    { label: "Create", command: "ui.panel.open=panel-new-shape" },
    { label: "cubes", command: "scene.load.cubes" },
    { label: "Shapes", command: "scene.load.shapes" },
  ],
}
```

These menu items are also registered as wrapped widgets at runtime, which is why they can be driven by commands like:

```text
ui.test.click.menuitem-scene-cubes
```

and queried through:

```text
ui.widget.menuitem-scene-cubes
```

### Panels

Each panel contains:

- `label`
- `open`
- `closable`
- `flags`
- `layout`
- optional `flexLayout`
- `widgets`

The `layout` field is the panel window rectangle:

```json5
layout: { x: 408, y: 48, w: 360, h: 560 }
```

That rectangle is the authored panel window position/size, not the individual widget layout.

### Properties Panel Example

The Properties panel in [ui/layout.json5](/home/lgramling/dev/dop-gui/ui/layout.json5) is a good example of the authored format:

```json5
{
  label: "Properties",
  open: true,
  closable: true,
  flags: ["NoCollapse"],
  layout: { x: 408, y: 48, w: 360, h: 560 },
  flexLayout: {
    type: "column",
    gap: 8,
    children: [
      {
        type: "row",
        gap: 8,
        children: [
          { labelFor: "selected-object", width: 116, height: 24 },
          { widget: "selected-object", flex: 1, height: 24 },
        ],
      },
      { widget: "selected-object-summary", width: 360, height: 20 },
      {
        type: "row",
        gap: 8,
        children: [
          { labelFor: "position-x", width: 116, height: 24 },
          { widget: "position-x", flex: 1, height: 24 },
        ],
      },
    ],
  },
  widgets: [
    {
      id: "selected-object",
      slotId: "panel-selected-object",
      labelSlot: "panel-properties-selected-object-label",
      type: "combo",
      label: "Selected Object",
      bind: "scene.selectedObjectId",
      onChange: "scene.select_object"
    },
    {
      id: "position-x",
      slotId: "input-properties-position-x",
      type: "input_double",
      label: "Location X",
      bind: "scene.selected.position.x",
      unit: "m",
      precision: 2
    },
  ],
}
```

The important split is:

- `flexLayout` defines where UI pieces go
- `widgets` defines what those UI pieces are and what state/commands they connect to

## How The Layout Section Works

The `flexLayout` subtree is authored in JSON5 but executed through Yoga.

The layout node fields are:

- `type`
  - `"row"` or `"column"`
- `gap`
- `width`
- `height`
- `flex`
- `children`
- `widget`
- `labelFor`
- `slot`

Examples:

- `{ widget: "position-x", flex: 1, height: 24 }`
- `{ labelFor: "position-x", width: 116, height: 24 }`
- `{ slot: "panel-theme-label", width: 360, height: 20 }`

These are parsed into `UiFlexNodeState` in [AppState.h](/home/lgramling/dev/dop-gui/src/AppState.h).

Then [UiLayoutUtils.cpp](/home/lgramling/dev/dop-gui/src/UiLayoutUtils.cpp):

- converts each flex node into a `YogaLayout::Spec`
- resolves each node into a stable slot id
- uses the widget `slotId` and `labelSlot` when present
- falls back to generated slot names when needed

The key helper functions are:

- `buildYogaLayoutSpec(...)`
- `collectYogaSlotIds(...)`
- `labelSlotForWidget(...)`
- `valueSlotForWidget(...)`

## How Yoga Calculates Positions

[YogaLayout.cpp](/home/lgramling/dev/dop-gui/src/YogaLayout.cpp) is the runtime bridge from authored flex spec to concrete rectangles.

The basic flow is:

1. `setLayout(spec)`
   - creates a Yoga node tree
   - applies width/height/flex/gap/direction styles

2. `resize(x, y, width, height)`
   - calls `YGNodeCalculateLayout(...)`
   - computes final rectangles for every named layout node

3. `computeRects(...)`
   - stores the final result in `_rects`
   - each rect becomes a `UiLayoutRectState`

The stored rectangle looks like:

```cpp
UiLayoutRectState{
    .x = nodeAbsX,
    .y = nodeAbsY,
    .width = width,
    .height = height,
    .enabled = true,
}
```

Those rects are then registered into `UiState.layoutSlots` through:

- `registerLayoutSlot(...)`
- `registerLayoutSlots(...)`

So after Yoga runs, the panel has a stable set of named slot rectangles such as:

- `panel-selected-object`
- `panel-properties-selected-object-label`
- `input-properties-position-x`

## How Widgets Use The Computed Positions

Once Yoga has produced slot rectangles, widget renderers use:

- `setNextWidgetLayoutIfPresent(...)`

That function pushes the computed `UiLayoutRectState` into `UiState.nextWidgetLayout`.

Then the wrapped widget in [Widgets.cpp](/home/lgramling/dev/dop-gui/src/Widgets.cpp) reads that pending layout and applies it before calling ImGui.

The real wrapped widget code also handles test registration and query state, but the core layout idea is much smaller:

### Trimmed Button Example

```cpp
bool Button(UiState& uiState, const char* id, const char* label)
{
    const auto requestedLayout = consumeNextLayout(uiState);
    applyLayoutRect(requestedLayout);
    return ImGui::Button(label);
}
```

### Trimmed Input Example

```cpp
std::string Input(UiState& uiState, const char* id, const char* label, const std::string& value)
{
    std::array<char, 256> buffer{};
    std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
    const auto requestedLayout = consumeNextLayout(uiState);
    applyLayoutRect(requestedLayout);
    ImGui::InputText(label, buffer.data(), buffer.size());
    return buffer.data();
}
```

### Trimmed Checkbox Example

```cpp
bool Checkbox(UiState& uiState, const char* id, const char* label, bool& value)
{
    const auto requestedLayout = consumeNextLayout(uiState);
    applyLayoutRect(requestedLayout);
    return ImGui::Checkbox(label, &value);
}
```

So the important layout contract is:

1. Yoga computes named slot rects
2. the panel selects the slot for the next widget
3. the wrapped widget applies that rect before calling ImGui

## How Callbacks Are Given In JSON5

The authored widget spec supports callback-like command fields:

- `onChange`
- `onClick`

Examples from [ui/layout.json5](/home/lgramling/dev/dop-gui/ui/layout.json5):

```json5
{ id: "display-grid", type: "checkbox", onChange: "ui.grid.set_visible" }
{ id: "scene-select", type: "combo", onChange: "scene.load" }
{ id: "theme-light", type: "radio", arg: "light", onClick: "ui.theme.set" }
{ id: "selected-object", type: "combo", onChange: "scene.select_object" }
```

These are not direct C++ function pointers. They are command names.

At runtime, [UiPanelTree.cpp](/home/lgramling/dev/dop-gui/src/UiPanelTree.cpp) turns widget changes into queued command strings:

- `bindCheckbox(...)` queues `onChange=true/false`
- `bindStringInput(...)` queues `onChange=<text>`
- `bindStringCombo(...)` queues `onChange=<selected value>`
- `bindRadioChoice(...)` queues `onClick=<arg>`

The shared mechanism is:

```cpp
queueUiCommand(state.ui, node.spec.onChange, *value);
```

or:

```cpp
queueUiCommand(state.ui, node.spec.onClick, node.spec.arg);
```

That means the JSON5 callback is really:

- a declarative command route
- with the widget's current value or argument attached

## How Callback Routes Are Defined In Code

The JSON5 only declares the command names. The actual meaning lives in command handling code.

For example:

- `scene.load`
- `scene.select_object`
- `ui.grid.set_visible`
- `ui.theme.set`

are resolved through the command system in the app, not in the panel JSON.

So the division of responsibility is:

- JSON5 says what event should happen
- panel tree decides when to queue it
- command handling code decides what it means

This is why the UI spec stays small and declarative.

## Widget Registration And Other Runtime Metadata

Wrapped widgets also register themselves at runtime in `UiState.registry`.

The stored widget data includes:

- `label`
- `panelId`
- `widgetId`
- `type`
- `textValue`
- `boolValue`
- `layout`

This registration is what makes these possible:

- `ui.widget.<id>` queries
- `ui.panel.<panel-id>.widget.<id>` queries
- headless UI tests
- script-driven widget interactions such as `ui.test.*`

Menu items are also registered, which is why menu-driven tests can query them.

Layout slots are registered separately in `UiState.layoutSlots`, which is what powers:

- `ui.layout.slot.<panel>.<slot>`

So there are really two related runtime registration surfaces:

1. widget registry
2. layout slot registry

## How The Properties Panel Is Bound

The authored Properties panel is implemented in [PropertiesPanel.cpp](/home/lgramling/dev/dop-gui/src/PropertiesPanel.cpp).

Its `init(...)` does not manually place each widget. Instead it:

1. builds a `UiPanelTree` from the authored panel spec
2. assigns special renderers where needed
3. binds authored widgets to application state

Examples:

- `bindStringCombo(...)` for `scene.selectedObjectId`
- `bindDoubleInput(...)` for selected object position/rotation/scale values

The selected object numeric bindings are resolved by code such as:

```cpp
double* resolveSelectedObjectDouble(AppState& state, std::string_view bind)
```

So the JSON5 `bind` string:

```json5
bind: "scene.selected.position.x"
```

is connected to real state access by the panel code.

That is an important design point:

- JSON5 declares the binding name
- panel code decides how that binding name maps to live state

## Hand-Coded UI

The hand-coded path is different. The current example is [NewShapePanel.cpp](/home/lgramling/dev/dop-gui/src/NewShapePanel.cpp).

This panel still uses Yoga and wrapped widgets, but it does not use authored `widgets` and `flexLayout` from JSON5.

Instead:

- layout is created directly in C++ in `init()`
- render logic is written directly in `render()`
- callbacks are handled inline in panel code

### Hand-Coded Layout

In `init()`, the panel builds a `YogaLayout::Builder` tree directly:

```cpp
builder.root("new-shape-root", ...);
builder.begin("action-row", rowStyle)
    .item("create-shape", ...)
    .item("cancel", ...)
    .end();
```

The panel also manually lists `_slotIds`.

### Hand-Coded Render

In `render()`, the panel:

1. runs Yoga layout
2. registers the layout slots
3. calls wrapped widgets directly in the desired order

Example:

```cpp
setNextWidgetLayoutIfPresent(state.ui, layout, "color");
_colorHex = TextField(state.ui, "color", "", _colorHex);

setNextWidgetLayoutIfPresent(state.ui, layout, "create-shape");
if (ActionButton(state.ui, "create-shape", "Create Shape"))
{
    ...
}
```

### Hand-Coded Callbacks

The create/cancel behavior is implemented directly in the panel:

- creating a new scene object
- selecting it
- closing the panel
- resetting the form

That logic is written inline in C++, not declared as JSON5 callback command names.

## JSON5 UI Vs Hand-Coded UI

The main difference is where the structure lives.

### JSON5 UI

- panel structure is authored in `ui/layout.json5`
- Yoga layout comes from authored `flexLayout`
- widgets come from authored `widgets`
- callbacks are command names like `onChange` / `onClick`
- panel code mainly binds authored widget ids to state

Best for:

- stable, inspectable panels
- queryable widget surfaces
- declarative layout evolution
- consistent testing

### Hand-Coded UI

- panel structure lives in C++
- Yoga layout is built manually in code
- render order is manual
- callbacks are handwritten inline logic

Best for:

- development examples
- exceptional workflows
- panels whose behavior is still too custom for the authored schema

## Practical Rule

Use JSON5 UI when:

- the panel can be described as a set of standard widgets and bindings
- layout should be authored and queryable
- command routing should remain declarative

Use hand-coded UI when:

- the panel is intentionally custom
- the panel is serving as an example
- the authored schema does not yet cover the required behavior

That split is already visible in the current codebase:

- Properties and Scene Info are authored-panel oriented
- New Shape is the hand-coded example panel
