# Game UI Design Skill

**Status:** ACTIVE

This is the canonical agent-neutral procedure for future player-visible UI work. It is repository documentation, not an installed Claude/Codex/community skill. Source provenance is in [`SOURCES.md`](SOURCES.md); durable approved choices live only in [`DESIGN_SYSTEM.md`](DESIGN_SYSTEM.md); hard usability/accessibility rules live in [`UI_UX_RULES.md`](UI_UX_RULES.md); bounded visual QA lives in [`DESIGN_REVIEW.md`](DESIGN_REVIEW.md).

At the current repository stage there is **no game UI implementation**. This file is policy, not evidence that a renderer, widget set, palette, font system, or visual direction exists.

## 1. When this applies

Use this procedure whenever a task creates, changes, reviews, or documents something a player will see or operate: HUD, menus/settings, inventory/trade/equipment, dialogue, maps/logs/journal, tooltips/prompts, controls, loading/error/empty states, typography, color, iconography, layout, motion, focus/navigation, accessibility, redesign, polish, or screenshot critique.

Do not apply it to backend/simulation/build/platform work with no player-visible surface.

## 2. Mandatory read order

Before player-visible work read:

1. repository root `AGENTS.md`;
2. `docs/INDEX.md`;
3. `docs/engineering/AGENT_RUNBOOK.md`;
4. this file;
5. `DESIGN_SYSTEM.md`;
6. `UI_UX_RULES.md`;
7. `DESIGN_REVIEW.md`;
8. existing UI implementation/tokens/components/evidence once they exist.

Do not re-install or re-research external UI skill packs during ordinary UI work. Their reviewed contribution is already captured in repo-owned policy; revisit upstream only in a separate bounded design-foundation task or when a concrete new requirement exposes a gap.

## 3. Design from the game, not a template

Do not begin from “dashboard”, “RPG inventory”, “dark fantasy”, “modern clean”, a remembered component library, or a web trend. Begin with the actual surface:

- **Player** — who uses it, at what moment, with what input mode?
- **Task** — what verb/outcome matters now?
- **World** — what real concepts/materials/tools/institutions/vocabulary belong here?
- **Information** — what must be noticed first, second, and only on demand?
- **Consequence** — what misunderstanding/error is costly?
- **Scene** — what visual competition exists behind/around the UI?

A design is specific when structure and language arise from these facts rather than from a reusable AI template.

## 4. Design Read before code

Form this compact internal read before implementation:

```text
Surface:    [screen/component]
Player:     [who / moment / input]
Task:       [single primary verb/outcome]
Mode:       [Operate | Read | Experience]
Feel:       [specific experiential words]
World cues: [legitimate domain materials/vocabulary/metaphors]
Risk:       [most important misunderstanding/failure]
```

Modes:

- **Operate** — completion/control lead; scanability, predictability, state feedback, focus/input clarity outrank decoration.
- **Read** — comprehension/reading hierarchy lead.
- **Experience** — authored expression may lead, but required control/state remains clear.

Choose mode per surface, not once for the whole game.

## 5. Domain exploration

Before a new visual direction identify:

1. at least five concrete domain concepts/materials/artifacts;
2. the plausible **color world** before naming UI values;
3. one **signature** structural/typographic/interaction idea that could belong specifically to this game/surface;
4. at least three obvious **defaults to avoid** for this interface category.

The purpose is not forced novelty. It is to expose generic defaults before they silently become the design.

## 6. Explicit design dials

An approved project direction records three 1–10 baselines in `DESIGN_SYSTEM.md`:

- `DESIGN_VARIANCE`: regular/symmetrical/predictable -> irregular/asymmetrical/experimental;
- `MOTION_INTENSITY`: essentially static -> motion as major authored material;
- `VISUAL_DENSITY`: sparse/airy -> dense/cockpit-like information.

They are intent coordinates, not presets. Accessibility and task constraints override them. Until a real direction is approved, `UNSET` is correct.

## 7. Pre-implementation checkpoint

Immediately before player-visible code make these decisions explicit internally or in the task artifact when useful:

```text
Intent:        player / task / scene / feeling
Hierarchy:     focal element and how it wins
Palette:       semantic color roles and rationale
Depth:         coherent layer model
Typography:    roles, readability, rationale
Spacing:       rhythm and density
Motion:        what moves, why, what stays still
States:        applicable interaction/system states
Accessibility: focus, nav, text, contrast, redundant cues, motion risk
Signature:     one memorable element if warranted
```

“Because it is common/modern” is not a rationale.

## 8. Persistent design memory

`DESIGN_SYSTEM.md` is the **only** canonical home for approved durable visual decisions across chats/sessions: project read, design dials, semantic color roles, typography, spacing, depth, iconography, motion vocabulary, focus/state language, reusable components/patterns, surface exceptions, and retired patterns.

