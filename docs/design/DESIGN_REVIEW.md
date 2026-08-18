# Game UI Design Review

**Status:** ACTIVE

This document defines the bounded visual/usability review method for future player-visible UI. Design procedure is [`GAME_UI_DESIGN_SKILL.md`](GAME_UI_DESIGN_SKILL.md), hard rules are [`UI_UX_RULES.md`](UI_UX_RULES.md), and approved durable choices are stored in [`DESIGN_SYSTEM.md`](DESIGN_SYSTEM.md).

At the current repository stage no real game UI exists, so this is a future acceptance procedure. The existing `platform_graphics_smoke` screenshot is platform evidence only.

## 1. Review principles

- Review the **real rendered executable**, not prose or a synthetic mock, when evaluating shipped UI.
- Functional success and visual success are separate claims.
- A screenshot proves appearance, not all interaction behavior; pair visual evidence with input/state evidence.
- Review representative real content/states, not only a perfect happy-path specimen.
- Find defects in one batched pass, then fix them together.
- The approved brief/design system is visual authority; this rubric is the quality floor.
- Do not enter endless screenshot micro-polish loops.

## 2. Severity scale

| Severity | Name | Meaning | Completion policy |
|---|---|---|---|
| 0 | Not a defect | preference/disagreement with no material usability/craft impact | no action |
| 1 | Cosmetic | visible polish issue with low task/accessibility impact | fix when cheap/in scope |
| 2 | Minor | noticeable friction, ambiguity, inconsistency, or hierarchy/craft problem | fix in bounded task when related |
| 3 | Major | significant task failure, accessibility barrier, misleading state, or serious visual break | must fix before task completion |
| 4 | Catastrophic | blocks task, traps player, hides critical information, or causes severe accessibility/system-truth failure | task cannot ship |

Severity considers frequency, impact, and persistence/recoverability. Do not inflate every aesthetic preference into severity 3.

## 3. Representative evidence set

Capture the smallest set that covers the changed surface. Depending on scope it may include:

- primary/default view;
- focused control;
- selected/toggled state;
- disabled/unavailable state;
- loading/pending state;
- error/recovery state;
- success/confirmation state;
- empty state;
- open overlay/menu/tooltip when changed;
- long/localization-like text, large numbers, or scaling state when relevant;
- accessibility/high-contrast/text-scale/motion setting state when the task changes it.

Do not manufacture irrelevant states merely to increase screenshot count.

## 4. Review dimensions

Review the whole evidence batch against these dimensions.

### Intent fidelity

- Does the surface solve the named player task?
- Does it match the approved `DESIGN_SYSTEM.md` direction/dials?
- Does it feel specific to this game/surface rather than a generic template?
- Does expression fit the surface mode (`Operate`, `Read`, `Experience`)?

### Information architecture and hierarchy

- Is one primary task/state obvious?
- Can player identify screen/context/location/entity affected?
- Are groups/proportions meaningful?
- Is secondary information truly secondary?
- Does a squint/blur test preserve major structure?

### Typography and copy

- Is hierarchy obvious and important text readable at target conditions?
- Do real long names/numbers/wrapping remain stable?
- Are labels player-facing and action-specific?
- Do errors say what happened and how to recover?
- Is any copy fabricated merely for visual completeness?

### Color and contrast

- Do important elements meet applicable contrast targets?
- Does semantic color mean the same thing consistently?
- Is any important state conveyed by color alone?
- Is legibility checked over the actual rendered background/composition?

### Spacing, density, and alignment

- Are related elements grouped more tightly than unrelated groups?
- Are alignments intentional?
- Does density fit the player task/use scene?
- Does space establish hierarchy rather than simply make the layout sparse?

### Surfaces and depth

- Is there one coherent depth/layer language?
- Do overlays/popovers read above their parent surfaces?
- Are borders/shadows/glows/blur purposeful rather than stacked decoratively?

### Interaction states

- Are applicable states distinct and truthful?
- Is focus distinct from selection?
- Is disabled distinct from low priority?
- Is pending/accepted/completed feedback truthful?
- Are pointer-only cues duplicated for supported non-pointer navigation where required?

### Navigation and focus

- Is focus always visible?
- Is movement predictable?
- Can player leave every navigable element/mode?
- Are back/cancel/confirm conventions consistent?
- Is context/focus preserved/restored after overlays?

### Motion

- Does each motion have a functional/experiential purpose?
- Does anything distract from reading/task completion?
- Is critical meaning available outside animation?
- Is reduced-motion behavior respected where implemented?

### Accessibility

Apply `UI_UX_RULES.md`: text/readability, contrast, redundant channels beyond color, navigation, focus, UI context, error/destructive actions, time limits/transient UI, and motion/distraction.

A visual review does not substitute for eventual screen narration/remapping/other accessibility testing when those systems exist.

### Consistency/design-system integrity

- Did implementation reuse approved tokens/patterns?
- Were durable new values intentionally promoted to `DESIGN_SYSTEM.md`?
- Did task introduce a parallel semantic style for an existing control/state?
- Are surface exceptions documented and justified?

### Anti-slop / specificity

- Did a template macrostructure appear by reflex?
- Are effects/decorations doing real work?
- Is there unnecessary card/pill/container proliferation?
- Did implementation invent world/product data to make composition attractive?
- Is the signature element meaningful rather than novelty?

## 5. Bounded review procedure

A normal player-visible task gets at most **two meaningful visual review rounds** unless the user explicitly opens a separate design-polish task.

### Round 1 — inspection

1. Build/test the complete bounded implementation first.
2. Exercise real interaction through the canonical runner/input path.
3. Capture representative states in one batch.
4. Inspect all captures before editing again.
5. Produce one consolidated defect list with severity and review dimension.
6. Fix all in-scope severity 4/3 issues and coherent severity 2 issues in one batch.

### Round 2 — confirmation

1. Rebuild/re-run the relevant path.
2. Recapture affected representative states.
3. Confirm the fix and check for regressions caused by it.
4. If a severity 4/3 defect remains after two meaningful attempts, stop with diagnostics under the repository two-attempt rule.
5. Do not begin a third polish loop simply because minor aesthetic alternatives remain.

## 6. Review record format

A concise task review can use:

```text
Surface: <name>
Evidence: <artifact paths / scenario>
Design system: unchanged | updated

S3 Focus visibility — focused control blends into selected state.
  Impact: keyboard/controller navigation can lose position.
  Fix: separate focus boundary from selection fill.

S2 Hierarchy — secondary history competes with current decision value.
  Impact: slows primary decision.
  Fix: demote history through weight/value/group spacing.
```

Do not create permanent review documents for every task unless long-lived rationale is genuinely needed; task report/commit plus screenshots are normally enough.

## 7. Completion gate

A changed player-visible surface cannot be called verified if the real executable was not rendered/run when it could be, representative changed states were not visually inspected, an in-scope severity 4/3 defect remains, focus/navigation relevant to the task was not exercised, authoritative gameplay outcome was inferred from visuals without semantic/debug proof where ambiguity exists, durable design-system decisions were introduced but not persisted, or visual QA leaked/hung processes or required broad system cleanup.

Compilation alone is never visual verification.

## 8. Current-stage note

Until Milestone 0 creates the real game executable and canonical gameplay/UI playtest runner:

- `python tools/dev.py graphics-check` verifies only native platform/window/input/capture foundation;
- `final.png` from `platform_graphics_smoke` is **not** evidence that a future game UI meets this rubric;
- do not invent a second design-only screenshot harness that bypasses the eventual real game executable.
