# Agent Stack Runbook

**Status:** ACTIVE

This is the complete operational runbook for a fresh coding-agent session. Follow it instead of rediscovering how the repository is built or how the graphical verification works.

Engineering policy is canonical in [`DEVELOPMENT_RULES.md`](DEVELOPMENT_RULES.md); implemented structure is canonical in [`../ARCHITECTURE.md`](../ARCHITECTURE.md).

## 1. First five minutes in a fresh session

Read, in order:

1. repository root [`AGENTS.md`](../../AGENTS.md);
2. [`../INDEX.md`](../INDEX.md);
3. this runbook;
4. [`DEVELOPMENT_RULES.md`](DEVELOPMENT_RULES.md);
5. [`../ARCHITECTURE.md`](../ARCHITECTURE.md).

Then inspect repository status and run:

```bash
git status --short
python tools/dev.py doctor
```

If the task concerns stack/build/platform/tooling or you are auditing a previous foundation change, establish the complete baseline with:

```bash
python tools/dev.py verify
```

Do **not** search for `tools/play.py`, `sim_cli`, browser/WASM commands or gameplay tests. They intentionally do not exist at the current foundation stage.

## 2. Current repository stage

The repository is not yet a game implementation.

Implemented:

- C++23 build foundation;
- debug/release CMake/Ninja presets;
- dependency-free CTest foundation tests;
- vendored native framebuffer/window/input layer;
- Linux X11 graphical stack diagnostic;
- fully local headless graphical verification under Xvfb;
- documentation/vendor integrity checks.

Not implemented:

- Simulation Core;
- gameplay protocol;
- real game executable;
- player/NPC/economy/social/magic mechanics;
- save/load;
- `GAME.md`;
- gameplay playtest runner.

`platform_graphics_smoke` is a diagnostic executable only.

## 3. Repository map for stack work

```text
AGENTS.md
  short mandatory agent entry point

CMakeLists.txt
  targets, warnings and OS link selection

CMakePresets.json
  native-debug and native-release presets

src/platform/graphics_smoke.cpp
  native window/framebuffer/input fixture; NOT gameplay

tests/foundation/foundation_test.cpp
  C++23/toolchain proof

tools/dev.py
  canonical front door for all foundation commands

tools/graphics_check.py
  Linux graphical verification supervisor

tools/platform/x11_key_sender.cpp
  sends real X11 key press/release to one known fixture window

tools/platform/x11_capture.cpp
  captures one known X11 window with XGetImage into PPM

tools/docs_check.py
  local Markdown/index/CI-policy hygiene

tools/vendor_check.py
  third-party local SHA-256 integrity

third_party/fenster/
  vendored source + MIT license + provenance
third_party/manifest.json
  dependency metadata and local hashes

docs/engineering/DEVELOPMENT_RULES.md
  engineering policy
docs/ARCHITECTURE.md
  implemented foundation architecture
docs/specs/PROJECT_SPEC.md
  temporary product/roadmap contract
```

## 4. Host prerequisites

### Verified Linux agent host

The full canonical foundation verification requires these to already exist on the host:

- `git`;
- `g++` with C++23 support;
- CMake 3.31+;
- Ninja;
- CTest;
- Python 3;
- X11 headers/libraries discoverable by CMake;
- `Xvfb`;
- `xwininfo`;
- ImageMagick CLI (`magick`, with `convert` accepted only as fallback).

`python tools/dev.py doctor` checks executable prerequisites and vendored integrity. CMake configure is the authoritative check that X11 development files are usable.

Do not add an automatic apt/brew/choco/network bootstrap to compensate for a missing host prerequisite. Report the missing tool as an environment blocker unless the user explicitly requests a stack/bootstrap change.

### Verified environment snapshot

The foundation was established on a Linux x86_64 agent host with:

```text
GCC 14.2.0
Clang 17.0.0 available as optional diagnostic compiler
CMake/CTest 3.31.6
Ninja 1.12.1
Python 3.13.5
X11 1.8.12
xwininfo 1.1.6
ImageMagick 7.1.2-1
```

