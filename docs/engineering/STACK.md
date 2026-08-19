# Stack architecture for implementers

This directory contains **how/how-not** guidance for the selected stack. It does not own product intent, simulation-model policy, verification acceptance or exact active dependency pins.

Canonical cross-cutting owners:

- product/playable invariants: [`../PRODUCT.md`](../PRODUCT.md)
- runtime dependency direction: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- simulation/modeling: [`../MODELING.md`](../MODELING.md)
- verification/playtest: [`../VERIFICATION.md`](../VERIFICATION.md)
- milestones: [`../ROADMAP.md`](../ROADMAP.md)
- Godot/GDExtension version policy: [`VERSIONS.md`](VERSIONS.md)

## Routes

| Area | Guide | Owns |
| --- | --- | --- |
| C++23 Simulation Core + protocol | [`cpp.md`](cpp.md) | ownership, headers, error model, deterministic coding, native tests |
| GDExtension adapter | [`gdextension.md`](gdextension.md) | thin Godot ↔ protocol translation, registration, manifest/build seam |
| Godot 4 client | [`godot.md`](godot.md) | scenes, typed GDScript, input, resources, presentation wiring |
| CMake / GoogleTest / Python tools | [`cmake-python.md`](cmake-python.md) | build/test/tooling implementation |
| versions | [`VERSIONS.md`](VERSIONS.md) | Godot/API/godot-cpp version dimensions and upgrade gate |
| upstream evidence | [`SOURCES.md`](SOURCES.md) | primary sources behind these rules |

## Non-negotiable dependency direction

```text
godot/ -> src/adapters/gdextension -> src/protocol -> src/sim

src/sim      !-> Godot
src/protocol !-> Godot
godot-cpp    !-> src/sim or src/protocol
```

A folder name is not an architecture boundary. CMake target edges, imports and tests must enforce the direction once runtime code exists.

## Guidance policy

These docs encode stack practices only when they serve this repository's chosen boundaries and are supported by upstream/project evidence.

| Source | Adopt | Do not cargo-cult |
| --- | --- | --- |
| Godot official best practices | scene independence, parent-mediated wiring, restrained Autoloads, typed GDScript, Resource semantics, InputMap | world truth in nodes/Autoloads |
| godot-cpp + official GDExtension examples | ClassDB registration, manifest semantics, API compatibility rules | turning a tutorial `GDCLASS` into the domain model |
| godot-cpp template | binding/manifest conventions | a second SCons build graph merely because the template uses it |
| C++ Core Guidelines | RAII, clear ownership, self-contained headers, concrete interfaces | abstract frameworks before a second real implementation |
| CMake docs | target-scoped configuration, explicit source lists, pinned dependencies | global include/compile settings or source GLOBs |
| GoogleTest primer | independent fixtures and appropriate EXPECT/ASSERT use | tests requiring Godot to prove native world rules |
| Python subprocess docs | argv-based bounded process control | shell command strings / process-wide kill shortcuts |

## First implementation shape

Milestone 0 in [`../ROADMAP.md`](../ROADMAP.md) should establish only the real dependency spine:

1. a Godot-free native `sim_core`/protocol target;
2. a deterministic native test;
3. a thin GDExtension facade linking the pinned godot-cpp revision;
4. a Godot 2D scene that sends one semantic input and renders the returned projection;
5. a bounded playtest proving the round-trip.

Do not start with ECS, an Autoload EventBus/world, C#, a web/WASM client, SCons in parallel with CMake, or a Python simulation.

## Updating this folder

- Prefer primary/upstream documentation; record important sources in [`SOURCES.md`](SOURCES.md).
- Do not repeat exact dependency SHAs that belong in build/lock files.
- When a rule can be tested mechanically, make the executable check authoritative and keep prose concise.
- Do not add host-agent workflow rules here; agent-context design lives in [`../AGENT_CONTEXT.md`](../AGENT_CONTEXT.md) and durable workflow belongs to AI Layer.
