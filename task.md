# Task

## Active Slice

- [ ] Define the first declarative JSON5 flex-layout slice for `Properties`.

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
- [ ] Define a minimal JSON5 flex-layout vocabulary for authored panels.
- [ ] Convert the `Properties` panel layout from builder code to authored JSON5 layout.
- [ ] Preserve `ui.layout.slot.*` and `ui.widget.*` query behavior during the layout-source transition.

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
- [ ] Confirm the first JSON5-authored flex layout produces the same runtime slot/widget query surface as the current builder-based layout.
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