These are evidence of the verified host, not all universal minimums. The hard version floor currently encoded by the build is CMake 3.31 and C++23 compilation.

## 5. Command reference

### Doctor

```bash
python tools/dev.py doctor
```

Checks:

- core host executables;
- Linux graphical verification executables when on Linux;
- vendored dependency hashes.

Expected final dependency signal:

```text
vendor integrity check passed
```

### Configure debug

```bash
python tools/dev.py configure --preset native-debug
```

Equivalent core operation: `cmake --preset native-debug`.

Generated tree: `build/native-debug/`.

### Build debug

```bash
python tools/dev.py build --preset native-debug
```

Expected relevant targets on Linux:

- `foundation_tests`;
- `platform_graphics_smoke`;
- `platform_x11_key_sender`;
- `platform_x11_capture`.

### Tests

All native tests:

```bash
python tools/dev.py test
```

Focused labels:

```bash
python tools/dev.py test --target foundation
python tools/dev.py test --target toolchain
python tools/dev.py test --target graphics
```

Current CTest tests:

- `foundation_tests`;
- `graphics_framebuffer_self_test`.

The graphics self-test does not open a window. It does not replace `graphics-check`.

### Vendor integrity

```bash
python tools/dev.py vendor-check
```

Expected:

```text
vendor integrity check passed
```

If this fails, do not edit the hash just to make the check green. See third-party triage below.

### Documentation hygiene

```bash
python tools/dev.py docs-check
```

Expected:

```text
documentation link/index hygiene check passed
```

### Fast foundation check

```bash
python tools/dev.py check
```

Runs:

```text
doctor
-> debug configure
-> debug build
-> all CTest tests
-> Python compileall for tools
-> docs-check
```

Use this for normal edit iterations that do not require an OS graphical run every time.

### Real graphical stack check

```bash
python tools/dev.py graphics-check
```

Expected success line:

```text
GRAPHICS CHECK SUCCESS <repo>/.cache/graphics-check/<run-id>
```

This is not a screenshot generator detached from the application. It drives and captures a real native X11 window.

### Full foundation acceptance

```bash
python tools/dev.py verify
```

Runs:

```text
check
-> graphics-check
-> release configure
-> release build
```

On the verified Linux agent host, this is the command to run before publishing a stack/foundation change.

## 6. What `graphics-check` actually does

Do not rewrite this lifecycle ad hoc in another agent session.

`tools/graphics_check.py`:

1. requires Linux;
2. verifies that the graphical fixture and X11 helper binaries were already built;
3. obtains a non-blocking `flock` on `.cache/graphics-check/graphics.lock`;
4. selects an unused X display socket in `:90..:119`;
5. creates one run directory;
6. starts one Xvfb process with TCP listening disabled;
7. waits for the selected X11 Unix socket with a bounded poll;
8. starts one `platform_graphics_smoke` process in its own session/process group;
9. locates the exact window title `Simulation Graphics Stack Smoke` with `xwininfo`;
10. invokes `platform_x11_key_sender <window-id> D`;
11. requires application evidence file `state.txt` containing `d_received=1`;
12. invokes `platform_x11_capture <window-id> final.ppm`;
13. parses the captured PPM itself and checks:
    - size `320x200`;
    - pixel `(228,100)` equals RGB `(53,168,83)` / `#35A853`;
14. converts `final.ppm` to `final.png` with the local ImageMagick executable;
15. verifies the graphical process is still alive after input and capture;
16. terminates the owned application process group;
17. terminates the owned Xvfb process group;
18. writes `run.json` and retains logs/evidence.

Hard inner wall-clock timeout: 15 seconds.

The outer `tools/dev.py graphics-check` command also has a bounded subprocess timeout.

## 7. Why teardown does not press Q

A previous harness iteration attempted to send another X11 key during teardown. That created an unnecessary lifecycle race: after evidence was already captured, the window id could become invalid before the teardown key operation.

Current rule:

- `D` proves the real input path;
- screenshot/captured pixels prove the real rendering path;
- **the supervisor owns fixture lifetime**;
- teardown uses only the process groups created by that supervisor.

