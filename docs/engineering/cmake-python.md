# CMake, native tests, and Python developer tools

Python is developer tooling/orchestration only; it is not a second Simulation Core.

Canonical contracts:

- dependency direction: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- active Godot/native pins: [`VERSIONS.md`](VERSIONS.md)
- verification/playtest: [`../VERIFICATION.md`](../VERIFICATION.md)
- C++ policy: [`cpp.md`](cpp.md)

Primary upstream references are tracked in [`SOURCES.md`](SOURCES.md).

## Actual build graph

The root `CMakeLists.txt` owns the executable native graph:

```text
sim_core
  ^
  |-- sim_tests (+ GTest::gtest_main)
  `-- world_sim_gdextension (+ godot::cpp)
```

`sim_core` is C++23 and Godot-free. `world_sim_gdextension` is the only project target that links godot-cpp. The architecture check is registered with CTest when tests are enabled.

## CMake principles

### Targets, not directory globals

Use `target_sources`, `target_include_directories`, `target_compile_features`, `target_compile_options` and `target_link_libraries`. Avoid project-wide `include_directories()`, `add_definitions()` and broad `add_compile_options()` for project code.

C++23 and no compiler extensions are target/project requirements, not ambient compiler assumptions.

### Explicit project sources

Do not use `file(GLOB)` / `GLOB_RECURSE` to discover project source files. Adding a project `.cpp` must be an explicit build-graph change.

Upstream dependencies may internally choose different source-management policy; do not rewrite vendored/upstream build internals merely to mimic project style.

### Presets

`CMakePresets.json` is shared project configuration:

- `native` — Debug native core/tests, no GDExtension;
- `dev` — Debug native core/tests + `template_debug` GDExtension;
- `release` — Release verification + `template_release` GDExtension.

Machine-specific overrides belong in uncommitted `CMakeUserPresets.json`.

## Dependency policy

`cmake/Dependencies.cmake` is the active native dependency owner. It uses immutable revisions for GoogleTest and godot-cpp. Current Godot binding configuration is API 4.7, single precision; exact values are summarized in [`VERSIONS.md`](VERSIONS.md).

Initial configure/bootstrap may acquire dependencies from the network. After population, ordinary tests/playtests must not silently update dependency checkouts.

Dependency roles remain narrow:

- GoogleTest — tests only;
- godot-cpp — `world_sim_gdextension` only;
- future serialization libraries stay at serialization/adapter/persistence boundaries rather than becoming live domain state.

Project warning policy must not accidentally turn third-party warnings into project `-Werror` failures.

## Native tests

`sim_tests` links `sim_core` plus `GTest::gtest_main`; it does not link Godot. CTest discovery is used after the executable exists.

Prefer independent fresh state per test. Use behavior-oriented suite/test names. Prefer `EXPECT_*` when multiple observations can still be useful after one failure; use `ASSERT_*` when continuing would be invalid.

Do not add a custom `main()` while also linking `gtest_main` unless there is a real process-level initialization need.

The architecture check is a separate CTest test because dependency direction is a build/source property, not a gameplay assertion.

## Local front door

Bootstrap the project-owned CMake/Ninja environment, validate the exact Godot baseline and configure pinned dependencies:

```bash
python3 tools/bootstrap.py
```

Then use the thin orchestration front door:

```bash
.venv/bin/python tools/dev.py configure --preset native
.venv/bin/python tools/dev.py build --preset native
.venv/bin/python tools/dev.py test --preset native
.venv/bin/python tools/dev.py check --preset dev
.venv/bin/python tools/dev.py play --scenario smoke
```

Windows uses the interpreter under `.venv\\Scripts`.

`tools/dev.py` delegates to CMake, CTest and `tools/play.py`; it is deliberately not a custom build system.

## Python tooling policy

Use a repository `.venv`, `pathlib`, typed public functions where useful and explicit argv subprocess execution with deliberate cwd/environment.

Child work must be bounded by timeout/deadline and cleaned up with `try/finally` or context managers. Avoid `shell=True` unless a reviewed command truly requires shell semantics.

Forbidden shortcuts include:

- `os.system("godot ... &")`;
- `pkill godot` / `killall godot`;
- unbounded polling;
- a second Playwright/Chromium runner for the Godot client;
- Python code that reimplements the authoritative simulation.

## Playtest process ownership

`tools/play.py` owns one repository playtest process group, a non-blocking lock, timeout, logs and artifact validation. The complete contract is in [`../VERIFICATION.md`](../VERIFICATION.md#godot-playtest-supervisor).

## Agent traps

- adding SCons beside CMake because an upstream example uses SCons;
- changing dependency versions during ordinary gameplay work;
- treating a configured immutable pin as a verified build/load;
- letting Godot headers leak into `sim_core`;
- using source GLOBs “temporarily”;
- fetching dependencies from an unrelated test/play command;
- treating compile-green as gameplay acceptance.
