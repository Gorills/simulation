# Documentation router

This repository deliberately has **no monolithic `TZ.md`**. Each durable concern has one canonical owner, and agent/bootstrap files route to that owner instead of copying it.

## Choose the smallest relevant source

| Concern | Canonical source | Open when |
| --- | --- | --- |
| product goals, playable invariants, player-role philosophy | [`PRODUCT.md`](PRODUCT.md) | gameplay/product work |
| runtime ownership, dependency direction, protocol/GDExtension seam, AI Layer runtime boundary | [`ARCHITECTURE.md`](ARCHITECTURE.md) | architecture/integration work |
| determinism, NPC/economy/politics/social/history/magic/save modeling | [`MODELING.md`](MODELING.md) | simulation/model changes |
| tests, playtest supervisor, evidence, local DoD | [`VERIFICATION.md`](VERIFICATION.md) | verification/tooling/gameplay acceptance |
| current milestone direction | [`ROADMAP.md`](ROADMAP.md) | choosing/understanding implementation target |
| agent context packaging | [`AGENT_CONTEXT.md`](AGENT_CONTEXT.md) | maintaining AGENTS/Cursor/Claude/Gemini/skills |
| stack-specific implementation | [`engineering/STACK.md`](engineering/STACK.md) | coding/build work |
| Godot/GDExtension versions | [`engineering/VERSIONS.md`](engineering/VERSIONS.md) | engine/binding/dependency work |
| consequential rationale | [`decisions/`](decisions/) | when an accepted choice may be affected |
| serious mechanic model | `models/<mechanic>.md` | once that mechanic has a durable model contract |
| load-bearing research | `research/` | when historical/scientific evidence needs a durable artifact |

## Engineering routes

| Area | Guide |
| --- | --- |
| C++23 Simulation Core + protocol | [`engineering/cpp.md`](engineering/cpp.md) |
| Simulation authority ↔ Godot commands/projections/materialization | [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md) |
| Godot 4 typed GDScript presentation client | [`engineering/godot.md`](engineering/godot.md) |
| Godot UI design system / responsive layout | [`engineering/ui-design-system.md`](engineering/ui-design-system.md) |
| GDExtension adapter | [`engineering/gdextension.md`](engineering/gdextension.md) |
| CMake / GoogleTest / Python tooling | [`engineering/cmake-python.md`](engineering/cmake-python.md) |
| primary upstream references | [`engineering/SOURCES.md`](engineering/SOURCES.md) |

## Entry points for tools and humans

| File | Purpose |
| --- | --- |
| [`README.md`](../README.md) | human project entry |
| [`AGENTS.md`](../AGENTS.md) | universal short agent bootstrap |
| [`CLAUDE.md`](../CLAUDE.md) | Claude import shim for `AGENTS.md` |
| [`GEMINI.md`](../GEMINI.md) | Gemini import shim for `AGENTS.md` |
| [`.cursor/rules/`](../.cursor/rules/) | path-scoped Cursor reminders only |

Host files do not own product/engineering truth. They point here.

## Authority and conflicts

Different document types answer different questions; avoid pretending one total ordering solves every conflict.

For **implemented behavior**, current source, build configuration, lock files and executable tests are authoritative evidence.

For **intentional architecture decisions**, accepted ADRs explain why a choice exists. If current implementation intentionally changes a consequential accepted decision, update/supersede the ADR rather than silently diverging.

For **required behavior/design not yet implemented**, use the canonical concern owner above.

A stale prose statement is a documentation defect. Do not change working code merely to imitate stale prose without first identifying the intended outcome.

## Documentation rules

- One canonical owner per durable fact.
- Do not copy exact dependency/tool versions into README, AGENTS or editor rules; route to machine-readable pins / [`engineering/VERSIONS.md`](engineering/VERSIONS.md).
- Do not copy AI Layer Work/Task/Epic procedure into this repository.
- ADRs record consequential decisions and rationale; they are not alternate full specs.
- Engineering guides explain how/how-not for a chosen stack; they do not redefine product goals.
- Model documents exist only for serious mechanics that need a durable causal contract.
- Research artifacts should record source quality, uncertainty and the modeling consequence, not become an uncurated link dump.
- Planned files/directories must not be described as if they already exist.
- When a mechanically enforceable invariant gains real code/targets, prefer an executable check over additional prose.

## Progressive disclosure for agents

Default route:

```text
AGENTS.md
  -> this INDEX
  -> one canonical concern document
  -> one relevant engineering/model/ADR source
  -> current code/tests/build files
```

Do not preload all documentation “for safety”. Weak models benefit from fewer competing rules; strong models can load deeper context when the task actually requires it.
