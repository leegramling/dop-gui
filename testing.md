# Testing

This file is the current demo walkthrough for building and testing `dop-gui`.

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
