# Documentation Index

**Status:** ACTIVE

This is the navigation root and catalog for repository knowledge. Agents start here instead of scanning every Markdown file.

## Status model

- **ACTIVE** — current canonical source of truth.
- **TEMPORARY** — intentionally phase-bound guidance; it must state when it can be removed.
- **RETIRED** — no longer active. Retired temporary documents are normally deleted from the active tree; Git history preserves them.

## Canonical documents

| Status | Document | Purpose | Update when |
|---|---|---|---|
| ACTIVE | [`DOCUMENTATION.md`](DOCUMENTATION.md) | Documentation structure, indexing, lifecycle, and anti-drift rules | Documentation conventions change |
| ACTIVE | [`engineering/AGENT_RUNBOOK.md`](engineering/AGENT_RUNBOOK.md) | Exact commands and operational procedure for agents: setup, build, tests, graphical verification, artifacts, teardown, triage | Agent-visible command/lifecycle/tooling behavior changes |
| ACTIVE | [`engineering/DEVELOPMENT_RULES.md`](engineering/DEVELOPMENT_RULES.md) | Stack, C++/CMake/platform/dependency rules, verification policy, and bounded engineering workflow | Engineering/toolchain contract changes |
| ACTIVE | [`ARCHITECTURE.md`](ARCHITECTURE.md) | Implemented foundation structure, graphical platform boundary, vendoring, and Linux verification lifecycle | Implemented architecture changes |
| TEMPORARY | [`specs/PROJECT_SPEC.md`](specs/PROJECT_SPEC.md) | Condensed product contract and roadmap from the original greenfield brief; explicitly separates foundation from future gameplay | Product invariants or roadmap change; delete when retirement condition is met |

## Current stage

The repository contains a verified **development/graphics foundation** and intentionally contains no gameplay implementation.

Do not create `GAME.md` until the first real player-visible gameplay path exists. When it exists, `GAME.md` becomes the canonical description of what a player can actually do and must be added to the table above in the same task.

## Future canonical docs

Create these only when corresponding implementation exists; do not create placeholders:

- `GAME.md` — actual playable behavior, controls, player-visible state and known gaps.
- `MODELING_POLICY.md` — accepted historical baseline, magic deviations, uncertainty/fidelity rules and anti-overmodeling policy once those rules become operational.
- `models/<mechanic>.md` — serious mechanics whose causal model needs an explicit durable contract.
- `decisions/<decision>.md` — expensive or hard-to-reverse architecture decisions whose rationale must be retained.

## Navigation rule

Before adding a documentation file, decide which is true:

1. it is a new canonical source of truth and must be indexed here;
2. it is subordinate detail linked from an indexed canonical document;
3. it is temporary and has an explicit retirement condition;
4. it should not exist.

For full policy, see [`DOCUMENTATION.md`](DOCUMENTATION.md).
