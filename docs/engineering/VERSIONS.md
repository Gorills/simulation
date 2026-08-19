# Godot and binding version policy

This file owns version semantics for the Godot/GDExtension boundary. **Active pins live in machine-readable build/tool files** and take precedence over copied prose.

Runtime ownership lives in [`../ARCHITECTURE.md`](../ARCHITECTURE.md); implementation guidance lives in [`gdextension.md`](gdextension.md); upgrade evidence requirements live in [`../VERIFICATION.md`](../VERIFICATION.md).

Facts were re-checked against upstream sources on **2026-08-19**.

## Active baseline

| Dimension | Active value | Machine-readable owner |
| --- | --- | --- |
| Godot Engine baseline | `4.7.1` stable | `tools/toolchain.lock.json` |
| GDExtension API target | `4.7` | `CMakePresets.json` / `cmake/Dependencies.cmake` |
| godot-cpp revision | `9c8aeff0f58ad030f3d1030e8262de1322cd0ccd` | `cmake/Dependencies.cmake` |
| godot-cpp precision | `single` | CMake presets/dependency configuration |
| GoogleTest revision | `52eb8108c5bdec04579160ae17225d66034bd723` | `cmake/Dependencies.cmake` |
| bootstrap CMake | `3.31.6` | `requirements-dev.txt` / toolchain lock |
| bootstrap Ninja | `1.12.1` | `requirements-dev.txt` / toolchain lock |

The locally observed editor during repository preparation was `4.7.1.stable.mono`; Mono capability does not make C# part of this project's gameplay stack.

Patch upgrades inside the 4.7 engine line are deliberate dependency changes, not automatic “latest patch” behavior.

## godot-cpp is versioned independently

Starting with **godot-cpp v10**, godot-cpp uses its own version line. The engine version, API target and binding revision are separate dimensions:

```text
Godot Engine version:      4.7.1-stable
GDExtension API target:    4.7
godot-cpp commit:          9c8aeff0f58ad030f3d1030e8262de1322cd0ccd
```

Do not write or configure `godot-cpp 4.7.x` for this project.

The older published `godot-cpp 10.0.0-rc1` predates 4.7 API support, so it is not the project pin for a 4.7 target.

The configured immutable commit exposes `GODOTCPP_API_VERSION` through `4.7` in upstream CMake and defines `godot-cpp` with alias `godot::cpp`. The project deliberately pins that commit instead of depending on a floating `master` branch.

**Configured is not verified.** Until `world_sim_gdextension` actually builds and the pinned Godot 4.7.1 engine loads it, this is a configured dependency pin rather than verified runtime evidence.

## API compatibility rule

GDExtensions are designed to support later Godot minor versions when built against an older supported API floor, subject to the extension's actual API use and Godot compatibility contract. The reverse direction is not assumed: an extension built against a newer API may not load in an older engine.

Godot and extension floating-point precision must match.

For this repository initially:

- target `api_version=4.7`;
- use standard single precision on engine/bindings;
- set `.gdextension` `compatibility_minimum = "4.7"`;
- do not generate API metadata from an arbitrary editor binary;
- custom engine builds require their matching API metadata;
- do not lower the API target merely to suppress a warning.

If broader backwards compatibility later matters, target the **lowest Godot API actually required by implemented features** and prove that compatibility.

## Machine-readable ownership

Use one active source of truth per version dimension:

```text
tools/toolchain.lock.json  exact local engine/tool policy
requirements-dev.txt       project-owned CMake/Ninja bootstrap versions
cmake/Dependencies.cmake   immutable native dependency revisions
CMakePresets.json          GDExtension API/precision/target presets
godot/project.godot        project engine feature compatibility
godot/bin/*.gdextension    loader compatibility and emitted libraries
```

README, AGENTS and editor rules must link to these owners or this policy rather than duplicate dependency SHAs.

## Upgrade gate

A Godot patch update, GDExtension API-target change, godot-cpp revision update or other native dependency revision change is its own bounded dependency task.

Before accepting it, verify at minimum:

1. native Simulation Core/protocol build/tests remain Godot-free;
2. the GDExtension target builds against the selected immutable godot-cpp revision;
3. the pinned Godot engine loads the produced extension without manifest/API/ABI errors;
4. the smallest playable scenario runs through the authoritative round-trip;
5. machine-readable pins and documentation agree.

No report may call a dependency revision “working” until those checks actually ran.

## Primary upstream sources

- Godot release archive / 4.7.1 baseline: <https://godotengine.org/download/archive/>
- Godot 4.7 GDExtension compatibility: <https://docs.godotengine.org/en/4.7/engine_details/engine_api/gdextension/what_is_gdextension.html>
- Godot 4.7 `.gdextension` configuration: <https://docs.godotengine.org/en/4.7/engine_details/engine_api/gdextension/gdextension_file.html>
- godot-cpp repository/versioning: <https://github.com/godotengine/godot-cpp>
- godot-cpp CMake options/API versions: <https://github.com/godotengine/godot-cpp/blob/master/cmake/godotcpp.cmake>
- GoogleTest releases: <https://github.com/google/googletest/releases>
