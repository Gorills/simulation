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
sim_core_tests ------> sim_core
                           ^
                           |
protocol_tests ------> sim_protocol
                           ^
                           |
world_sim_gdextension ----+----> godot::cpp
```

`sim_core` contains only authoritative domain code and is Godot/protocol-free. `sim_protocol` is a separate static library that validates/translates application requests and has a private dependency on `sim_core`. `world_sim_gdextension` links `sim_protocol` plus godot-cpp rather than owning the domain world directly.

This target split is intentional: the desired dependency direction is expressed by the build graph, not only by folder names.

## CMake principles

### Targets, not directory globals

Use `target_sources`, `target_include_directories`, `target_compile_features`, `target_compile_options` and `target_link_libraries`. Avoid project-wide `include_directories()`, `add_definitions()` and broad `add_compile_options()` for project code.

C++23 and no compiler extensions are target/project requirements, not ambient compiler assumptions.

Project-owned C++ targets share the same warning policy through the small `world_sim_enable_project_warnings()` CMake helper. Keep warnings target-scoped; do not push project warning settings into third-party targets.

### Explicit project sources

Do not use `file(GLOB)` / `GLOB_RECURSE` to discover project source files. Adding a project `.cpp` must be an explicit build-graph change.

Upstream dependencies may internally choose different source-management policy; do not rewrite vendored/upstream build internals merely to mimic project style.

### Presets

`CMakePresets.json` is shared project configuration:

- `native` — Debug native core/protocol/tests, no GDExtension;
- `dev` — Debug native graph + `template_debug` GDExtension;
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

Native tests follow the same layer ownership as production code:

- `sim_core_tests` links only `sim_core` + `GTest::gtest_main`;
- `protocol_tests` links `sim_protocol` + `GTest::gtest_main`.

A domain test should not need protocol DTOs merely to exercise a world law. A protocol test should verify boundary validation/translation and the observable application result without duplicating the world rule.

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

- putting protocol DTOs back into `sim_core` because it is convenient for one call site;
- letting the GDExtension adapter own or mutate `sim::World` directly instead of using the protocol/application surface;
- adding SCons beside CMake because an upstream example uses SCons;
- changing dependency versions during ordinary gameplay work;
- treating a configured immutable pin as a verified build/load;
- letting Godot headers leak into `sim_core`;
- using source GLOBs “temporarily”;
- fetching dependencies from an unrelated test/play command;
- treating compile-green as gameplay acceptance.
