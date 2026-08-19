# Godot and binding version policy

This file owns version semantics for the Godot/GDExtension boundary. Exact active dependency revisions belong in machine-readable build/lock files once bootstrap exists.

Runtime ownership lives in [`../ARCHITECTURE.md`](../ARCHITECTURE.md); implementation guidance lives in [`gdextension.md`](gdextension.md); upgrade evidence requirements live in [`../VERIFICATION.md`](../VERIFICATION.md).

Facts below were re-checked against upstream sources on **2026-08-19**.

## Engine baseline

- **Godot Engine baseline:** `4.7.1-stable`.
- **GDExtension API target:** Godot `4.7`.
- The locally observed editor during repository preparation was `4.7.1.stable.mono`; Mono capability does not make C# part of this project's gameplay stack.
- Patch upgrades inside the 4.7 line are deliberate dependency changes, not automatic “latest patch” behavior.

For reproducible local work, use the exact engine patch recorded by the future toolchain lock rather than an arbitrary editor found on `PATH`.

## godot-cpp is versioned independently

Starting with **godot-cpp v10**, godot-cpp uses its own version line. The engine version, API target and binding revision are separate dimensions:

```text
Godot Engine version:      4.7.1-stable
GDExtension API target:    4.7
godot-cpp version/commit:  v10 line, exact immutable revision
```

Do not write or configure `godot-cpp 4.7.x` for this project.

The published `godot-cpp 10.0.0-rc1` predates 4.7 API support: its upstream build configuration lists supported API versions through `4.6`. It is therefore **not** the project pin for a 4.7 target.

Current upstream `godot-cpp` master checked during this preparation supports API 4.7. The observed upstream head on 2026-08-19 was:

```text
9c8aeff0f58ad030f3d1030e8262de1322cd0ccd
```

That SHA is a **bootstrap candidate, not verified project evidence**. A bootstrap implementation may pin this exact immutable SHA or a newer deliberately reviewed immutable v10 revision, but it must prove build/load before declaring the dependency working.

Never configure a floating `master`, `main` or `latest` dependency.

## API compatibility rule

GDExtensions are designed to support later Godot minor versions when built against an older supported API floor, subject to the extension's actual API use and Godot compatibility contract. The reverse direction is not assumed: an extension built against a newer API may not load in an older engine.

Godot and extension floating-point precision must match.

For this repository initially:

- target `api_version=4.7` unless a deliberate compatibility decision chooses a lower API floor;
- use the standard single-precision engine/binding configuration unless both sides are intentionally changed;
- set the future `.gdextension` loader compatibility floor consistently with the API actually targeted (for a true 4.7 target, `compatibility_minimum = "4.7"`);
- do not generate API metadata from an arbitrary editor binary; custom engine builds require matching API metadata.

If broader backwards compatibility later matters, target the **lowest Godot API actually required by implemented features** and prove that compatibility. Do not lower the declared API merely to suppress a warning.

## Machine-readable ownership once bootstrap exists

Use one source of truth per version dimension:

```text
tools/toolchain.lock        exact developer/runtime tool versions
cmake/Dependencies.cmake    exact immutable native dependency revisions
CMake presets/options       explicit GDExtension API target
Godot project config        project engine feature/config compatibility
*.gdextension               loader compatibility and emitted libraries
```

The exact final filenames may evolve with real bootstrap code; do not create placeholder lock files solely to satisfy this diagram. The invariant is **one machine-readable active owner**, not the spelling of the file.

README/AGENTS/editor rules must link here or to active machine-readable pins rather than copy exact SHAs.

## Upgrade gate

A Godot patch update, GDExtension API-target change or godot-cpp revision update is a bounded dependency change.

Before accepting it, verify at minimum:

1. native Simulation Core/protocol build/tests remain Godot-free;
2. the GDExtension target builds against the selected immutable godot-cpp revision;
3. the pinned Godot engine loads the produced extension without manifest/API/ABI errors;
4. the smallest playable scenario still runs through the authoritative round-trip;
5. machine-readable pins and this policy agree.

No report may call a godot-cpp revision “working” until those checks actually ran.

## Primary upstream sources

- Godot release archive / 4.7.1 baseline: <https://godotengine.org/download/archive/>
- Godot 4.7 GDExtension compatibility: <https://docs.godotengine.org/en/4.7/engine_details/engine_api/gdextension/what_is_gdextension.html>
- Godot 4.7 `.gdextension` configuration: <https://docs.godotengine.org/en/4.7/engine_details/engine_api/gdextension/gdextension_file.html>
- godot-cpp repository/versioning: <https://github.com/godotengine/godot-cpp>
- godot-cpp build options/API versions: <https://github.com/godotengine/godot-cpp/blob/master/tools/godotcpp.py>
