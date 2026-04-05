# How To Add Test Commands

This guide explains how to extend the command/query/UI-test surface without breaking the existing automation model.

## Goal

Test commands should be:

- stable across CLI, headless, and live GUI runs
- serializable in JSON5 where practical
- explicit about the state they read or mutate
- queryable through JSON output so tests can assert on results

## Choose The Right Surface

Add a new entry to one of these surfaces:

- `Command` for state changes, UI simulation, and scripted actions
- `Query` for read-only inspection
- `UiLayer` and wrapped widgets for labeled UI elements and simulated UI interaction
- `ScriptRunner` for batch or timed command/query playback

Prefer the smallest surface that matches the behavior.

## Adding A New Command

1. Add a canonical command path in [src/Command.cpp](/home/lgramling/dev/dop-gui/src/Command.cpp).
2. Update `canonicalizeCommandPath(...)` if you need aliases.
3. Add a handler in the command route table.
4. Return a JSON-serializable result describing the state change.
5. Add the new command name to `commandNames()`.
6. Add a regression script under [tests/](/home/lgramling/dev/dop-gui/tests) if the command should be exercised in batch or live runs.
7. Register a `ctest` case in [CMakeLists.txt](/home/lgramling/dev/dop-gui/CMakeLists.txt).

Example command types:

- `scene.load.cubes`
- `app.exit`
- `ui.test.click.menuitem-scene-cubes`
- `ui.test.set_text.panel-bgcolor=#0000FF`

## Adding A New Query

1. Add a canonical query path in [src/Query.cpp](/home/lgramling/dev/dop-gui/src/Query.cpp).
2. Update `canonicalizeQueryPath(...)` if the new query needs aliases.
3. Add a route handler for the namespace or prefix.
4. Return a JSON-friendly value tree.
5. Add the new query name to `queryNames()`.
6. Add a regression script or `ctest` case that asserts on the returned output.

Example query types:

- `view.background.color`
- `data.scene.objects`
- `ui.widget.panel-bgcolor`
- `runtime.capabilities`

## Adding A New UI Test Command

UI test commands should target the widget label, not the visual caption.

1. Give the widget a stable label such as `panel-bgcolor` or `menuitem-scene-cubes`.
2. Register the widget label in the wrapper layer.
3. Add a `ui.test.*` command path that stores an explicit pending action by label.
4. Make the wrapped widget consume the pending action in both headless and live GUI mode.
5. Add a query such as `ui.widget.<label>` so tests can verify the resulting widget state.
6. Add headless coverage with `--ui-test-mode --script ...`.
7. Add live coverage with `test_run.sh` helper modes when the interaction needs a desktop session.

Use these command shapes as the default pattern:

- `ui.test.click.<label>`
- `ui.test.set_bool.<label>=true|false`
- `ui.test.set_text.<label>=<value>`

## Adding A New Script

Use JSON5 scripts in [tests/](/home/lgramling/dev/dop-gui/tests).

There are two current shapes:

- legacy `commands` and `queries` arrays
- ordered `actions: [...]` objects with `command`, `query`, and `sleepMs`

Prefer ordered `actions` when a test needs time sequencing or wants command/query output in a single result stream.

Example:

```json5
{
  actions: [
    { command: "scene.load.cubes" },
    { command: "ui.test.set_bool.panel-display-grid=false" },
    { query: "view.background.color" },
    { sleepMs: 500 },
    { command: "app.exit" },
  ],
}
```

## Add Verification

For each new command/query/test command:

- add at least one direct CLI probe
- add at least one `ctest` case when the behavior is stable
- add a JSON5 script under [tests/](/home/lgramling/dev/dop-gui/tests) if the flow is reusable
- update [README.md](/home/lgramling/dev/dop-gui/README.md) with the new user-facing workflow if it matters for day-to-day use

## Avoid These Mistakes

- do not hide state changes behind opaque callbacks
- do not add one-off test hooks that cannot be queried later
- do not bake visual captions into test identifiers
- do not add a new command path without updating the result format and the workflow docs
