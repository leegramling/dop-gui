# Flexible Scripting And Test System Alternatives

## Purpose

`dop-gui` needs a test system that can:

- run commands and queries in a deterministic order
- assert query and command results without relying on CTest regular expressions
- report individual pass/fail results and useful failure details
- support simple headless tests
- support branching when a test genuinely needs it
- drive the live GUI through recorded mouse, keyboard, and window events
- verify GUI effects through state queries rather than screenshot comparison
- preserve the current command/query surface as the application automation API

The desired tools are:

- GoogleTest for native C++ unit and component tests
- Python and Robot Framework as acceptable external dependencies
- JSON5 for simple declarative test cases
- optional Python or Lua for general branching logic
- real GUI input record/replay when semantic UI command names are inconvenient

## Current System

The current `ScriptRunner` supports:

```json5
{
  commands: ["..."],
  queries: ["..."],
}
```

and ordered actions:

```json5
{
  actions: [
    { command: "..." },
    { query: "..." },
    { sleepMs: 100 },
  ],
}
```

Current strengths:

- commands and queries already return structured result types
- ordered actions can interleave commands and queries
- `--ui-test-mode` evaluates the real panel code without opening a window
- panel-scoped UI actions map stable panel and widget IDs to pending input
- CTest can launch the executable and inspect output

Current limitations:

- scripts cannot assert values themselves
- an unsuccessful expectation does not have a first-class test result
- no variables, result capture, conditions, loops, setup, or teardown
- the JSON5 reader is a narrow regex/balanced-block parser, not a general parser
- CTest regular expressions are difficult to maintain and provide weak diagnostics
- live UI commands and queries can cross frame boundaries differently from headless UI evaluation
- there is no persistent external automation protocol
- no mouse or keyboard recording/playback exists in this repository

Before adding nested assertions or conditions, replace the ad hoc JSON5 parsing with a real JSON5-capable parser or use strict JSON for the new test schema. Extending the current regex parser would create ambiguous behavior and poor error messages.

## Required Execution Semantics

The new system should define these rules explicitly:

1. Steps execute in file order.
2. A headless command completes and refreshes UI state before the next step.
3. A query returns one immutable result value for that step.
4. An assertion evaluates against a captured result, not against formatted console text.
5. A live input step completes only after its events have been delivered and at least one UI frame has finished.
6. A live query after input must use an explicit frame barrier or `waitUntil` operation.
7. Any failed command, query, assertion, timeout, or replay operation makes the test process return nonzero.
8. Every step receives an index and optional name so failures identify the exact source step.

These rules prevent panel draw order from becoming script order. A panel may draw X before Y, but a test step targeting Y remains a Y operation. Draw order only controls where a pending UI event is consumed inside a completed frame.

## Recommended Layered Architecture

No single tool is the best fit for all test levels. Use four layers over one shared automation API.

```text
GoogleTest ----------> typed C++ command/query APIs
JSON5 test runner ---> AutomationEngine
Python/Robot --------> JSON-lines process protocol ---> AutomationEngine
GUI replay ----------> VSG input event injection ------> live UI
                                                   |
All assertion paths <--------- typed query results <+
```

### Shared AutomationEngine

Extract the execution logic from `ScriptRunner` into an `AutomationEngine` with operations such as:

- `executeCommand(path)`
- `executeQuery(path)`
- `evaluateUi()`
- `advanceFrame(count)`
- `waitUntil(query, matcher, timeout)`
- `injectInput(event)`
- `startRecording(path)`
- `stopRecording()`
- `replay(path)`

It should return typed results, not serialized JSON strings. Serialization belongs at the CLI, Robot, and report boundaries.

## Alternative 1: Native JSON5 Assertions

Add a deliberately small assertion vocabulary to ordered scripts.

```json5
{
  name: "properties preserve Y before X",
  actions: [
    { command: "ui.test.panel.panel-properties.set_text.position-y=2.50 m" },
    {
      query: "data.scene.object.bootstrap_triangle.transform",
      saveAs: "afterY",
    },
    {
      assert: {
        actual: "$afterY.position[1]",
        equals: 2.5,
        tolerance: 0.0001,
      },
    },
    { command: "ui.test.panel.panel-properties.set_text.position-x=1.25 m" },
    {
      query: "data.scene.object.bootstrap_triangle.transform",
      expect: {
        position: [1.25, 2.5, 0.0],
      },
    },
  ],
}
```

Recommended matchers:

- `equals`
- `notEquals`
- `near` or numeric `tolerance`
- `contains`
- `exists`
- `isTrue` / `isFalse`
- array and object partial matching

