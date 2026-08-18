# Development Rules

**Status:** ACTIVE

These are the canonical engineering rules for the current repository stack. Operational command details live in [`AGENT_RUNBOOK.md`](AGENT_RUNBOOK.md).

## 1. Current stage gate

The repository currently contains **foundation/toolchain/graphical-platform code only**.

Until the user explicitly starts gameplay in a later bounded task:

- do not create a Simulation Core;
- do not create gameplay protocol/types;
- do not create a game executable;
- do not create NPC/economy/social/magic/persistence systems;
- do not create `GAME.md`;
- do not create a gameplay `tools/play.py` runner;
- do not treat `platform_graphics_smoke` as a game.

Foundation work may improve build, vendoring, platform diagnostics, tests, documentation and developer tooling only.

## 2. Runnable-stack invariant

The required development loop must work inside the agent environment without network access.

Required foundation stack:

- C++23;
- GCC with working C++23 support on the verified Linux agent host;
- CMake 3.31+;
- Ninja;
- CTest;
- Python 3 standard library;
- vendored Fenster source for native window/framebuffer/input;
- Linux agent graphics verification: X11 development/runtime, Xvfb, `xwininfo`, ImageMagick CLI.

The ordinary loop must not download SDKs, package-manager dependencies, browser binaries, test frameworks or build tools.

A new required dependency, package manager, browser runtime, WASM toolchain, GPU API requirement or network bootstrap is a separate bounded stack task and must prove local build/run viability before becoming mandatory.

## 3. C++ policy

Language standard: C++23.

CMake must keep:

```text
CMAKE_CXX_STANDARD = 23
CMAKE_CXX_STANDARD_REQUIRED = ON
CMAKE_CXX_EXTENSIONS = OFF
```

Project code defaults:

- value semantics and RAII;
- explicit ownership and lifetime;
- no mutable global application/game state;
- concrete types before interfaces;
- no speculative generic frameworks;
- no exceptions as ordinary domain-control flow;
- no unchecked integer narrowing in load-bearing code;
- no hidden thread/process ownership;
- no mass-formatting unrelated files in a bounded task.

GCC/Clang project targets use target-scoped warnings:

```text
-Wall
-Wextra
-Wpedantic
-Wconversion
-Wsign-conversion
-Wshadow
-Wformat=2
-Wundef
-Wnon-virtual-dtor
-Wold-style-cast
```

MSVC project targets use `/W4 /permissive-`.

Third-party headers must be included as `SYSTEM` where appropriate so project warning policy does not turn vendored upstream implementation warnings into local project noise.

## 4. CMake/build policy

Use:

- CMake;
- Ninja;
- CMake Presets;
- CTest.

Rules:

- canonical presets live in `CMakePresets.json`;
- machine-local `CMakeUserPresets.json` is ignored and never required;
- build directories stay under `build/`;
- use target-based CMake;
- no project-wide `include_directories()` or `add_definitions()` for project code;
- compile/link options are target-scoped;
- platform dependencies are selected explicitly with `WIN32` / `APPLE` / Linux branches;
- no `FetchContent`, CPM, Conan, vcpkg or network dependency acquisition in the required path;
- debug and release builds must both configure and build from clean-enough generated directories using canonical presets.

## 5. Third-party dependency policy

Current required third-party source: vendored Fenster under `third_party/fenster/`.

The repository copy, not an external checkout, is the build input.

Required metadata:

- upstream repository;
- pinned upstream commit;
- license copy;
- local SHA-256 hashes in `third_party/manifest.json`;
- human-readable provenance in `third_party/fenster/UPSTREAM.md`.

`python tools/dev.py vendor-check` must pass before a foundation change is accepted.

Do not edit vendored source during ordinary development. A dependency upgrade is a separate bounded task that must:

1. inspect the upstream diff and license;
2. replace the vendored snapshot intentionally;
3. update provenance and hashes;
4. rerun full foundation verification;
5. report which platforms were actually verified.

Do not silently replace a vendored dependency with a system package because the required loop must remain reproducible from repository contents plus documented host prerequisites.

## 6. Native graphical platform policy

The graphical foundation is a software RGB framebuffer plus a thin native window/input layer.

Fenster is a platform boundary only. It must not grow into gameplay architecture.

Current backend selection:

- Linux -> X11;
- Windows -> Win32;
- macOS -> Cocoa.

The current agent environment fully verifies only Linux/X11.

Do not claim Windows/macOS verification merely because the vendored dependency contains those backends. Platform claims must distinguish:

- implemented in repository/upstream;
- configured by CMake;
- actually built/run in the current environment.

The default renderer foundation remains software framebuffer. Hardware acceleration may be introduced only in a separate bounded stack task with a concrete demonstrated need and full agent-run viability.

## 7. Future gameplay authority

Gameplay is not implemented yet, but the durable boundary is already fixed:

```text
presentation/input -> typed protocol -> authoritative C++23 simulation
simulation !-> presentation/platform tooling
```

When gameplay starts:

- platform/input/rendering code must not own authoritative world state;
- clients send intent, not desired state;
- the simulation computes outcomes;
- player-visible rendering consumes projections/results;
- no second gameplay truth may exist in renderer/UI/debug code.

Do not pre-implement this architecture before the first gameplay task requires it.

