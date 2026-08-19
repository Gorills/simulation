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

`sim_core` contains only authoritative domain code and is Godot/protocol-free. `sim_protocol` validates/translates application requests and privately depends on `sim_core`. `world_sim_gdextension` links `sim_protocol` plus godot-cpp rather than owning the domain world directly.

This target split is intentional: dependency direction is expressed by the build graph, not only by folder names.

## CMake principles

### Targets, not directory globals

Use `target_sources`, `target_include_directories`, `target_compile_features`, `target_compile_options` and `target_link_libraries`. Avoid project-wide `include_directories()`, `add_definitions()` and broad `add_compile_options()` for project code.

C++23 and no compiler extensions are target/project requirements. Project-owned C++ targets share the warning policy through `world_sim_enable_project_warnings()`; do not push project warnings into third-party targets.

### Explicit project sources

Do not use `file(GLOB)` / `GLOB_RECURSE` to discover project source files. Adding a project `.cpp` is an explicit build-graph change.

### Presets

`CMakePresets.json` is shared project configuration:

- `native` — Debug native core/protocol/tests, no GDExtension;
- `dev` — Debug native graph + `template_debug` GDExtension;
- `release` — Release verification + `template_release` GDExtension.

Machine-specific overrides belong in uncommitted `CMakeUserPresets.json`.

## Dependency and cache policy

`cmake/Dependencies.cmake` owns immutable GoogleTest and godot-cpp revisions. Current Godot binding configuration is API 4.7, single precision; exact values live in [`VERSIONS.md`](VERSIONS.md).

Initial bootstrap/configure may acquire pinned dependencies. CMake build trees and FetchContent state live under repository `build/`; the Python tool environment lives under `.venv/`; playtest artifacts live under `.cache/`; Godot project import state lives under `godot/.godot/`; generated GDExtension libraries live under `godot/bin/`. These paths are gitignored.

Bootstrap installs the pinned Python-hosted CMake/Ninja tools with pip `--no-cache-dir`, so the project does not intentionally populate the user's pip download cache. Do not redirect project build or FetchContent caches into arbitrary global directories.

Ordinary incremental build/test/play commands reuse the repository-local state and must not silently update dependency revisions.

Dependency roles remain narrow:

- GoogleTest — tests only;
- godot-cpp — `world_sim_gdextension` only;
- future serialization libraries stay at serialization/adapter/persistence boundaries rather than becoming live domain state.

## Native tests

Native tests follow the same layer ownership as production code:

- `sim_core_tests` links only `sim_core` + `GTest::gtest_main`;
- `protocol_tests` links `sim_protocol` + `GTest::gtest_main`.

A domain test should not need protocol DTOs merely to exercise a world law. A protocol test should verify boundary validation/translation and the observable application result without duplicating the world rule.

Prefer independent fresh state per test and behavior-oriented test names. The architecture check remains a separate CTest test because dependency direction is a source/build property.

## Human front door

For normal human playtesting, the root `Makefile` is a **thin convenience layer**, not another build system.

```bash
make play
```

The first invocation:

1. runs `tools/bootstrap.py` and creates the repository `.venv`;
2. validates the pinned Godot executable;
3. configures/fetches the pinned `dev` native dependency graph;
4. records a bootstrap stamp inside `.venv`.

Then every invocation runs the incremental `dev` build, performs a headless incremental Godot import to refresh project-local `.godot` metadata and global `class_name` registrations, and launches the configured project directly. This makes a clean checkout independent of whether the project was ever opened in the Godot Editor. When bootstrap inputs change, Make invalidates the stamp and reruns bootstrap automatically.

Companion commands:

```bash
make check
make smoke
```

`make check` runs the local dev configure/build/test path. `make smoke` builds, refreshes Godot project metadata the same way, and then executes the bounded artifact-producing playtest supervisor.

The Makefile only delegates to `tools/bootstrap.py` / `tools/dev.py`; build truth remains CMake presets and the repository Python tools.

## Direct game launch versus bounded playtest

`tools/run_game.py` is the **interactive human run** path. It validates the exact Godot version and expected debug GDExtension library, runs a bounded metadata refresh:

```text
godot --headless --path <repo>/godot --import
```

and only after that starts the human-owned game session:

```text
godot --path <repo>/godot
```

The second command has no `--editor` argument. `godot/project.godot` owns `run/main_scene`, so this launches the game directly rather than opening the editor. The headless import is intentionally repeated before play because it is incremental and prevents stale/missing global script-class metadata after a clean checkout or pull that adds a new `class_name` script.

Godot's official command-line contract documents `--path <directory>`, `--headless` and `--import` for project selection/import; Godot 4.7's IDE guidance also notes that removing `--editor` runs the project instead of editing it:

- <https://docs.godotengine.org/en/4.7/tutorials/editor/command_line_tutorial.html>
- <https://docs.godotengine.org/en/4.7/engine_details/development/configuring_an_ide/visual_studio_code.html>

`tools/play.py` is different: it owns one bounded repository playtest process group, lock, timeout, logs and artifact validation. It performs the same metadata refresh before the bounded run. Use it for evidence, not as the ordinary unlimited play session. The complete contract is in [`../VERIFICATION.md`](../VERIFICATION.md#godot-playtest-supervisor).

## Focused Python front door

Agents and developers may bypass Make when they need a specific operation:

```bash
python3 tools/bootstrap.py
python3 tools/dev.py configure --preset native
python3 tools/dev.py build --preset native
python3 tools/dev.py test --preset native
python3 tools/dev.py check --preset dev
python3 tools/dev.py run
python3 tools/dev.py play --scenario smoke
```

`tools/dev.py` delegates to CMake, CTest, `tools/run_game.py` and `tools/play.py`; it is deliberately not a custom build system.

## Python tooling policy

Use a repository `.venv`, `pathlib`, typed public functions where useful and explicit argv subprocess execution with deliberate cwd/environment.

Bounded automated child work needs a timeout/deadline and explicit cleanup. Interactive `run_game.py` is the deliberate exception only for the final game process; its preparatory headless import remains bounded. Avoid `shell=True` unless a reviewed command truly requires shell semantics.

Forbidden shortcuts include:

- `os.system("godot ... &")`;
- `pkill godot` / `killall godot`;
- unbounded automated polling;
- a second Playwright/Chromium runner for the Godot client;
- Python code that reimplements the authoritative simulation.

## Agent traps

- putting protocol DTOs back into `sim_core` because it is convenient for one call site;
- letting the GDExtension adapter own or mutate `sim::World` directly instead of using the protocol/application surface;
- adding SCons beside CMake because an upstream example uses SCons;
- putting real build logic into the Makefile instead of delegating to repository owners;
- changing dependency versions during ordinary gameplay work;
- treating a configured immutable pin as a verified build/load;
- letting Godot headers leak into `sim_core`;
- using source GLOBs “temporarily”;
- fetching/updating unrelated dependencies from an ordinary play command;
- treating compile-green as gameplay acceptance.
