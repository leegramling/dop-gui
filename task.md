# Task

## Active Slice

- [x] Define the first declarative JSON5 flex-layout slice for `Properties`.
- [x] Extend the declarative JSON5 flex-layout path to `Scene Info`.
- [x] Reduce duplicated slot naming in authored flex layout with `widget:` and `labelFor:` references.
- [x] Add panel-scoped runtime widget lookup with `ui.panel.<panel>.widget.<id>` while keeping `ui.widget.<id>` working.
- [x] Convert `Properties` widget ids to shorter panel-local ids while keeping stable layout slot ids and legacy flat widget-query aliases.

## Next Tasks

- [x] Add Doxygen-style doc comments to current public classes, structs, enums, methods, and free functions.
- [x] Add a `HowToAddTestCommands` contributor guide.
- [x] Add the first richer widgets: combo box and radio button.
- [ ] Continue expanding the widget set with additional input variants beyond combo, radio, popup, and table.
- [x] Define `Theme` expansion for named colors and reusable panel/window flags.
- [x] Define panel callback requirements, docking policy, and tear-out expectations.
- [x] Evaluate the local `../vsgLayt` Yoga examples and choose the first adapter approach.
- [x] Define how Yoga `setPos`-style placement will integrate with panel init code.
- [x] Define the first `.gltf` / `.glb` scene-loading slice and the required scene-model growth.
- [x] Define a minimal JSON5 flex-layout vocabulary for authored panels.
- [x] Convert the `Properties` panel layout from builder code to authored JSON5 layout.
- [x] Preserve `ui.layout.slot.*` and `ui.widget.*` query behavior during the layout-source transition.
- [x] Move `Scene Info` from builder code to authored JSON5 flex layout.
- [x] Add widget-aware slot references so authored flex layout no longer has to repeat most value and label slot ids verbatim.
- [x] Convert `Scene Info` widget ids to shorter panel-local ids while preserving panel-scoped queries and legacy flat widget-query aliases during the transition.
- [x] Add panel-scoped UI test commands so shorter local widget ids do not depend on globally flat `ui.test.*` labels.
- [x] Introduce a generic JSON5-built panel tree and move `Properties` onto that path so it no longer owns a panel-local `buildLayout()` implementation.
- [x] Move `Properties` widget rendering dispatch into the built panel tree so the panel render path trends toward `root.render(...)` instead of hand-iterating widget specs.
- [x] Replace the remaining hard-coded selected-object numeric bind `if` chain in `Properties` with a table-driven binding resolver.
- [x] Move `Scene Info` onto the same built panel-tree render path and replace its basic text/string/bool bind `if` chains with table-driven binding maps.
- [x] Add generic panel-tree binder helpers for common text/input/checkbox/combo/radio/double widget patterns so panel classes only keep custom renderers for exceptional widgets.
- [x] Switch both panels to the shared generic panel-tree binder helpers for the common widget cases, leaving only selected-object, popup, and table behavior as custom renderers.
- [x] Move the selected-object controls in both panels onto the shared generic panel-tree combo binder path so popup and table behavior are the only intentional custom renderers left in this feature.
- [ ] Reduce the remaining split between panel-local widget ids and globally flat widget lookup so the declarative UI path can rely primarily on scoped queries and commands.
- [ ] Document and preserve a fallback hand-coded panel path for exceptional panels that still need an `initialize` builder flow and explicit `render` method before they move to JSON5-authored UI.

## Next Slice Candidates

- [ ] Add richer model/view/data query namespaces as those layers are introduced.
- [x] Add `ui.*` query namespaces for panels, menu items, widgets, and theme-facing state.
- [x] Add richer command argument and script-object formats beyond simple numeric path commands.
- [ ] Add more authored widget properties and panel/window options to the JSON5 UI format.
- [x] Add Yoga-backed layout state and queries where layout rects become inspectable.
- [x] Add a `WindowManager` boundary for primary-window ownership and future tear-out callbacks/windows.
- [x] Add test commands and queries for the first richer widgets such as radio groups and combos.
- [ ] Add test commands and queries for popups, tables, and scene-asset widgets.
- [ ] Add scene queries for loaded asset instances and larger scene structures.

## Follow-Up Validation

- [x] Confirm Doxygen coverage is applied consistently to the currently public code surface.
- [x] Confirm the widget expansion still preserves headless/live simulation symmetry.
- [x] Confirm Yoga-based placement can be inspected and tested without relying on ad hoc cursor positioning.
- [x] Confirm the first JSON5-authored flex layout produces the same runtime slot/widget query surface as the current builder-based layout.
- [x] Confirm `Scene Info` and `Properties` both preserve the existing runtime slot/widget query surface after the JSON5 layout transition.
- [ ] Confirm the `WindowManager` can observe ImGui platform window callbacks before any tear-out implementation starts.
- [ ] Confirm the first `.glb` scene can load and be queried from the command/query surface.

## Constraints

- [x] Use the local external `external/vsgImGui/src/imgui` checkout at `v1.91.6-docking`.
- [x] Preserve the current scene-loading and multi-object render behavior while adding UI.
- [x] Keep platform assumptions minimal.
- [x] Avoid runtime GLSL compilation because the local VSG build lacks GLSLang support.
- [x] Use JSON5 for authored command and UI files.
- [x] Do not add Lua-specific architecture unless a later requirement forces it.
- [x] Do not implement full recording/playback yet unless required to establish the command/query seam.

## Done Definition

- [x] Current public interfaces are documented in Doxygen style.
- [x] A `HowToAddTestCommands` guide exists in the repo.
- [ ] The next widget/panel/theme/docking/Yoga/gltf scope is clearly implemented in thin slices.