When a choice becomes durable, update it in the same bounded task. Do not create `.interface-design/system.md`, `.claude/`, `.agents/`, `DESIGN.md`, or other hidden parallel design-memory systems.

## 9. Reuse before invention

Before adding a visual constant, control, state behavior, or layout primitive:

1. inspect `DESIGN_SYSTEM.md`;
2. inspect existing primitives/components;
3. reuse when semantic role matches;
4. extend when the need is genuinely adjacent;
5. create a new pattern only when existing patterns cannot express the task without semantic distortion.

Consistency is not sameness. Reuse semantic rules; allow macrostructure to vary when the player task differs. Inventory, trade, dialogue, settings, and journal should not all inherit one reflexive card-grid skeleton.

## 10. Craft principles

- Give each surface one focal task/state and make it win through coordinated position, contrast, size/weight/value, grouping, and space.
- Use typography as first-class design material; validate real copy, long names/numbers, localization-like expansion, wrapping, and scaling.
- Use semantic color roles once tokens exist; color is never the only important information channel.
- Use one coherent depth language; do not stack border + shadow + glow + blur decoratively.
- Spend boldness selectively. A clear signature idea beats many unrelated tricks.
- Use motion only to clarify state, spatial relationship, causality, feedback, or a deliberate authored experiential moment; preserve critical meaning without animation and plan reduced-motion behavior when motion exists.
- Player-facing copy uses player vocabulary, action verbs, truthful state, useful recovery, and honest placeholders. Never invent world data/metrics/history merely to fill a composition.

## 11. Interaction state coverage

Every interactive component explicitly covers applicable states and marks non-applicable ones: default, pointer-over, focused, pressed/active, selected/toggled, disabled/unavailable, busy/pending, error/invalid, success/confirmed, empty/no-content, destructive/irreversible emphasis.

Focus is always discernible, hover is never the only required cue, focus differs semantically from selection, unavailable differs from low priority, back/cancel/confirm behavior is predictable, and overlays restore meaningful context/focus.

## 12. Anti-slop gate

Reject reflexive patterns such as:

- repeated equal card grids regardless of task;
- excessive container nesting/pills for plain text/actions;
- decorative glass/glow/gradient everywhere;
- fake terminal/monospace styling without meaning;
- tiny uppercase labels as universal hierarchy;
- arbitrary extreme roundedness with no system rationale;
- one macrostructure cloned across unrelated surfaces;
- invented data to make screenshots attractive;
- random novelty with no task/world relation.

The cure is not default minimalism. It is specificity, hierarchy, coherent tokens, honest content, and structural decisions tied to the player task.

## 13. Usability/accessibility gate

Before visual polish, apply `UI_UX_RULES.md`, including system-state visibility, control/recoverability, recognition over recall, error prevention/recovery, focus/navigation/context, text/readability, contrast, redundant cues beyond color, transient UI timing, motion/reduced-motion, and dark-pattern prohibition.

Hard accessibility/truth/control constraints outrank aesthetic intensity.

## 14. Bounded visual QA

When a real game UI exists, use `DESIGN_REVIEW.md`:

1. finish the bounded implementation;
2. run the real executable and interaction path;
3. capture representative states in one batch;
4. inspect the whole batch before editing again;
5. produce one consolidated severity-ranked defect list;
6. fix all in-scope severity 4/3 and coherent severity 2 defects together;
7. recapture only affected representative states;
8. confirm once and stop.

Maximum two meaningful visual review rounds per normal feature task. Do not enter endless screenshot micro-polish.

The current `platform_graphics_smoke` screenshot proves only window/input/capture infrastructure. It is **not** game/UI acceptance evidence.

## 15. Refinement vs redesign

**Refinement** preserves approved hierarchy, palette, type roles, depth language, component semantics, and interaction model while improving craft. **Redesign** intentionally changes one or more of those. Do not perform a stealth redesign while claiming to polish.

## 16. Future completion contract

Once the real game executable/playtest runner exists, a player-visible change is verified only when the smallest sufficient evidence shows both behavior and appearance: functional/unit/protocol evidence as applicable, real executable interaction, representative rendered-state captures, design review with no unresolved in-scope severity 4/3 defect, focus/navigation exercise when controls changed, semantic/debug proof for authoritative outcomes visuals cannot prove alone, durable `DESIGN_SYSTEM.md` update for newly approved decisions, and docs hygiene.

Compilation alone is never visual verification.

## 17. What was intentionally not imported

External sources contain valuable reasoning but also web/tool-specific material. This repository does **not** adopt as required stack: React/Next/Tailwind/component-library prescriptions, CSS/browser implementation rules, hosted fonts, web responsive breakpoint doctrine, mobile/iOS policy without an actual target, automatic theme/style rotation, community-skill installers/hooks, external synthetic quality scores, or another hidden design-memory file.

See `SOURCES.md` for exact provenance and filtering decisions.
