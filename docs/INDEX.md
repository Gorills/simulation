# Documentation Index

This is the navigation root and catalog for repository knowledge. Agents should start here instead of scanning all Markdown files.

## Status model

- **ACTIVE** — current canonical source of truth.
- **TEMPORARY** — intentionally short-lived guidance used during a phase of development; must state when it can be removed.
- **RETIRED** — no longer active. Retired temporary documents should normally be deleted rather than kept in the active docs tree; Git history preserves them.

## Canonical documents

| Status | Document | Purpose | Update when |
|---|---|---|---|
| ACTIVE | [`DOCUMENTATION.md`](DOCUMENTATION.md) | Documentation structure, indexing, lifecycle, and anti-drift rules | Documentation conventions change |
| ACTIVE | [`engineering/DEVELOPMENT_RULES.md`](engineering/DEVELOPMENT_RULES.md) | Stack, architecture boundaries, determinism, local verification, playtest and agent engineering rules | Engineering/toolchain contract changes |
| ACTIVE | [`GAME.md`](GAME.md) | What is actually playable now: controls, current scenario, milestone and player-facing gaps | Player-visible gameplay changes |
| ACTIVE | [`ARCHITECTURE.md`](ARCHITECTURE.md) | Implemented dependency direction, authority, protocol, client and playtest lifecycle | Implemented architecture changes |
| TEMPORARY | [`specs/PROJECT_SPEC.md`](specs/PROJECT_SPEC.md) | Condensed product/architecture contract and milestone roadmap derived from the original greenfield brief | Product invariants or roadmap change; delete when retirement condition is met |

## Future canonical docs

Create these only when the corresponding implementation exists; do not create placeholders:

- `MODELING_POLICY.md` — accepted historical baseline, magic deviations, uncertainty, fidelity rules, anti-overmodeling decisions.
- `models/<mechanic>.md` — only for serious mechanics whose causal model needs an explicit contract.
- `decisions/<decision>.md` — only for expensive or hard-to-reverse architecture decisions.

When one of these becomes real, add it to the canonical table above in the same task.

## Navigation rule

Do not add a new documentation file without deciding which of these is true:

1. it is a new canonical source of truth and must be indexed here;
2. it is subordinate detail linked from an indexed canonical document;
3. it is temporary and has an explicit retirement condition;
4. it should not exist.

For the full policy, see [`DOCUMENTATION.md`](DOCUMENTATION.md).
