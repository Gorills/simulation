# Game UI Design System

**Status:** ACTIVE

This file is the single persistent design-memory source for player-visible UI. It records only durable, approved decisions. It is intentionally sparse until real player-visible surfaces exist.

Do not invent values here just to make the document look complete. `UNSET` is the correct state when no approved decision exists.

## 1. Maturity

```text
Design system maturity: UNSET / pre-UI foundation
Player-visible UI implementation: none
Approved visual direction: none
Approved UI components: none
Approved production font assets: none
```

The first real UI task should establish only the minimum decisions needed by that bounded surface, then persist choices here only if they are durable beyond that one implementation detail.

## 2. Project visual read

**Status:** UNSET

When approved, record briefly:

```text
Primary player/use scene:
Relationship between UI and game world:
Core experiential words:
Deliberately avoided default aesthetics:
Signature design idea:
Theme/light-environment rationale:
```

## 3. Design dials

Current values:

```text
DESIGN_VARIANCE: UNSET
MOTION_INTENSITY: UNSET
VISUAL_DENSITY: UNSET
```

Definitions:

- `DESIGN_VARIANCE`: regular/symmetrical/predictable (1) -> irregular/asymmetrical/experimental (10).
- `MOTION_INTENSITY`: essentially static (1) -> motion as major authored material (10).
- `VISUAL_DENSITY`: sparse/airy (1) -> dense/cockpit-like information presentation (10).

These are intent coordinates, not immutable limits. Accessibility and task constraints override them.

## 4. Color system

**Status:** UNSET

When implemented, record semantic roles rather than a pile of incidental swatches. Record canonical name, renderer value, semantic role, valid backgrounds/combinations, and relevant contrast/accessibility notes.

Possible roles should be added only when needed: canvas/world-overlay ground, surface levels, primary/secondary/muted text, focus, primary action, selection, success, warning, error/destructive, informational accent.

## 5. Typography

**Status:** UNSET

When implemented, record font asset/family plus license/provenance, semantic roles, size/scale strategy at target resolutions, weight hierarchy, line spacing/wrapping, numeric alignment if relevant, localization expansion assumptions, and accessibility scaling behavior.

Required production fonts must remain available to the required network-free build/runtime path.

## 6. Spacing and density

**Status:** UNSET

When implemented, record a small semantic spacing rhythm for intra-control, tightly related items, groups, sections/surfaces, and screen-safe margins. Density follows player task and display context; HUD, dialogue, settings, and reading-heavy surfaces do not need identical density.

## 7. Surfaces and depth

**Status:** UNSET

Record the one coherent depth model used by the project and the semantic meaning of surface levels. Do not accumulate unrelated border + shadow + glow + blur treatments without purpose.

## 8. Iconography and imagery

**Status:** UNSET

When established, record icon family/authoring rules, stroke/fill/optical-weight rules, size grid, semantic symbol use, label requirements for important/ambiguous actions, image treatment, and redundant-cue/accessibility rules.

## 9. Motion vocabulary

**Status:** UNSET

When motion exists, record named semantic roles such as focus/selection feedback, press/activation feedback, navigation transition, state-change confirmation, attention request, or authored experiential moment. Record duration/range, easing approach, interruptibility, and reduced-motion behavior.

Presentation animation must never drive authoritative simulation state.

## 10. Focus, navigation, and input language

**Status:** UNSET

When controls exist, record focus treatment, traversal model, keyboard/controller/pointer equivalence, cancel/back convention, confirm/select convention, remap/capture behavior, focus restoration after overlays, and unavailable-control behavior.

## 11. Component and pattern registry

No approved production UI components exist yet.

When a component/pattern becomes durable, add one compact entry:

```text
Name:
Purpose:
Variants:
Applicable states:
Token dependencies:
Input/focus behavior:
Accessibility notes:
Known surface usages:
```

Keep code signatures in code; keep semantic design decisions here.

## 12. Surface exceptions

No player-visible surfaces exist yet.

A deliberate deviation from the global system must record surface, mode (`Operate` / `Read` / `Experience`), override, why the global rule is insufficient, accessibility impact, and approving task/commit. Do not use exceptions to justify random one-off styling.

## 13. Retired patterns

None yet. Record only durable reasons future agents are likely to need; Git history is the full archive.

## 14. Update protocol

Update this file only when a decision is durable beyond one local implementation detail. A UI task that introduces or changes such a decision must first implement and verify the real surface, visually review it using `DESIGN_REVIEW.md`, persist the approved choice here, and run documentation hygiene checks.
