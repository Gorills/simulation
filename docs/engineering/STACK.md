# Stack architecture for agents

Product invariants live in [`docs/specs/TZ.md`](../specs/TZ.md). This folder is **how** to write maintainable code on the chosen stack so the first implementation does not bake in failure modes that are expensive to unwind.

| Layer | Owns | Guide |
| --- | --- | --- |
| C++23 Simulation Core + protocol | world laws, commands, projections | [`cpp.md`](cpp.md) |
| GDExtension adapter | Godot ↔ protocol translation | [`gdextension.md`](gdextension.md) |
| Godot 4 GDScript client | scenes, input, camera, UI, audio | [`godot.md`](godot.md) |
| CMake / CTest / GoogleTest / Python tools | build, native tests, playtest orchestration | [`cmake-python.md`](cmake-python.md) |

Primary citations: [`SOURCES.md`](SOURCES.md). Stack choice: [`docs/decisions/0001-cpp-godot-gdextension.md`](../decisions/0001-cpp-godot-gdextension.md).

## How these docs relate to the TZ

- **TZ** = what must be true (authority, determinism, playable slices).
- **These files** = how/how-not on C++, Godot, GDExtension, CMake, Python, with real upstream examples.
- **Cursor rules** = short reminders when matching files are open. Details stay here.
- **`docs/ARCHITECTURE.md`** remains planned until runtime code exists (dependency graph of real targets). Do not invent that file from this guide.

Do not copy toolchain versions or AI Layer procedure here. Versions stay in TZ §2.

## Non-negotiable dependency direction

```text
godot/  ->  adapters/gdextension  ->  protocol  ->  sim
sim !-> Godot
protocol !-> Godot
godot-cpp !-> src/sim
```

A folder named `domain/` is not a boundary. Imports and CMake `target_link_libraries` are.

## What to copy from real projects — and what not to

| Source | Copy | Do not copy |
| --- | --- | --- |
| Godot 4.7 official best practices | scene independence, parent-mediated wiring, autoload restraint, typed GDScript, Resource sharing | putting world truth in nodes |
| godot-cpp + official template | ClassDB registration, `.gdextension` manifest, pin bindings to engine series | SCons as a second build graph; putting sim laws in GDCLASS nodes |
| godot-jolt | Godot-free library + thin engine binding | `file(GLOB)` of sources |
| C++ Core Guidelines | RAII, ownership in signatures, self-contained headers | exceptions as ordinary gameplay results (TZ uses `std::expected`) |
| CMake docs | target-scoped options, explicit sources, pinned FetchContent | global `include_directories`, floating `main` tags |
| GoogleTest primer | independent fixtures, `EXPECT_*` vs `ASSERT_*`, `gtest_main` | tests that require Godot to prove a sim rule |

## First code the agents should produce

Milestone 0 in TZ §38, in this shape:

1. `sim_core` static/shared library with no Godot headers.
2. Native GoogleTest that submits one command and asserts one projection.
3. Thin GDExtension facade that forwards that command.
4. Godot scene that reads InputMap and displays the projection.

Do not start with ECS, autoload EventBus, SCons, C#, or a second Python simulator.