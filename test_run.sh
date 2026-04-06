#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build/dop-gui}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
DEFAULT_SCRIPT="$ROOT_DIR/tests/desktop_bootstrap.json5"
MODE="${1:-script}"

build_app() {
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    cmake --build "$BUILD_DIR" -j "${BUILD_JOBS:-8}"
}

run_script_mode() {
    local script_file="${2:-$DEFAULT_SCRIPT}"
    shift 2 || true
    exec "$BUILD_DIR/dop-gui" --script "$script_file" --stay-open "$@"
}

run_command_mode() {
    local command_text="${2:-state.reset.bootstrap}"
    shift 2 || true
    exec "$BUILD_DIR/dop-gui" --command "$command_text" --stay-open "$@"
}

usage() {
    cat <<'EOF'
Usage:
  ./test_run.sh script [script_file] [extra dop-gui args...]
  ./test_run.sh command [command_text] [extra dop-gui args...]
  ./test_run.sh live-bg [extra dop-gui args...]
  ./test_run.sh live-grid-off [extra dop-gui args...]
  ./test_run.sh live-scene-cubes [extra dop-gui args...]
  ./test_run.sh live-scene-create [extra dop-gui args...]
  ./test_run.sh live-regression [extra dop-gui args...]

Examples:
  ./test_run.sh
  ./test_run.sh script tests/desktop_bootstrap.json5
  ./test_run.sh script tests/smoke_cli.json5
  ./test_run.sh script tests/mutate_cli.json5
  ./test_run.sh script tests/mutate_cli.json5 --startup-delay-ms 5000
  ./test_run.sh command state.reset.bootstrap
  ./test_run.sh command data.scene.object.bootstrap_triangle.translate=1.0,0.0,2.0 --startup-delay-ms 5000
  ./test_run.sh command view.camera.set_pose=1.0,-3.0,2.0,0.0,0.0,0.0,0.0,0.0,1.0
  ./test_run.sh script tests/mutate_cli.json5 -f 600
  ./test_run.sh live-bg
  ./test_run.sh live-grid-off
  ./test_run.sh live-scene-cubes
  ./test_run.sh live-scene-create
  ./test_run.sh live-regression

Notes:
  - The script configures and builds the app before launch.
  - It uses --stay-open so startup commands/scripts are applied and the desktop app remains open.
  - Pass --startup-delay-ms 5000 to show the default view for 5 seconds before a startup script or command applies.
  - Extra arguments are passed directly to dop-gui.
EOF
}

case "$MODE" in
    script)
        build_app
        run_script_mode "$@"
        ;;
    command)
        build_app
        run_command_mode "$@"
        ;;
    live-bg)
        build_app
        exec "$BUILD_DIR/dop-gui" --script "$ROOT_DIR/tests/live_ui_bg_blue.json5" --stay-open --startup-delay-ms 5000 "${@:2}"
        ;;
    live-grid-off)
        build_app
        exec "$BUILD_DIR/dop-gui" --script "$ROOT_DIR/tests/live_ui_grid_off.json5" --stay-open --startup-delay-ms 5000 "${@:2}"
        ;;
    live-scene-cubes)
        build_app
        exec "$BUILD_DIR/dop-gui" --script "$ROOT_DIR/tests/live_ui_scene_cubes.json5" --stay-open --startup-delay-ms 5000 "${@:2}"
        ;;
    live-scene-create)
        build_app
        exec "$BUILD_DIR/dop-gui" --script "$ROOT_DIR/tests/live_ui_scene_create.json5" --stay-open "${@:2}"
        ;;
    live-regression)
        build_app
        exec "$BUILD_DIR/dop-gui" --script "$ROOT_DIR/tests/live_regression.json5" --stay-open "${@:2}"
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "Unknown mode: $MODE" >&2
        usage >&2
        exit 1
        ;;
esac
