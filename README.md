# dop-gui

`dop-gui` is a VulkanSceneGraph application project aimed at:

- data oriented programming
- functional-style C++20
- spec-driven development
- small, reviewable implementation steps

This repository is currently being bootstrapped from documentation first so we can make deliberate architectural choices before code lands.

The bootstrap scene now comes from [bootstrap_scene.json5](/home/lgramling/dev/dop-gui/scenes/bootstrap_scene.json5), which seeds the app with multiple authored shape records for testing and inspection.

## Development Style

We want a codebase that favors:

- plain data layouts over deep object hierarchies
- explicit state transforms over hidden mutation
- free functions and narrow modules over large classes
- deterministic frame/update pipelines
- testable subsystems with clear data ownership

A practical interpretation for this project is:

- keep runtime state in compact structs
- move behavior into stateless or mostly stateless C++20 functions
- isolate Vulkan/VSG integration from domain data and domain transforms
- prefer pipelines like `input -> transform -> render data -> submit`
- make system boundaries visible in the spec before implementation

## Spec-Driven Workflow

We will drive work through four small markdown files in repo root:

- [`spec.md`](/home/lgramling/dev/dop-gui/spec.md): stable top-level product and architecture specification
- [`agents.md`](/home/lgramling/dev/dop-gui/agents.md): working agreements for Codex agents and human collaborators
- [`plan.md`](/home/lgramling/dev/dop-gui/plan.md): current phased roadmap
- [`task.md`](/home/lgramling/dev/dop-gui/task.md): the next small implementation steps
- [`notes.md`](/home/lgramling/dev/dop-gui/notes.md): architecture notes, dependency findings, and decisions

Recommended loop:

1. Start or switch to a dedicated feature branch for the slice.
2. Capture stable application and architecture intent in [`spec.md`](/home/lgramling/dev/dop-gui/spec.md).
3. Capture branch-specific intent and constraints in [`notes.md`](/home/lgramling/dev/dop-gui/notes.md).
4. Convert the next milestone into concrete steps in [`plan.md`](/home/lgramling/dev/dop-gui/plan.md).
5. Pull only the smallest executable slice into [`task.md`](/home/lgramling/dev/dop-gui/task.md).
6. Use [`agents.md`](/home/lgramling/dev/dop-gui/agents.md) to keep prompts and implementation behavior aligned.
7. Implement one slice, verify it, then update the docs before moving on.

## Proposed Application Direction

Initial target:

- a small VulkanSceneGraph desktop application
- optional `vsgImGui` overlay for tools/debug UI
- data-first scene representation
- functional systems for scene update, input mapping, and render extraction

Suggested early module split:

- `app/`: startup, main loop, configuration, dependency wiring
- `scene/`: pure data for entities, transforms, cameras, tags, selection
- `systems/`: update functions operating on scene data
- `render/`: translation from scene data to VSG structures
- `ui/`: ImGui tools and inspection panels
- `io/`: scene loading, serialization, config

## Local Dependencies

Dependencies available in `../vsg_deps`:

- `VulkanSceneGraph`
- `vsgXchange`
- `vsgImGui`
- `vsgExamples`
- `install` tree with built headers and libraries

These local paths should be treated as the source of truth for initial integration work on this machine.

## Local External vsgImGui

This project currently expects a local external `vsgImGui` checkout at [`external/vsgImGui`](/home/lgramling/dev/dop-gui/external/vsgImGui), but that directory is intentionally not committed to this repository.

Current dependency state:

- parent repo cloned from local source `/home/lgramling/dev/vsg_deps/vsgImGui`
- `imgui` cloned from local source `/home/lgramling/dev/vsg_deps/vsgImGui/src/imgui`
- `implot` cloned from local source `/home/lgramling/dev/vsg_deps/vsgImGui/src/implot`
- `imgui` checked out at tag `v1.91.6-docking`

This gives us a project-local docking-capable ImGui integration without depending on a network fetch during bootstrap.

Build validation performed:

```bash
cmake -S external/vsgImGui -B build/vsgImGui \
  -DCMAKE_PREFIX_PATH=/home/lgramling/dev/vsg_deps/install \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/vsgImGui -j 8
```

Result:

- configure passed
- build passed
- output library: `build/vsgImGui/lib/libvsgImGui.a`

## ImGui Docking Status

The vendored ImGui inside `../vsg_deps/vsgImGui/src/imgui` is not checked out on a dedicated docking branch. The `vsgImGui` repository is currently on `master`, and the local ImGui headers contain comments referencing docking support but do not expose the typical docking entry points such as `DockSpace()` or `ImGuiConfigFlags_DockingEnable`.

Working conclusion:

- this checkout is standard upstream `master`
- it contains some branch-sync comments and shared internals
- it should not be assumed to be a full docking build

If docking is required, we should either:

- switch `vsgImGui` to an ImGui docking-capable revision intentionally, or
- vendor a known docking-enabled ImGui snapshot and verify the API surface explicitly

## Prompt Packs And Command Aliases

This repo now includes prompt specs in [`prompts/`](/home/lgramling/dev/dop-gui/prompts). They are not a guaranteed built-in slash-command registration mechanism by themselves, but they give us a version-controlled command vocabulary similar to `spec-kit`.

Current prompt files:

