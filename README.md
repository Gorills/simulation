# World Simulation

Playable systemic RPG: one C++23 Simulation Core, one Godot 4 client.

- Contract: [`docs/specs/TZ.md`](docs/specs/TZ.md)
- Docs map: [`docs/INDEX.md`](docs/INDEX.md)
- Stack how/how-not: [`docs/engineering/STACK.md`](docs/engineering/STACK.md)
- Stack decision: [`docs/decisions/0001-cpp-godot-gdextension.md`](docs/decisions/0001-cpp-godot-gdextension.md)
- Agent pointer: [`AGENTS.md`](AGENTS.md)

## Stack

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` C++23 | world laws, state, outcomes | Godot, input, frames, UI |
| `src/protocol` | commands, results, events, projections | rendering |
| `src/adapters/gdextension` | Godot ↔ protocol translation | domain rules |
| `godot/` | scenes, input, camera, UI, audio | inventory/money/world truth |

Details: [TZ §3–5](docs/specs/TZ.md#3-архитектура-верхнего-уровня), [ADR 0001](docs/decisions/0001-cpp-godot-gdextension.md), [engineering how/how-not](docs/engineering/STACK.md).

## Local tools (checked 2026-08-19)

Present: g++ 13.3.0 (C++23 `std::expected`), Python 3.12.3, Godot 4.7.1.stable.mono, gdb.

Missing until bootstrap: cmake, ninja. Do not claim they are installed.

The installed Godot binary is the Mono editor. This project still uses **GDScript + C++ GDExtension**, not C#. Toolchain policy: [TZ §2](docs/specs/TZ.md#2-проверенный-локальный-toolchain-и-выбранный-стек).

## First spine

Do not restore deleted Fenster/TypeScript/WASM work. Next implementation step is [Milestone 0](docs/specs/TZ.md#38-roadmap-слои-развиваются-одновременно): native `sim_core` + Godot 2D client + one authoritative move command.