Do not reintroduce a UI-event-based teardown unless a future gameplay requirement specifically needs graceful in-game quit behavior to be tested.

## 8. Graphical artifacts and how to inspect them

A successful run creates:

```text
.cache/graphics-check/<run-id>/
  run.json
  state.txt
  stdout.log
  stderr.log
  xvfb.stderr.log
  final.ppm
  final.png
```

Expected evidence:

`state.txt`:

```text
d_received=1
```

`stdout.log` includes:

```text
GRAPHICS_SMOKE D_RECEIVED
```

`run.json` must say `status: success` and record the expected render probe and harness-owned teardown.

`final.png` is the actual captured window. Current fixture appearance after `D`:

- dark background;
- light rectangular border;
- blue block on the left;
- green block on the right.

When a task changes rendering/platform behavior, inspect `final.png` visually in addition to trusting the pixel probe.

## 9. Process ownership and anti-hang rules

Never globally kill display servers or unrelated processes.

Allowed cleanup scope: only PIDs/process groups created by the current tool invocation.

Never use:

```text
pkill Xvfb
killall Xvfb
pkill <game>
killall <game>
```

The graphics supervisor uses `start_new_session=True` so it can terminate the exact owned group.

If `GRAPHICS CHECK BUSY` occurs:

- another process holds the `flock`;
- do not delete the lock file as a workaround;
- do not kill unknown processes;
- stop/report or wait only if the current bounded task explicitly permits it.

The lock file itself may remain on disk after success; ownership is represented by the OS lock, not file existence.

## 10. Failure triage

### `doctor`: required tool missing

Example:

```text
required local tool is missing: Xvfb
```

Meaning: host prerequisite missing.

Action: report environment blocker. Do not introduce a network bootstrap or downgrade to terminal/browser fallback without an explicit stack task.

### CMake cannot find X11 on Linux

Meaning: X11 development files are missing/unusable.

Action: environment blocker. Do not silently remove the graphical target or make it optional on the agent host.

### Vendor integrity mismatch

Action sequence:

1. inspect `git status` and the changed vendored file;
2. if accidental, restore it from the repository;
3. if an intentional dependency upgrade, stop ordinary work and perform the upgrade as its own bounded stack task;
4. never update the expected hash without reviewing provenance/source/license.

### `GRAPHICS CHECK BUSY`

Meaning: another live run owns the lock.

Action: do not remove the lock or kill unknown processes.

### Xvfb socket timeout/early exit

Inspect:

```text
.cache/graphics-check/<run-id>/xvfb.stderr.log
.cache/graphics-check/<run-id>/run.json
```

Do not scan/kill all X servers.

### Window not found

Inspect:

```text
stdout.log
stderr.log
xvfb.stderr.log
run.json
```

Likely categories: application failed to open display, title changed without updating the harness contract, application exited early.

Do not increase sleeps blindly; polling is already bounded.

### `state.txt` absent/invalid

Meaning: real X11 input did not reach/trigger the fixture contract.

Inspect `stdout.log`/`stderr.log` and `platform_x11_key_sender` behavior. Do not fake state evidence from Python.

### Capture fails

The repository deliberately uses its own `XGetImage` helper rather than ImageMagick X11 capture because the verified ImageMagick build does not provide a usable X11 capture delegate.

Do not replace `platform_x11_capture` with `import`/screen scraping just for convenience.

### Pixel probe mismatch

Meaning: captured framebuffer differs from the expected post-input diagnostic state.

Inspect `final.ppm`/`final.png` and the fixture rendering code. Do not simply change the probe value unless the expected diagnostic contract intentionally changed.

### PNG conversion fails

`final.ppm` is the load-bearing X11 capture. PNG is the convenient visual artifact.

Check local ImageMagick availability and command behavior. The canonical host prefers `magick`; `convert` is fallback.

### Teardown problems

Cleanup is supervisor-owned. Diagnose process-group lifecycle. Do not add a second key event as teardown and do not use global process kills.