- [`prompts/spec-arch.md`](/home/lgramling/dev/dop-gui/prompts/spec-arch.md)
- [`prompts/spec-init.md`](/home/lgramling/dev/dop-gui/prompts/spec-init.md)
- [`prompts/spec-plan.md`](/home/lgramling/dev/dop-gui/prompts/spec-plan.md)
- [`prompts/spec-task.md`](/home/lgramling/dev/dop-gui/prompts/spec-task.md)
- [`prompts/spec-review.md`](/home/lgramling/dev/dop-gui/prompts/spec-review.md)

Recommended usage pattern:

- treat the file name as the command alias, such as `$spec-init`
- paste the file contents into a prompt, or ask Codex to follow that prompt file
- keep the prompts small, opinionated, and tied to repo artifacts like `plan.md` and `task.md`
- make `$spec-init` responsible for creating or selecting the feature branch before planning starts

Example:

```text
Use $spec-plan from prompts/spec-plan.md and update plan.md for the next milestone.
```

If we later want a more formal command surface, we can add one of these:

- a repo-local Codex skill that maps named workflows to prompt files
- a local plugin scaffold that exposes project-specific tools or command entry points
- wrapper shell scripts that open the prompt files and inject repo context

## Near-Term Goal

The next milestone is to create a minimal VSG application skeleton that:

- builds against the local `../vsg_deps/install` tree
- links against the local external `external/vsgImGui` build when UI support is enabled
- opens a window
- renders a simple scene
- leaves clear seams for data-oriented scene state and later ImGui tooling

## Build

```bash
cmake -S . -B build/dop-gui -DCMAKE_BUILD_TYPE=Release
cmake --build build/dop-gui -j 8
ctest --test-dir build/dop-gui --output-on-failure
```

Binary:

- `build/dop-gui/dop-gui`
- `./test_run.sh` helper for startup script or startup command desktop runs

Quick probe:

```bash
./build/dop-gui/dop-gui -f 1
./build/dop-gui/dop-gui scenes/bootstrap_scene.json5
./build/dop-gui/dop-gui scenes/cubes.json5
./test_run.sh
./test_run.sh script tests/desktop_bootstrap.json5
./test_run.sh script tests/smoke_cli.json5
./test_run.sh script tests/mutate_cli.json5 --startup-delay-ms 5000
./test_run.sh command state.reset.bootstrap
./test_run.sh command data.scene.object.bootstrap_triangle.translate=1.0,0.0,2.0
```

Current result in this session:

- build succeeded
- `ctest` smoke tests succeeded
- shaders are compiled during the CMake build with `glslangValidator`
- runtime probe failed in the sandboxed environment because XCB window creation is unavailable
- reported error: `Failed to create Window, unable to establish xcb connection`
- `--stay-open` desktop launch paths and `test_run.sh` are wired, but they need a real desktop/XCB session to run successfully
- `--startup-delay-ms <ms>` can delay startup commands or scripts so the default scene is visible before automation applies
- the first positional argument can be a scene file, for example `./build/dop-gui/dop-gui scenes/bootstrap_scene.json5`
- example alternate scene: `./build/dop-gui/dop-gui scenes/cubes.json5`

Why shaders are built ahead of time:

- the local VulkanSceneGraph install was not compiled with GLSLang support
- runtime-generated VSG shader paths, such as `vsg::Builder`, fail in this environment
- `dop-gui` therefore compiles its GLSL sources to SPIR-V during the build and loads `.spv` files at runtime

## CLI Testing Surface

The first test-facing interface is CLI-driven.

Direct queries:

```bash
./build/dop-gui/dop-gui --query window.size
./build/dop-gui/dop-gui --query view.window.size
./build/dop-gui/dop-gui --query scene.objects
./build/dop-gui/dop-gui --query scene.object.bootstrap_triangle
./build/dop-gui/dop-gui --query scene.object.transform.bootstrap_triangle
./build/dop-gui/dop-gui --query data.scene.object.properties.bootstrap_triangle
./build/dop-gui/dop-gui --query camera.pose
./build/dop-gui/dop-gui --query view.camera.pose
./build/dop-gui/dop-gui --query data.scene.object.bootstrap_triangle
./build/dop-gui/dop-gui --query runtime.capabilities
./build/dop-gui/dop-gui --query help
```

Direct commands:

```bash
./build/dop-gui/dop-gui --command help
./build/dop-gui/dop-gui --command noop
./build/dop-gui/dop-gui --command state.reset.bootstrap
./build/dop-gui/dop-gui --command data.scene.object.bootstrap_triangle.translate=1.0,0.0,2.0
./build/dop-gui/dop-gui --command view.camera.set_pose=1.0,-3.0,2.0,0.0,0.0,0.0,0.0,0.0,1.0
```

Batch script mode:

```bash
./build/dop-gui/dop-gui --script tests/smoke_cli.json5
./build/dop-gui/dop-gui --script tests/mutate_cli.json5
```

Current behavior:

- command, query, and script results are emitted as JSON-compatible structured output
- JSON5-style authored files are supported through a constrained first parser
- the current script loader supports string arrays for `commands` and `queries`
- non-window queries can run without XCB access
- mutation commands can update `AppState` in headless mode and later queries in the same process can observe those changes
- when the renderer is initialized, successful commands now trigger a `VsgVisualizer` sync from `AppState` so camera and model transforms follow state changes
- unknown commands, queries, and missing script files return structured JSON error output in machine mode
- `ctest` currently covers the headless smoke path for direct queries, baseline scripts, and mutation scripts
- `data.scene.objects` now reflects authored scene data loaded from `scenes/bootstrap_scene.json5`, including multiple shape kinds and positions
