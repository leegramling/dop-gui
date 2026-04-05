# dop-gui

`dop-gui` is a VulkanSceneGraph application project aimed at:

- data oriented programming
- functional-style C++20
- spec-driven development
- small, reviewable implementation steps

This repository is currently being bootstrapped from documentation first so we can make deliberate architectural choices before code lands.

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

- [`agents.md`](/home/lgramling/dev/dop-gui/agents.md): working agreements for Codex agents and human collaborators
- [`plan.md`](/home/lgramling/dev/dop-gui/plan.md): current phased roadmap
- [`task.md`](/home/lgramling/dev/dop-gui/task.md): the next small implementation steps
- [`notes.md`](/home/lgramling/dev/dop-gui/notes.md): architecture notes, dependency findings, and decisions

Recommended loop:

1. Start or switch to a dedicated feature branch for the slice.
2. Capture intent and constraints in [`notes.md`](/home/lgramling/dev/dop-gui/notes.md).
3. Convert the next milestone into concrete steps in [`plan.md`](/home/lgramling/dev/dop-gui/plan.md).
4. Pull only the smallest executable slice into [`task.md`](/home/lgramling/dev/dop-gui/task.md).
5. Use [`agents.md`](/home/lgramling/dev/dop-gui/agents.md) to keep prompts and implementation behavior aligned.
6. Implement one slice, verify it, then update the docs before moving on.

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
```

Binary:

- `build/dop-gui/dop-gui`

Quick probe:

```bash
./build/dop-gui/dop-gui -f 1
```

Current result in this session:

- build succeeded
- shaders are compiled during the CMake build with `glslangValidator`
- runtime probe failed in the sandboxed environment because XCB window creation is unavailable
- reported error: `Failed to create Window, unable to establish xcb connection`

Why shaders are built ahead of time:

- the local VulkanSceneGraph install was not compiled with GLSLang support
- runtime-generated VSG shader paths, such as `vsg::Builder`, fail in this environment
- `dop-gui` therefore compiles its GLSL sources to SPIR-V during the build and loads `.spv` files at runtime
