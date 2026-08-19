# CMake, native tests, and Python developer tools

Python in this project is developer tooling/orchestration only; it is not a second Simulation Core.

Canonical contracts:

- target/dependency direction: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- Godot/godot-cpp version dimensions: [`VERSIONS.md`](VERSIONS.md)
- verification/playtest lifecycle: [`../VERIFICATION.md`](../VERIFICATION.md)
- C++ implementation policy: [`cpp.md`](cpp.md)

Primary upstream references are tracked in [`SOURCES.md`](SOURCES.md): CMake dependency/file guidance, GoogleTest and Python `subprocess` security/lifecycle behavior.

## CMake principles

### Targets, not directory-wide globals

Use target-scoped configuration:

- `target_sources`;
- `target_include_directories`;
- `target_compile_features`;
- `target_compile_options`;
- `target_link_libraries`.

Avoid project-wide `include_directories()`, `add_definitions()` and broad `add_compile_options()` for project code.

The initial project uses C++23 without compiler extensions. Express that through target features/project configuration rather than relying on ambient compiler defaults.

### Explicit source lists

Do not use `file(GLOB)` / `GLOB_RECURSE` to discover project source files. CMake documentation explicitly discourages source collection by glob because source additions/removals are not an explicit build-graph change.

Adding a `.cpp` means updating the owning target's source list.

### Presets

Shared project configuration belongs in `CMakePresets.json`. Machine-specific developer overrides belong in uncommitted `CMakeUserPresets.json`.

Do not state that CMake/Ninja/Clang are installed merely because the project intends to use them. Bootstrap/current environment must prove tool availability.

## Dependency policy

Declare third-party native dependencies centrally (planned location: `cmake/Dependencies.cmake`) and use immutable reviewed revisions/versions.

Initial intended dependency roles:

- GoogleTest — tests only;
- nlohmann/json — serialization/persistence/adapter boundaries only, not live domain state;
- godot-cpp — GDExtension target only.

Exact active revisions belong in build/lock files once those files exist. Godot/godot-cpp compatibility semantics belong in [`VERSIONS.md`](VERSIONS.md).

Never use floating `main`, `master` or `latest` as the project pin.

Initial dependency acquisition may use explicit bootstrap/network access, but `ctest`, an ordinary playtest or a narrow local check should not unexpectedly mutate/fetch dependencies as a hidden side effect.

Third-party warnings must not inherit the project's strongest warning/`-Werror` policy accidentally.

## FetchContent

When FetchContent is used, declare an immutable dependency revision and keep the ownership centralized.

Do not add system-package fallback complexity until there is a concrete environment requirement. Do not cargo-cult Windows/gtest flags onto platforms that do not need them.

## Native tests

Enable CTest and use GoogleTest discovery after test executables exist.

Test executables prove simulation/protocol behavior by linking native project targets plus GoogleTest; they do not link Godot merely to access world rules.

Use `GTest::gtest_main` unless a test executable has a real process-level initialization need not expressible through fixtures/environment.

```cmake
add_library(sim_core STATIC)
target_sources(sim_core PRIVATE src/sim/application/step.cpp)

target_link_libraries(sim_tests PRIVATE
  sim_core
  GTest::gtest_main
)
```

Bad:

```cmake
include_directories(src)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_compile_options(-Werror)
```

## Python tooling policy

Use a repository `.venv` and pin the Python developer tooling actually required. Ruff is the intended formatter/linter when Python tooling is bootstrapped; the active pinned version belongs in machine-readable dependency configuration.

Use:

- `pathlib` for paths;
- typed public tool functions where it improves clarity;
- `subprocess` with an explicit argv sequence;
- explicit `cwd` and deliberate environment handling;
- bounded timeouts/deadlines;
- `try/finally` or context managers for child/process cleanup.

Avoid `shell=True` unless an exceptional, reviewed command genuinely requires shell semantics. Do not build command strings that rely on shell quoting.

```python
subprocess.run(
    [godot, "--path", str(godot_dir), "--", "--scenario", name],
    cwd=root,
    timeout=120,
    check=False,
)
```

Forbidden process shortcuts include `os.system("godot ... &")`, `pkill godot`, `killall godot` and unbounded polling.

## Playtest process ownership

Godot process lifecycle is implemented in one supervisor path once `tools/play.py` exists. The full contract for locking, timeouts, artifacts and process-group ownership is in [`../VERIFICATION.md`](../VERIFICATION.md#godot-playtest-supervisor).

Do not create a second test runner around Playwright/Chromium, and do not create Python bindings that reimplement the native simulation merely for test convenience.

## Agent traps

- Installing random project tooling globally instead of using the repository environment/bootstrap.
- Fetching godot-cpp during an unrelated `ctest` or playtest.
- Adding SCons beside CMake because upstream godot-cpp examples use SCons.
- Using GLOB “temporarily”.
- Writing custom `main()` while also linking `gtest_main`.
- Letting global compiler flags poison third-party targets.
- Treating compile-green as gameplay acceptance; see [`../VERIFICATION.md`](../VERIFICATION.md).
