# Game UI/UX Rules

**Status:** ACTIVE

This is the canonical usability, accessibility, and craft floor for future native game UI. It complements [`GAME_UI_DESIGN_SKILL.md`](GAME_UI_DESIGN_SKILL.md), persistent choices in [`DESIGN_SYSTEM.md`](DESIGN_SYSTEM.md), and acceptance review in [`DESIGN_REVIEW.md`](DESIGN_REVIEW.md).

At the current foundation stage there is no game UI implementation. These rules constrain future work; they do not imply components/features exist.

## 1. Precedence

When rules conflict, use this order:

1. player safety, truthfulness, authoritative-state integrity, recoverability;
2. accessibility and input/focus usability;
3. explicit user/task requirements;
4. approved `DESIGN_SYSTEM.md` decisions;
5. local visual preference.

Aesthetic intensity never excuses a broken focus path, misleading state, unreadable text, destructive-action trap, or inaccessible information channel.

## 2. Core usability floor

Use Nielsen-style heuristics as a review lens:

- **Visibility of system status:** expose important state changes/outcomes soon enough to understand what happened. Pending, accepted, rejected, and completed are not interchangeable.
- **Match the game world/player language:** use concepts and action verbs players understand; do not leak internal architecture names because they are convenient to print.
- **Player control and freedom:** predictable back/cancel, reversible actions where feasible, clear confirmation for genuinely risky irreversible actions, no navigation/focus traps.
- **Consistency:** the same semantic role uses consistent vocabulary/state behavior/token family unless a deliberate documented exception exists.
- **Error prevention:** prevent invalid/destructive actions where possible; explain consequence before commitment when needed.
- **Recognition over recall:** keep necessary context/options visible or locally discoverable; do not require memorizing unlabeled icons, hidden modifier rules, previous-screen values, or color-only state codes without a deliberate game reason.
- **Flexibility and efficiency:** shortcuts/accelerated navigation/dense expert views may exist without destroying a comprehensible default path.
- **Minimal relevant design:** attention should serve the current task; “minimal” means low irrelevant load, not necessarily empty/sparse.
- **Error recovery:** say what happened, what remains true, and what the player can do next.
- **Help/documentation:** prefer self-explanatory structure/contextual help, with deeper explanation when a system is genuinely complex.

Do not optimize a synthetic “10/10 UX” score. Track concrete defects and severity instead.

## 3. Information hierarchy

Each surface has one primary task/state. Establish hierarchy using coordinated position, contrast/value, type weight/size, grouping, and space. Related elements cluster more tightly than unrelated groups. Secondary history/metadata must not overpower current decision data. Empty/loading/error states preserve enough context to know where the player is.

The squint/blur test should preserve major structure once real screenshots exist.

## 4. Contrast and redundant cues

Baseline derived from the reviewed Microsoft Xbox Accessibility Guidelines:

- standard important text/visual information: target at least **4.5:1** contrast;
- large text/elements: target at least **3:1**;
- inactive/disabled text that still conveys information: target at least **3:1**.

Exact platform/certification requirements must be rechecked when target-specific UI ships; do not freeze unverified pixel-size assumptions now.

**Color is never the only channel for important meaning.** Pair important state color with shape, icon, label, pattern, position, border/state treatment, or another redundant cue as appropriate.

Measure contrast against the actual rendered/composited background, not merely token-to-token values in isolation.

## 5. Text and typography accessibility

- Important text remains readable at target resolution/viewing conditions.
- The eventual UI system must provide a strategy for configurable/scalable text; do not hard-wire a single tiny pixel size as the accessibility model.
- Layout must survive longer localization-like strings, player names, quantities, dates/numbers, wrapping, and explicit accessibility scaling when relevant.
- Critical state must not be communicated solely through font style/case.
- Uppercase/condensed/decorative/handwritten/pixel/serif/monospace are design choices, never automatic readability wins.
- Required font assets need license/provenance and local availability in the network-free required path.

## 6. Focus and navigation

When controls exist:

- focus is always visually discernible;
- focus differs from selected/toggled state;
- traversal follows meaningful spatial/task order;
- every navigable control/mode has an exit;
- back/cancel/confirm conventions remain stable;
- entering/leaving overlays restores meaningful context/focus;
- unavailable controls do not silently swallow navigation;
- keyboard/controller/pointer behavior is equivalent for supported tasks unless a deliberate device-specific interaction is documented;
- required information cannot be hover-only.

Do not implement navigation as incidental pixel-coordinate behavior that becomes impossible to reason about when layout changes.

## 7. UI context

The player should understand the current screen/surface, which entity/object/context current controls affect, what primary action will do, what changed after the action, and how to return/cancel when applicable. Modal/overlay presentation must not silently discard context.

## 8. Interaction states

Applicable states must be explicit rather than accidental styling side effects:

| State | Required distinction |
|---|---|
| default | stable baseline |
| hover/pointer-over | optional pointer cue; never sole required cue |
| focused | keyboard/controller navigation position |
| pressed/active | immediate activation feedback |
| selected/toggled | persistent choice/state distinct from focus |
| disabled/unavailable | clearly unavailable, not merely low priority |
| busy/pending | work/state transition in progress |
| error/invalid | problem plus recovery/context |
| success/confirmed | truthful completion feedback |
| empty | no content plus useful next action where available |
| destructive | consequence clear before irreversible commitment |

Do not pretend to support states a component cannot semantically enter.

## 9. Timing and transient UI

Players need adequate time to perceive/read/act. When transient UI carries important information, duration must fit likely reading/action time or be configurable/persistent where warranted; important lost information needs another discoverable channel/history when loss would matter; UI timeouts must not accidentally become gameplay reflex tests; transitions must not unnecessarily block input.

## 10. Motion and visual distraction

Motion supports state, causality, spatial relation, feedback, or a deliberate authored experience. Avoid continuous decorative movement in task-heavy surfaces. When motion is material: preserve critical meaning without animation, provide appropriate reduced-motion behavior, avoid rapid/repetitive flashes that create risk, prefer a predictable shared motion vocabulary, and do not let presentation timing drive authoritative simulation/world time.

## 11. Copy, truth, and recovery

Player-facing copy names the actual action/outcome, uses consistent verbs through a flow, avoids architecture/debug jargon, gives actionable recovery for recoverable errors, avoids manipulative urgency/fake scarcity/guilt/deceptive defaults, and never fabricates game-world facts/data merely to make a screenshot complete.

Destructive actions identify consequence; confirmations are reserved for meaningful risk, not every trivial click.

## 12. Structural craft floor

- One focal task/state per surface.
- Semantic spacing beats arbitrary per-control gaps once a spacing system exists.
- Use containers only when grouping/layering requires them; do not wrap every label/action in a card/pill.
- Use one coherent depth language rather than border + shadow + glow + blur everywhere.
- Reuse canonical tokens/components when they exist.
- Avoid repeated generic macro-layouts across unrelated tasks.
- Do not use invented data/fake charts/placeholder success as decoration.
- Important or ambiguous icon actions need labels or another clear cue when required.

## 13. Dark-pattern prohibition

Do not intentionally manipulate the player against their stated interest through hidden costs/consequences, disguised ads/actions, confirm-shaming, forced continuation without clear exit, misleading scarcity/urgency, preselected harmful/destructive choices, or visual hierarchy designed to hide cancellation/privacy/accessibility choices.

Game fiction may deceive a character as authored narrative; the **interface itself** must not deceive the player about controls/system consequences unless deception is explicitly the bounded mechanic and remains safely understandable.

## 14. Platform honesty

Current full runtime verification is Linux/X11. Windows/macOS backends exist upstream but are not verified in this environment. Likewise these accessibility rules are a design baseline, not a claim of Xbox certification/compliance testing.

When a target platform, certification program, screen-reader path, remapping system, high-contrast mode, subtitle system, or other accessibility feature becomes real scope, recheck current first-party requirements and add concrete tests rather than relying on this summary alone.
