# Documentation router

This repository deliberately has **no monolithic `TZ.md`**. Each durable concern has one canonical owner, and agent/bootstrap files route to that owner instead of copying it.

## Choose the smallest relevant source

| Concern | Canonical source | Open when |
| --- | --- | --- |
| change readiness, dependency admission, premature/conflicting requests | [`CHANGE_ADMISSION.md`](CHANGE_ADMISSION.md) | before production implementation |
| product goals, playable invariants, player-role philosophy | [`PRODUCT.md`](PRODUCT.md) | gameplay/product work |
| runtime ownership, dependency direction, protocol/GDExtension seam, AI Layer runtime boundary | [`ARCHITECTURE.md`](ARCHITECTURE.md) | architecture/integration work |
| performance budgets, profiling, frame pacing, hot-path/scaling rules | [`PERFORMANCE.md`](PERFORMANCE.md) | any non-trivial runtime/simulation/bridge optimization or performance-sensitive feature |
| determinism, NPC/economy/politics/social/history/magic/fidelity policy | [`MODELING.md`](MODELING.md) | simulation/model changes |
| circa-1200 non-magical historical baseline | [`research/high-medieval-baseline-c1200.md`](research/high-medieval-baseline-c1200.md) | economy/social/politics/institution/history assumptions |
| historical counterfactual + adaptive causal fidelity decision | [`decisions/0005-historical-counterfactual-and-causal-fidelity.md`](decisions/0005-historical-counterfactual-and-causal-fidelity.md) | changing role/status, magic participation or simulation-detail policy |
| authoritative exact 3D location/spatial samples | [`models/spatial-location.md`](models/spatial-location.md) + [`decisions/0006-authoritative-spatial-contract.md`](decisions/0006-authoritative-spatial-contract.md) | movement/location/spatial ownership/interpolation work |
| protocol/Godot integer range and unsigned Core counter conversion | [`decisions/0007-protocol-integer-range.md`](decisions/0007-protocol-integer-range.md) | exporting tick/revision/seed/epoch or adding protocol integer fields |
| Core snapshot/restore and deterministic continuation | [`decisions/0008-core-snapshot-restore.md`](decisions/0008-core-snapshot-restore.md) | adding authoritative Core state or persistence/replay work |
| Simulation authority vs spatial implementation choice, offscreen exact-spatial limits, future external/LLM decision-source boundary | [`decisions/0009-simulation-authority-and-decision-sources.md`](decisions/0009-simulation-authority-and-decision-sources.md) | scaling spatial work, adding offscreen continuation/time systems, or considering external NPC policy/language control |
| M2 acceptance composition + post-locomotion resource ordering | [`decisions/0010-household-resource-composition-and-application-order.md`](decisions/0010-household-resource-composition-and-application-order.md) | changing acceptance-village ownership, actor collection, observed/resource discovery scope or revision-only Consume ordering |
| grounded locomotion behavior + neutral acceptance arena | [`models/grounded-locomotion.md`](models/grounded-locomotion.md) | movement solver, collision, slope/step/grounding or first test-arena work |
| first NPC living need / causal rest task | [`models/living-need.md`](models/living-need.md) | Milestone 1 need/task behavior, NPC rest decision or its presentation evidence |
| household grain stock, bounded Consume and derived shortage | [`models/household-resource.md`](models/household-resource.md) | Milestone 2 household resource state/rules and their causal semantics |
| whole Milestone 2 household-resource loop contract | [`milestones/m2-household-resource-loop.md`](milestones/m2-household-resource-loop.md) | gift / household-transfer / work acceptance, naming, and whole-M2 evidence |
| whole Milestone 3 playable social-consequence contract | [`milestones/m3-playable-social-consequence.md`](milestones/m3-playable-social-consequence.md) | active M3 vignette, remembered material aid, reciprocal opportunity, ordinary-play readability and acceptance boundaries |
| first M3 remembered material-aid social state | [`models/remembered-material-aid.md`](models/remembered-material-aid.md) | qualifying Gift attribution, bounded household favour memory, atomic material-social causality and snapshot semantics |
| runtime UI localization, supported locales, translation keys/plurals | [`engineering/localization.md`](engineering/localization.md) | adding/changing player-visible text, locale selection or localization verification |
| tests, playtest supervisor, evidence, local DoD | [`VERIFICATION.md`](VERIFICATION.md) | verification/tooling/gameplay acceptance |
| current milestone direction | [`ROADMAP.md`](ROADMAP.md) | choosing/understanding implementation target; active M3 detail routes to its milestone contract and accepted evidence lives in [`VERIFICATION.md`](VERIFICATION.md) |
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
| Godot runtime localization / ru-en catalogs / locale policy | [`engineering/localization.md`](engineering/localization.md) |
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

## Fact placement and maintenance

Use the document type to decide where a fact is allowed to live:

| Fact type | Owner |
| --- | --- |
| durable product outcome/non-goal | `PRODUCT.md` |
| durable runtime/authority/dependency invariant | `ARCHITECTURE.md` or one accepted ADR |
| mechanic causality, terminology, domain acceptance values | `MODELING.md` or the relevant `models/<mechanic>.md` |
| current implementation status and immediate sequencing | `ROADMAP.md` |
| verification commands, proof obligations and collected evidence | `VERIFICATION.md` |
| stack-specific implementation procedure | the relevant `engineering/*.md` guide |
| external/historical/scientific evidence | `research/` or `engineering/SOURCES.md` as appropriate |

A non-owner document may give a **short stable summary** so it remains readable in isolation, then link to the owner. It must not copy a changing implementation paragraph, status checklist, protocol-version history, numeric acceptance table or verification result merely for convenience.

If one implementation change appears to require editing the same changing fact in three or more prose documents, treat that as a documentation-design defect: choose the canonical owner, keep the full fact there, and replace the other copies with stable summaries/links unless each copy expresses a genuinely different contract.

When a feature PR changes source and also touches several broad documents, review those doc edits for ownership before merging. More documentation edits are not automatically safer; unnecessary copies increase stale-prose risk.

## Documentation rules

- One canonical owner per durable fact.
- Do not copy exact dependency/tool versions into README, AGENTS or editor rules; route to machine-readable pins / [`engineering/VERSIONS.md`](engineering/VERSIONS.md).
- Do not copy AI Layer Work/Task/Epic procedure into this repository.
- ADRs record consequential decisions and rationale; they are not alternate full specs or current-status logs.
- `ROADMAP.md` owns current milestone status/next sequencing; architecture/model documents should not become parallel roadmaps.
- `VERIFICATION.md` owns proof obligations/evidence; do not duplicate test-result inventories across architecture, roadmap and engineering guides.
- Engineering guides explain how/how-not for a chosen stack; they do not redefine product goals or copy complete mechanic specifications.
- Model documents exist only for serious mechanics that need a durable causal contract and own the detailed mechanic semantics they introduce.
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