Do not initially add arbitrary expressions. Use a small result path format such as JSON Pointer or a documented `$name.field[index]` path.

### Simple Conditions

If limited branching is useful in declarative scripts, support only `when` with `then` and `else` step arrays:

```json5
{
  actions: [
    { query: "runtime.capabilities", saveAs: "caps" },
    {
      when: { actual: "$caps.guiAvailable", equals: true },
      then: [
        { replay: "tests/recordings/create_shape.json5" },
        { query: "data.scene.objects", expect: { containsId: "sphere_1" } },
      ],
      else: [
        { skip: "GUI is unavailable on this worker" },
      ],
    },
  ],
}
```

This is sufficient for capability selection. General computation, loops, and complex branching should remain in Python rather than turning JSON5 into a programming language.

### Advantages

- smallest change for existing tests
- readable beside current scripts
- fast headless execution
- no Python required for basic cases
- easy CTest integration

### Disadvantages

- requires a real structured parser and matcher implementation
- reporting must be built
- branching must remain intentionally limited

## Alternative 2: GoogleTest

GoogleTest should test native C++ behavior directly.

Good targets:

- command parsing and canonicalization
- query path resolution
- typed query values
- widget-name compatibility mapping
- pending UI action consumption
- script parsing and action ordering
- assertion matcher behavior
- record/replay serialization
- `AutomationEngine` state transitions

Example shape:

```cpp
TEST(PropertyActions, YThenXRemainsOrdered)
{
    TestApplication app;
    AutomationEngine automation(app);

    ASSERT_TRUE(automation.command(
        "ui.test.panel.panel-properties.set_text.position-y=2.50 m"));
    EXPECT_DOUBLE_EQ(automation.queryDouble(
        "data.scene.object.bootstrap_triangle.transform", "/position/1"), 2.5);

    ASSERT_TRUE(automation.command(
        "ui.test.panel.panel-properties.set_text.position-x=1.25 m"));
    EXPECT_DOUBLE_EQ(automation.queryDouble(
        "data.scene.object.bootstrap_triangle.transform", "/position/0"), 1.25);
}
```

GoogleTest is not a replacement for user-authored scripts or live GUI replay. It is the right layer for fast native tests and detailed assertion diagnostics.

The repository does not currently include GoogleTest. Add it through a pinned dependency mechanism, preferably the same cross-platform dependency policy used by the rest of the project. Register discovered tests with CTest.

## Alternative 3: Python Driver

Python is the best general branching language for this project because it is acceptable as a dependency, already available on the current development machine, and integrates naturally with Robot Framework.

Do not embed Python in the C++ process initially. Run the app as a child process and expose a persistent JSON-lines protocol on stdin/stdout:

```json
{"id":1,"op":"command","path":"scene.load.cubes"}
{"id":2,"op":"query","path":"data.scene.objects"}
{"id":3,"op":"advanceFrames","count":1}
{"id":4,"op":"replay","path":"tests/recordings/select_cube.json5"}
```

Each request receives exactly one response:

```json
{"id":2,"ok":true,"value":[...]}
```

Suggested CLI mode:

```text
dop-gui --automation-stdio [--ui-test-mode | --live]
```

A Python client can then provide normal assertions and branching:

```python
with DopGui(headless_ui=True) as app:
    app.command("scene.load.cubes")
    objects = app.query("data.scene.objects")

    if len(objects) == 3:
        app.command("scene.select_object=cube_center")
    else:
        raise AssertionError(f"expected 3 cubes, got {len(objects)}")
```

### Advantages

- full variables, branching, loops, fixtures, retries, and libraries
- good JSON handling and failure messages
- no embedded VM lifecycle inside the renderer
- one client can support both direct Python tests and Robot keywords

### Disadvantages

- requires a persistent process protocol
- process synchronization and shutdown must be designed carefully
- live GUI mode needs explicit frame barriers

## Alternative 4: Robot Framework

Robot Framework is a good acceptance-test and reporting layer on top of the Python driver.

Create a custom Python library with keywords such as:

- `Start Dop Gui Headless`
- `Start Dop Gui Live`
- `Execute Command`
- `Query Should Equal`
- `Query Value Should Be Near`
- `Wait Until Query Matches`
- `Start Input Recording`
- `Stop Input Recording`
- `Replay Input Recording`
- `Advance Frames`
- `Stop Dop Gui`

Example:

```robot
*** Settings ***
Library    DopGuiLibrary.py
Suite Setup       Start Dop Gui Headless
Suite Teardown    Stop Dop Gui

*** Test Cases ***
Set Property Y Then X
    Execute Command    ui.test.panel.panel-properties.set_text.position-y=2.50 m
    Query Value Should Be Near
    ...    data.scene.object.bootstrap_triangle.transform
    ...    /position/1
    ...    2.5
    ...    0.0001
    Execute Command    ui.test.panel.panel-properties.set_text.position-x=1.25 m
    Query Value Should Be Near
    ...    data.scene.object.bootstrap_triangle.transform
    ...    /position/0
    ...    1.25
    ...    0.0001
```

Robot supplies suite/test organization and standard result/report artifacts. It should orchestrate the app; it should not know internal VSG or ImGui details.

Robot Framework is not currently installed on the development machine, so it must be added to a pinned Python requirements file and CI environment.

## Alternative 5: Embedded Lua

Lua can expose `command`, `query`, `assert`, and `replay` directly inside the app:

```lua
command("scene.load.cubes")
local objects = query("data.scene.objects")

if #objects ~= 3 then
    fail("expected three cubes")
end
```

### Advantages

- compact embedded runtime
- full branching and functions
- no child-process protocol
- scripts can run tightly with frame updates

### Disadvantages

- adds a new native dependency and binding layer
- C++/Lua value conversion must be implemented
- test discovery, fixtures, reporting, and CI output still need custom work
- script errors cross the rendering/application boundary
- duplicates capabilities already available through accepted Python dependencies

Lua is reasonable for future in-application behavior scripting. It is not the recommended first test orchestration layer because Python plus Robot provides stronger test tooling and reporting with less custom infrastructure.

## GUI Mouse And Keyboard Record/Replay

### Goal

Allow a developer to start the application, record real interaction, move the mouse, click controls, type values, stop recording, and replay the result as an automated GUI test.

The replay should use assertions on command/query state. It should not compare screenshots.

### Existing Foundation

The application already routes VSG input through `vsgImGui::SendEventsToImGui`, and the installed VSG headers include `vsg::RecordEvents` and `vsg::PlayEvents`. These should be evaluated before implementing a separate event transport.

### Proposed CLI

```text
dop-gui --record-input tests/recordings/create_shape.json5
dop-gui --replay-input tests/recordings/create_shape.json5 --script tests/create_shape_assertions.json5
```

Optional interactive controls:

- `F9`: start/stop recording
- `F10`: insert a named checkpoint
- `F11`: stop and save

### Recorded Event Schema

```json5
{
  formatVersion: 1,
  environment: {
    windowWidth: 1600,
    windowHeight: 900,
    uiScale: 1.5,
    dockLayoutVersion: 1,
  },
  events: [
    { frame: 4, type: "pointerMove", x: 1390, y: 210 },
    { frame: 5, type: "buttonPress", button: 1, x: 1390, y: 210 },
    { frame: 6, type: "buttonRelease", button: 1, x: 1390, y: 210 },
    { frame: 8, type: "text", value: "1.25" },
    { frame: 10, type: "keyPress", key: "Enter" },
    { frame: 11, type: "keyRelease", key: "Enter" },
  ],
}
```

Record at least:

- pointer movement
- mouse button press/release
- scroll wheel
- key press/release
- text input
- window configure/resize
- UI scale and window size metadata
- frame number or deterministic tick

### Determinism Requirements

Coordinate replay is fragile unless these values are controlled:

- window client/render extent
- DPI and application UI scale
- dock layout
- font files and font size
- initial scene and panel open state
- frame advancement

Playback should reject incompatible environment metadata by default instead of silently clicking the wrong location. An explicit override may allow normalized coordinates, but raw recorded client coordinates with a fixed test environment are more predictable.

### Replay And Assertions

A recording drives input; a companion test defines correctness:

```json5
{
  actions: [
    { replay: "tests/recordings/edit_cube_position.json5" },
    { barrier: "uiFrame" },
    {
      query: "data.scene.object.bootstrap_cube.transform",
      expect: { position: [1.25, 0.0, 0.25] },
    },
  ],
}
```

Robot equivalent:

```robot
Replay Input Recording    tests/recordings/edit_cube_position.json5
Wait For UI Frame
Query Value Should Equal
...    data.scene.object.bootstrap_cube.transform
...    /position/0
...    1.25
```

### Optional Semantic Hints

Record raw events as the source of truth, but attach diagnostic hints when the pointer is over a wrapped widget:

```json5
{
  frame: 5,
  type: "buttonPress",
  x: 1390,
  y: 210,
  hint: { panel: "panel-properties", widget: "position-x" },
}
```

Hints improve failure messages and may later support coordinate repair. Replay must not require a hint because recording exists specifically for controls that are difficult to map manually.

