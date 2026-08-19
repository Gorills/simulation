# Documentation router

This repository deliberately has **no monolithic `TZ.md`**. Each durable concern has one canonical owner, and agent/bootstrap files route to that owner instead of copying it.

## Choose the smallest relevant source

| Concern | Canonical source | Open when |
| --- | --- | --- |
| change readiness, dependency admission, premature/conflicting requests | [`CHANGE_ADMISSION.md`](CHANGE_ADMISSION.md) | before production implementation |
| product goals, playable invariants, player-role philosophy | [`PRODUCT.md`](PRODUCT.md) | gameplay/product work |
| runtime ownership, dependency direction, protocol/GDExtension seam, AI Layer runtime boundary | [`ARCHITECTURE.md`](ARCHITECTURE.md) | architecture/integration work |
| determinism, NPC/economy/politics/social/history/magic/fidelity policy | [`MODELING.md`](MODELING.md) | simulation/model changes |
| circa-1200 non-magical historical baseline | [`research/high-medieval-baseline-c1200.md`](research/high-medieval-baseline-c1200.md) | economy/social/politics/institution/history assumptions |
| historical counterfactual + adaptive causal fidelity decision | [`decisions/0005-historical-counterfactual-and-causal-fidelity.md`](decisions/0005-historical-counterfactual-and-causal-fidelity.md) | changing role/status, magic participation or simulation-detail policy |
| authoritative exact 3D location/spatial samples | [`models/spatial-location.md`](models/spatial-location.md) + [`decisions/0006-authoritative-spatial-contract.md`](decisions/0006-authoritative-spatial-contract.md) | movement/location/spatial ownership/interpolation work |
| grounded locomotion behavior + neutral acceptance arena | [`models/grounded-locomotion.md`](models/grounded-locomotion.md) | movement solver, collision, slope/step/grounding or first test-arena work |
| tests, playtest supervisor, evidence, local DoD | [`VERIFICATION.md`](VERIFICATION.md) | verification/tooling/gameplay acceptance |
| current milestone direction | [`ROADMAP.md`](ROADMAP.md) | choosing/understanding implementation target |
| agent context packaging | [`AGENT_CONTEXT.md`](AGENT_CONTEXT.md) | maintaining AGENTS/Cursor/Claude/Gemini/skills |
| stack-specific implementation | [`engineering/STACK.md`](engineering/STACK.md) | coding/build work |
| Godot/GDExtension versions | [`engineering/VERSIONS.md`](engineering/VERSIONS.md) | engine/binding/dependency work |
| consequential rationale | [`decisions/`](decisions/) | when an accepted choice may be affected |
| serious mechanic model | `models/<mechanic>.md` | once that mechanic has a durable causal model contract |
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

For **implemented behavior**, current source, build configuration, lock files and executable tests are authoritative evidence.

For **intentional architecture/model decisions**, accepted ADRs explain why a choice exists. If current implementation intentionally changes a consequential accepted decision, update/supersede the ADR rather than silently diverging.

For **required behavior/design not yet implemented**, use the canonical concern owner above.

A stale prose statement is a documentation defect. Do not change working code merely to imitate stale prose without first identifying the intended outcome.

A user directive is product intent, not a new source of implemented truth. Before production code, run the request through [`CHANGE_ADMISSION.md`](CHANGE_ADMISSION.md); if the desired outcome requires changing an accepted contract, change/supersede the canonical owner deliberately rather than bypassing it in one feature.

## Documentation rules

- One canonical owner per durable fact.
- Do not copy exact dependency/tool versions into README, AGENTS or editor rules; route to machine-readable pins / [`engineering/VERSIONS.md`](engineering/VERSIONS.md).
- Do not copy AI Layer Work/Task/Epic procedure into this repository.
- ADRs record consequential decisions and rationale; they are not alternate full specs.
- Engineering guides explain how/how-not for a chosen stack; they do not redefine product goals.
- Model documents exist only for serious mechanics that need a durable causal contract.
- Research artifacts record source quality, uncertainty and modeling consequence rather than becoming an uncurated link dump.
- Broad historical research is not permission to guess precise local rates/rules; narrow the source when a mechanic depends on them.
- Planned files/directories must not be described as if they already exist.
- When a mechanically enforceable invariant gains real code/targets, prefer an executable boundary over additional prose.

## Progressive disclosure for agents

Default route for a code-changing request:

```text
AGENTS.md
  -> CHANGE_ADMISSION.md
  -> this INDEX
  -> one canonical concern document
  -> one relevant engineering/model/ADR/research source
  -> current code/tests/build files
```

Do not preload all documentation “for safety”. Weak models benefit from fewer competing rules; strong models can load deeper context when the task actually requires it.