## 11. Third-party upgrade procedure

Fenster is vendored so normal work stays network-free.

Current provenance lives in [`../../third_party/fenster/UPSTREAM.md`](../../third_party/fenster/UPSTREAM.md) and [`../../third_party/manifest.json`](../../third_party/manifest.json).

For an intentional upgrade:

1. make the dependency upgrade the only bounded task;
2. inspect the exact upstream commit and license using an approved external/GitHub source;
3. review upstream changes relevant to Linux/Win32/Cocoa behavior;
4. replace `third_party/fenster/fenster.h` and license intentionally;
5. update provenance metadata;
6. recompute/update local SHA-256 hashes;
7. run `python tools/dev.py vendor-check`;
8. run `python tools/dev.py verify` on the Linux agent host;
9. report Windows/macOS as unverified unless actually tested on those platforms;
10. commit the dependency update as one coherent stack change.

Normal build/test commands must still work without network afterward.

## 12. Documentation change procedure

For any command, path, stack or lifecycle change:

1. update implementation first;
2. update the canonical owner document in the same task;
3. update this runbook if another agent's operation changes;
4. update `docs/INDEX.md` for added/removed canonical docs;
5. update README/AGENTS pointers only when entry points change;
6. run:

```bash
python tools/dev.py docs-check
```

If the stack behavior changed, finish with:

```bash
python tools/dev.py verify
```

Do not keep contradictory old instructions “for history”; delete them. Git history is the archive.

## 13. Task-to-verification matrix

### Docs-only, no command/contract change

```bash
python tools/dev.py docs-check
```

Also review the diff manually.

### CMake/foundation C++ change without graphical OS behavior

```bash
python tools/dev.py check
```

Run `verify` before publishing if the build/toolchain contract changed.

### Fenster/platform/window/input/capture/Xvfb helper change

```bash
python tools/dev.py verify
```

Inspect the newest `final.png` visually.

### Vendored dependency metadata/source change

```bash
python tools/dev.py vendor-check
python tools/dev.py verify
```

### Future gameplay change

Not applicable yet. When gameplay begins, the canonical gameplay playtest contract must be created and documented in a separate bounded task; do not reuse the diagnostic graphics smoke as gameplay evidence.

## 14. Git workflow for agents

Before edits:

```bash
git status --short
```

Never discard unknown user changes.

Default workflow is trunk-first, one coherent bounded task -> one meaningful commit where practical. Use a feature branch only for genuinely risky/long work or explicit user request.

Before publication:

- self-review all changed files;
- verify no generated `build/` or `.cache/` artifacts are staged;
- run the canonical verification for the task;
- confirm remote `main` has not unexpectedly moved before a fast-forward update;
- never force-update `main` over unrelated work.

This repository intentionally has no CI; do not search for or report CI status.

## 15. Handoff checklist for a new chat/agent

A new agent should **not** start by experimenting with compilers, browsers or window systems.

Use this exact sequence:

```text
1. Read AGENTS.md.
2. Read docs/INDEX.md.
3. Read this runbook + DEVELOPMENT_RULES + ARCHITECTURE.
4. Run git status --short.
5. Run python tools/dev.py doctor.
6. Audit the previous reported task if the user said “continue”.
7. For foundation baseline, run python tools/dev.py verify.
8. Use only documented front-door commands.
9. Inspect .cache/graphics-check/<run-id>/final.png when graphics evidence matters.
10. Do one bounded task, report, STOP.
```

If these commands fail, triage the documented failure mode. Do not begin a new parallel stack design before establishing why the canonical path failed.

## 16. Things a fresh agent must not resurrect

Unless the user explicitly changes the stack in a later bounded task, do not reintroduce as required paths:

- Emscripten/WASM;
- browser/Playwright runtime;
- TypeScript/npm frontend;
- terminal reference client;
- GoogleTest or package-fetched test frameworks;
- `FetchContent` network dependencies;
- CI/GitHub Actions;
- gameplay code before explicit start.

The native graphical stack is the required foundation because it is the path actually proven runnable inside the agent environment.