## Reporting

Every runner should converge on one internal result model:

```text
TestRunResult
  name
  status: passed | failed | skipped | error
  duration
  steps[]
    index
    name
    operation
    status
    expected
    actual
    message
```

Outputs:

- concise console output for developers
- structured JSON for tooling
- nonzero exit status on failure/error
- JUnit XML for CI
- Robot's normal report artifacts when Robot is the runner

CTest should decide pass/fail from the process exit code. Regular expressions should be reserved for a small number of compatibility checks.

## Decision Matrix

| Option | Assertions | Branching | Reporting | Headless | Live GUI | Native dependency |
|---|---:|---:|---:|---:|---:|---:|
| JSON5 runner | Good, custom | Limited | Custom | Excellent | Good with barriers | No new VM |
| GoogleTest | Excellent | C++ | Excellent | Excellent | Limited integration | GoogleTest |
| Python | Excellent | Excellent | Good/custom | Excellent | Good via protocol | Python runtime |
| Robot Framework | Excellent | Good | Excellent | Excellent | Good via Python library | Python packages |
| Embedded Lua | Custom | Excellent | Custom | Good | Excellent in-process | Lua runtime |

## Recommendation

Use this combination:

1. Keep command/query paths as the stable automation contract.
2. Add typed assertions and captures to JSON5 for simple deterministic tests.
3. Add GoogleTest for native command/query/parser/engine tests.
4. Add a persistent JSON-lines automation mode.
5. Build one Python client and expose it as a Robot Framework library.
6. Add VSG event recording/replay for real GUI mouse and keyboard tests.
7. Use Python or Robot conditions for general branching; keep JSON5 conditions small.
8. Defer Lua until the product needs embedded user scripting beyond testing.

## Implementation Phases

### Phase 1: Assertions And Results

- replace the current narrow script parser
- add `saveAs`, `expect`, and `assert` steps
- add exact, partial, and numeric-tolerance matchers
- return nonzero on failures
- emit step-level JSON and JUnit XML
- convert several CTest regex cases to script assertions

### Phase 2: GoogleTest

- add a pinned GoogleTest dependency
- extract `AutomationEngine`
- unit test command/query routing and typed values
- unit test widget mapping and action ordering
- unit test assertion matchers and parser diagnostics

### Phase 3: Persistent Python Protocol

- add `--automation-stdio`
- define versioned JSON-lines requests and responses
- implement startup, timeout, and graceful shutdown
- create a Python client with context-manager lifecycle
- add Python branching examples

### Phase 4: Robot Framework

- pin Robot dependencies
- wrap the Python client as keywords
- publish Robot output and reports in CI
- cover headless acceptance flows first

### Phase 5: GUI Recording And Replay

- evaluate VSG `RecordEvents`/`PlayEvents`
- add recorder hooks before ImGui and trackball consumption
- define a versioned event file
- add deterministic frame barriers
- validate window size, UI scale, and dock-layout metadata
- replay one recorded Properties edit
- assert the resulting transform through a query

## First Vertical Slice

The smallest useful implementation should be:

1. Add `expect` to an ordered query action.
2. Make expectation failures return exit code 1 with expected/actual details.
3. Convert `ui_property_order_cli.json5` from a CTest regex to native expectations.
4. Add GoogleTest coverage for the matcher.
5. Define, but do not yet implement, the JSON-lines protocol.

That delivers real assertions immediately without committing the project to Lua or requiring Robot for every test.

## Runnable C++ JSON Result Example

`tests/JsonResultTest.cpp` is a dependency-free C++ integration example registered as `dop_gui_cpp_json_result`. It launches `dop-gui`, reads the JSON result of a transform query, parses the response into typed objects and arrays, and returns nonzero when an expectation fails.

Run it through CTest:

```bash
cmake --build build/dop-gui -j 8
ctest --test-dir build/dop-gui -R dop_gui_cpp_json_result --output-on-failure
```

The example uses a small local JSON parser because no JSON development package or GoogleTest installation is currently available in this workspace. Once the project pins those dependencies, keep the integration-test flow but replace the local parser and `expect(...)` helpers with the selected JSON library and GoogleTest assertions.

## Reference Documentation

- GoogleTest primer: <https://google.github.io/googletest/primer.html>
- Robot Framework user guide: <https://robotframework.org/robotframework/latest/RobotFrameworkUserGuide.html>
- Lua 5.4 reference manual: <https://www.lua.org/manual/5.4/manual.html>
- Python subprocess documentation: <https://docs.python.org/3/library/subprocess.html>
