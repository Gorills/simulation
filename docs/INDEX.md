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
| ACTIVE | [`engineering/PLATFORM_CAPABILITIES.md`](engineering/PLATFORM_CAPABILITIES.md) | Required production native window/input capability contract, current Fenster limitation, candidate evidence, and selection acceptance gate | Production platform requirements/evidence/selection changes |
| ACTIVE | [`ARCHITECTURE.md`](ARCHITECTURE.md) | Implemented foundation structure, graphical diagnostic boundary, vendoring, Linux verification lifecycle, and current production-platform gate | Implemented architecture changes |
| ACTIVE | [`design/GAME_UI_DESIGN_SKILL.md`](design/GAME_UI_DESIGN_SKILL.md) | Canonical agent-neutral workflow for designing future player-visible UI without template/default drift | UI design workflow or design-task routing changes |
| ACTIVE | [`design/DESIGN_SYSTEM.md`](design/DESIGN_SYSTEM.md) | Single persistent memory for durable approved visual/UI decisions; intentionally `UNSET` until real UI establishes them | A real UI task approves a durable token/pattern/direction |
| ACTIVE | [`design/UI_UX_RULES.md`](design/UI_UX_RULES.md) | Hard usability, accessibility, interaction-state, copy, and craft floor for future native game UI | Usability/accessibility/craft policy changes or target requirements become concrete |
| ACTIVE | [`design/DESIGN_REVIEW.md`](design/DESIGN_REVIEW.md) | Bounded screenshot/interaction review procedure, severity model, evidence set, and completion gate for player-visible UI | Visual QA/evidence procedure changes |
| TEMPORARY | [`specs/PROJECT_SPEC.md`](specs/PROJECT_SPEC.md) | Condensed product contract and roadmap from the original greenfield brief; explicitly separates foundation from future gameplay | Product invariants or roadmap change; delete when retirement condition is met |

### Supporting provenance

- [`design/SOURCES.md`](design/SOURCES.md) records the exact external design-skill revisions and first-party usability/accessibility sources reviewed to produce the repo-owned design policy. It is provenance, not a competing rulebook.

## Current stage

The repository contains a verified **development/graphics diagnostic foundation** plus an active **UI/UX policy foundation**, and intentionally contains no gameplay or game-UI implementation.

The current Fenster-based graphical path proves that the agent can build/run a native framebuffer window, inject real X11 keyboard input and capture real window pixels. It is now explicitly classified as a **diagnostic fixture**, not the final production game platform API. Production window/input selection remains blocked until a candidate passes the acceptance gate in `engineering/PLATFORM_CAPABILITIES.md` inside the agent environment.

The design foundation defines how future UI must be conceived, remembered across sessions, reviewed, and checked for usability/accessibility. It does **not** claim that a palette, font system, component library, widget set, HUD, menu, inventory, dialogue UI, or other real game surface already exists. `design/DESIGN_SYSTEM.md` therefore keeps those decisions `UNSET` until a real bounded UI task approves them.

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