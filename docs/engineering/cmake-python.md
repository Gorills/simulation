# CMake, tests, and Python tools

Contract: TZ §2.3–2.7, §6.7–6.8. Python is **tooling only**, never Simulation Core.

Canonical sources: [CMake `file(GLOB)`](https://cmake.org/cmake/help/latest/command/file.html) (explicitly **not** for sources), [Using dependencies / FetchContent](https://cmake.org/cmake/help/latest/guide/using-dependencies.html), [GoogleTest primer](https://google.github.io/googletest/primer.html), [Python `subprocess` security](https://docs.python.org/3/library/subprocess.html#security-considerations).

## CMake: how

**Targets, not directory globals.** TZ: no project-wide `include_directories()` / `add_definitions()`. Use `target_include_directories`, `target_compile_options`, `target_compile_features(cxx_std_23)` with `CMAKE_CXX_EXTENSIONS OFF`.

**List sources explicitly.** CMake: *“We do not recommend using GLOB to collect a list of source files.”* `CONFIGURE_DEPENDS` is still not reliable on all generators and still costs a check every rebuild. godot-jolt globs anyway — **do not copy that**. Adding a `.cpp` means editing `CMakeLists.txt` / `target_sources` so every clone regenerates.

**Presets:** `CMakePresets.json` is the shared contract. `CMakeUserPresets.json` is local and uncommitted. Ninja is the generator once bootstrap installs it. Do not claim cmake/ninja exist until re-checked (TZ §2.1).

**Dependencies:** one `cmake/Dependencies.cmake`, exact version/commit + integrity hash. GoogleTest **1.17.0 test-only**. nlohmann/json only on serialization/adapter/persistence targets. godot-cpp 4.7.x only on the GDExtension target. No floating `main`/`latest`. Network fetch is **bootstrap**, not an accidental side effect of `ctest` or `play.py`.

**FetchContent:** `FetchContent_Declare` + `FetchContent_MakeAvailable` with a pinned tag. Optional `FIND_PACKAGE_ARGS` if a system package is acceptable; this repo prefers pinned source for reproducibility. `INSTALL_GTEST OFF`. `gtest_force_shared_crt` only if a Windows CRT mismatch appears — do not cargo-cult it on Linux.

**Warnings:** TZ §6.7 on **project** targets. Do not `add_compile_options` so godot-cpp/gtest inherit `-Werror`.

**Tests:** `enable_testing()`, CTest, `gtest_discover_tests` after the test binary exists. Test executables link `sim_core` + `GTest::gtest_main`. They must build **without** Godot.

```cmake
# Good
target_sources(sim_core PRIVATE src/sim/application/step.cpp)
target_link_libraries(sim_tests PRIVATE sim_core GTest::gtest_main)

# Bad
include_directories(src)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS src/*.cpp)
add_compile_options(-Werror)
```

## GoogleTest: how / how not

| Do | Don't |
| --- | --- |
| One fixture instance per test (framework already does this) | Share a process-global `World` across tests |
| `TEST(Trade, RejectsInsufficientFunds)` — no `_` in macro names | `TEST(Trade_Test, ...)` |
| `EXPECT_*` for multiple checks; `ASSERT_*` before a deref | `ASSERT_TRUE` on every equality |
| Determinism tests: same seed + commands → same projection | Tests that read `std::chrono::system_clock` |
| Prove a rule in C++ first | “We’ll cover it in Godot later” as the only test |

## Python tools: how

TZ: `.venv`, pinned **ruff**, type hints on public functions, `pathlib`, one Godot playtest entry `tools/play.py`.

**Process control.** `subprocess.run([...], cwd=..., env=..., timeout=..., check=False)` with an **argv list**. Official Python docs: `shell=True` is a security hazard and a quoting bug factory. Forbidden without a recorded exception.

**Lifecycle.** Godot is spawned in **one** module. `try/finally` or a context manager always terminates the child (TZ: never `pkill godot`). No unbounded polling; bound waits with timeout.

**Do not** use Playwright, Chromium, or a second simulator in Python. Do not import `src/sim` via bindings invented ad hoc — native tests already own that.

```python
# Good
subprocess.run(
    [godot, "--path", str(godot_dir), "--", "--scenario", name],
    cwd=root,
    timeout=120,
    check=False,
)

# Bad
os.system("godot --path godot &")
subprocess.run("pkill godot", shell=True)
```

## Agent traps

- Running `pip install` of random Godot tooling into the system interpreter instead of `.venv`.
- Fetching godot-cpp during `ctest` because Dependencies.cmake was not bootstrapped.
- Adding SCons “for the extension” beside CMake.
- GLOB “until we have more files”.
- Custom `main()` in every test file while also linking `gtest_main` (duplicate entry).
- Treating compile-green as DoD (TZ §39).