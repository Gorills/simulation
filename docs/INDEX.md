# Documentation map

Canonical contract: [`docs/specs/TZ.md`](specs/TZ.md).

Do not copy toolchain versions or AI Layer procedure into extra files. Stack how/how-not lives in [`docs/engineering/`](engineering/STACK.md). Product invariants stay in the TZ.

## Entry

| Document | Audience | Contents |
| --- | --- | --- |
| [`README.md`](../README.md) | humans | purpose, layer table, verified local tools |
| [`AGENTS.md`](../AGENTS.md) | agents | pointer to TZ + engineering STACK; AI Layer owns workflow |
| [`docs/specs/TZ.md`](specs/TZ.md) | humans + agents | product, architecture, modeling, engineering invariants |
| [`.cursor/rules/`](../.cursor/rules/) | Cursor | short enforceable splits; details in TZ + [`docs/engineering/`](engineering/STACK.md) |
| [`docs/engineering/STACK.md`](engineering/STACK.md) | agents | C++ / Godot / GDExtension / CMake / Python how and how-not |

## Architecture

| Document | Status |
| --- | --- |
| [`docs/decisions/0001-cpp-godot-gdextension.md`](decisions/0001-cpp-godot-gdextension.md) | accepted |
| [`docs/engineering/STACK.md`](engineering/STACK.md) | stack how/how-not for agents (C++, Godot, GDExtension, CMake/Python) |
| `docs/ARCHITECTURE.md` | planned — write when runtime code exists |
| `docs/GAME.md` | planned — current playable facts only |
| `docs/MODELING_POLICY.md` | planned — extract from TZ when a model needs a home |
| `docs/models/` | planned — one file per serious mechanic |

## Rules of this tree

- [`docs/specs/TZ.md`](specs/TZ.md) is the only full product/engineering contract.
- [`docs/engineering/`](engineering/STACK.md) is implementation architecture (how/how-not). It does not restate toolchain versions or AI Layer procedure.
- ADRs record irreversible choices; they do not restate the whole TZ.
- Planned files marked planned above are not present. Do not claim they exist.
- Old root path `world_sim_greenfield_TZ_v2.md` is a redirect only.