## 8. Python tooling policy

Python is repository/developer orchestration only.

The required tooling uses the standard library only.

Rules:

- use `pathlib` for filesystem paths;
- subprocesses use explicit argv and cwd;
- no `shell=True` without a specific reviewed reason;
- every wait/poll/process has a bounded timeout;
- process ownership is explicit;
- cleanup uses `try/finally` or equivalent structured lifecycle;
- kill/terminate only process groups created by the current tool run;
- never use broad `pkill`, `killall` or global browser/X-server cleanup;
- no infinite polling;
- no unexpected network access;
- local verification artifacts live under `.cache/` and are not committed.

## 9. Graphical verification contract

Canonical Linux graphical verification command:

```bash
python tools/dev.py graphics-check
```

Do not replace it with ad-hoc Xvfb/screenshot scripts for normal stack verification.

Contract:

- non-blocking lock: `.cache/graphics-check/graphics.lock`;
- one selected display in reserved range `:90..:119`;
- exactly one Xvfb process created by the run;
- exactly one `platform_graphics_smoke` process created by the run;
- fixed hard wall: 15 seconds inside `tools/graphics_check.py`;
- real X11 key press/release for `D` via the repository helper;
- application-side evidence that `D` was received;
- real window capture via X11 `XGetImage`, not terminal output or synthetic image generation;
- framebuffer size and pixel probe validation before PNG conversion;
- final PNG retained for visual inspection when relevant;
- harness-owned process-group termination after evidence is captured;
- Xvfb terminated only after the application fixture;
- no teardown key/mouse event is required.

If the lock is busy, stop rather than deleting the lock or killing unknown processes.

Artifacts:

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

## 10. Test architecture

Foundation tests are small dependency-free C++ executables registered with CTest.

They must be:

- deterministic;
- independent;
- order-independent;
- behavior/contract focused;
- fast;
- free of external services/network.

Current labels:

```text
foundation
toolchain
graphics
```

Future gameplay labels (`sim`, `protocol`, `determinism`, `scenario`, `slow`) should be introduced only when those layers actually exist.

A framebuffer self-test does not replace the real graphical check; it only proves deterministic drawing logic without OS/window integration.

## 11. Canonical commands

Use these exact front-door commands:

```bash
python tools/dev.py doctor
python tools/dev.py configure --preset native-debug
python tools/dev.py build --preset native-debug
python tools/dev.py test
python tools/dev.py test --target foundation
python tools/dev.py test --target toolchain
python tools/dev.py test --target graphics
python tools/dev.py vendor-check
python tools/dev.py docs-check
python tools/dev.py graphics-check
python tools/dev.py check
python tools/dev.py verify
```

`tools/dev.py` is thin orchestration over actual tools. It must not become a custom build system.

Do not invent alternate canonical commands when the front door already supports the operation.

## 12. Verification levels

### `doctor`

Checks required host executables for the current OS and vendored integrity. On Linux it also checks the headless graphics prerequisites used by the agent verification path.

### `check`

Required for ordinary foundation edits:

```text
doctor
-> native-debug configure
-> native-debug build
-> all CTest tests
-> Python compile check
-> documentation/index/link hygiene
```

### `graphics-check`

Required when platform/window/input/capture code or graphical foundation behavior changes. It opens and drives a real native window under Xvfb and captures the actual window.

### `verify`

Full foundation acceptance on the verified Linux agent host:

```text
check
-> graphics-check
-> native-release configure/build
```

Run `verify` before publishing a foundation-stack change unless the task is explicitly documentation-only and cannot affect executable/tooling contracts. Documentation-only changes still run `docs-check` at minimum.

## 13. Platform support claims

Current evidence:

- Linux x86_64 agent environment: debug/release build and X11/Xvfb graphical verification are required and verified by `tools/dev.py verify`.
- Windows: Win32 backend/link branch exists, but not verified in the current environment.
- macOS: Cocoa backend/link branch exists, but not verified in the current environment.

Do not downgrade Linux verification to a terminal fallback if graphics break. Fix the graphical path or stop with a blocker.

## 14. No CI

This repository intentionally has no CI.

Do not add:

- GitHub Actions;
- CI configuration;
- CI status checks/gates;
- CI polling/reporting code;
- CI documentation.

Only add CI after an explicit later user request in its own bounded task.

## 15. Bounded agent workflow

Every pass:

```text
audit previous task if user said continue
-> inspect relevant code/tests/docs
-> define IN SCOPE / OUT OF SCOPE internally
-> minimal coherent change
-> self-review diff
-> smallest sufficient canonical verification
-> full verify when stack contract changed
-> commit/push only when permitted
-> report VERIFIED / NOT VERIFIED / ASSUMPTIONS / BLOCKERS
-> STOP
```

If an audit discovers a blocker in the previous task, fix only that previous task in the pass.

If the same issue survives two meaningful attempts, stop with observed diagnostics instead of random retries.

## 16. No speculative architecture

Before adding an abstraction/dependency/framework ask:

1. what current problem does it solve;
2. is the problem demonstrated now;
3. can existing simple code solve it;
4. will the addition create another source of truth or lifecycle;
5. can the agent still build and verify the full required path without network;
6. does the user-visible project need it now.

If the only payoff is hypothetical future flexibility, do not add it.
