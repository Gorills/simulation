# ADR 0003: Project-wide Godot UI design system

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../engineering/ui-design-system.md`](../engineering/ui-design-system.md) · [`../engineering/SOURCES.md`](../engineering/SOURCES.md)

## Context

The game needs dense RPG interfaces: HUD, inventory, character, journal, skills, maps, dialogue, settings and modal flows. Those screens must share a visual language without turning every scene into a collection of copied colors, StyleBoxes, font sizes and one-off buttons.

The supplied visual references establish a useful mood rather than a frozen skin: dark obsidian/blue-black surfaces, restrained cold borders, warm gold selection, teal informational accents and cinematic world art. They also expose a risk worth correcting early: putting a framed panel around every group flattens hierarchy and makes a dense RPG screen feel older and busier than it needs to.

The architecture must make both operations cheap:

1. evolve one interaction/component without restyling the game by hand;
2. replace the visual language later without rewriting feature screens.

## Decision

### One project-wide Theme is the visual source of truth

`godot/ui/design_system/world_theme.tres` is the project-wide Godot `Theme`, configured through `gui/theme/custom`.

Static visual styling belongs there:

- palette and semantic colors;
- typography sizes;
- spacing constants;
- panel/surface StyleBoxes;
- button states and focus treatment;
- progress/status treatments;
- reusable layout spacing/margins;
- semantic theme type variations.

Feature screens consume these definitions with `theme_type_variation`. They do not recreate equivalent `theme_override_*` values, local StyleBoxes or literal colors.

Custom-drawn controls that genuinely need a token query use the `DesignTokens` theme type, for example:

```gdscript
var accent := get_theme_color(&"accent_gold", &"DesignTokens")
```

Do not introduce a second palette Resource, constants singleton or UI-colors script beside the Theme. A replacement design system should have one obvious asset to start from.

### Semantic variations are the component vocabulary

The first vocabulary uses semantic names rather than screen names:

```text
surfaces: DsCanvasPanel / DsSurfacePanel / DsRaisedPanel / DsInsetPanel / DsHudPanel
buttons:  DsPrimaryButton / DsSecondaryButton / DsGhostButton / DsDangerButton
text:     DsDisplayLabel / DsScreenTitleLabel / DsSectionTitleLabel /
          DsBodyLabel / DsMutedLabel / DsCaptionLabel /
          DsAccentLabel / DsInfoLabel / DsDangerLabel
layout:   DsScreenMargin / DsCompactScreenMargin / DsStack /
          DsStackLarge / DsRow / DsGrid
status:   DsProgressGold / DsProgressTeal / DsProgressHealth
```

A feature may compose these primitives freely. If a multi-node pattern repeats and has real behavior of its own, promote it into a reusable scene/script under `godot/ui/design_system/components/` at that time. Do not prebuild a generic component framework before repetition exists.

A screen-specific concept such as `InventoryWeaponCard` may exist when it owns inventory presentation behavior. Its visual states must still consume design-system variations/tokens rather than inventing another skin.

### Layout is container-driven

RPG screens use `Container` composition, size flags, anchors and semantic margins. Manual offsets are not the layout system for resizable screens.

The logical desktop baseline is **1920×1080**. The project uses:

```text
window/stretch/mode   = canvas_items
window/stretch/aspect = expand
```

This keeps UI authored against a stable logical baseline while allowing the canvas to adapt to different aspect ratios. Screens must use containers so newly exposed horizontal or vertical space changes composition rather than revealing hardcoded coordinates.

Do not add `if resolution == ...` layout branches for ordinary desktop resolutions. A genuinely different form factor may earn a deliberate layout variant later.

Future user-controlled UI scale belongs at the root/window content scale or a similarly centralized presentation setting. It must not multiply font sizes independently in every feature scene.

### Typography is centralized but the font family is not frozen yet

The initial Theme deliberately uses Godot's fallback font instead of committing an unverified/licensed decorative font just to imitate a reference image.

At the 1080p baseline:

- body text starts at 20 px;
- small/caption text does not go below 18 px;
- section, screen and display hierarchy use larger semantic sizes.

When the final font family is selected, it is replaced centrally in the Theme. Accessibility/user settings may later expose a readable sans-serif alternative without changing screen structure.

### Interaction state is part of the design system

Keyboard/controller focus is not an afterthought. Buttons have a deliberately visible focus outline distinct from hover, and UI scenes are expected to establish a valid initial focus when they open.

Gold represents selection/commitment emphasis. Teal is informational/secondary. Danger/health uses red. Status meaning must not rely on hue alone when the information is important: pair it with text, iconography, shape or another channel.

Gameplay InputMap actions remain separate from Godot's UI navigation actions. Modal UI uses the existing `PlayerControls.set_gameplay_enabled(false)` boundary rather than scattering gameplay-disable checks through controls.

### Visual direction: dark fantasy, quieter chrome

The mood direction keeps the references' cinematic dark-fantasy character while reducing panel noise:

- canvas and major surfaces establish depth;
- raised/inset surfaces are used only when hierarchy needs them;
- minor grouping prefers spacing, typography and separators over another frame;
- gold is scarce enough to mean focus/selection/commitment;
- teal supports information rather than competing as a second primary CTA;
- focus/hover/pressed/disabled states are visibly different;
- decorative art may be dramatic, but UI text and interaction state must remain legible over it.

The screenshots are mood references, not canonical assets or a promise that every future screen will preserve the same ornamentation.

## Reference implementation

The design-system catalog at `godot/ui/design_system/design_system_catalog.tscn` is the visual/component reference scene. It demonstrates hierarchy, surfaces, button states, focus and semantic status colors without local theme overrides.

The runtime debug HUD also consumes the same project Theme. This is intentional: production code, not only documentation, demonstrates the expected pattern.

## Consequences

Positive:

- replacing colors, spacing, typography or base component styling starts in one Theme;
- feature scenes communicate intent with semantic variation names;
- weak models have an obvious extension point instead of copying literal colors from a nearby screen;
- controller focus and resolution behavior are part of the initial foundation;
- dense RPG screens can evolve without a border around every nested group.

Costs:

- a project-wide Theme becomes a high-leverage asset and changes to base styles require visual review across representative screens;
- special-case visuals sometimes need a new semantic variation instead of a quick local override;
- final typography, icon language and UI-scale settings still require later product/art decisions and real playtests.

## Rejected alternatives

### Per-screen Themes

They make replacement expensive and allow inventory, journal, settings and HUD to drift into separate products.

### Local `theme_override_*` as normal styling

Overrides are useful for exceptional runtime/state-driven cases, but using them as the default styling method duplicates design decisions and hides them inside scenes.

### A global `UiColors.gd`/`UiMetrics.gd` singleton beside Theme

That creates two sources of visual truth. Godot already provides project-wide Theme items and type variations.

### Fixed 1920×1080 offsets

A 1080p baseline is useful for authoring and readability targets; fixed coordinates are not a responsive layout strategy.

### Copying the supplied references literally

They are valuable mood/composition references, but their heavy framing should not become a structural requirement. The foundation preserves the dark-fantasy identity while leaving room for a cleaner modern hierarchy.
