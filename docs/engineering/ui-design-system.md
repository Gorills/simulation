# UI design system

This document is the implementation guide for the Godot UI design system. The architectural decision is [`../decisions/0003-project-wide-ui-design-system.md`](../decisions/0003-project-wide-ui-design-system.md).

## Source of truth

The visual source of truth is:

```text
godot/ui/design_system/world_theme.tres
```

It is configured as the project-wide Theme in `godot/project.godot` through `gui/theme/custom`.

The visual reference scene is:

```text
godot/ui/design_system/design_system_catalog.tscn
```

Use the catalog to inspect the current hierarchy and interaction states. It is not a parallel UI framework and feature screens do not inherit from it.

## Change the smallest owner

Choose the owner by intent:

- change a project color, type size, spacing scale, panel style, button state or focus treatment -> `world_theme.tres`;
- change a one-screen composition -> that screen's `Container` tree;
- add a new visual role shared by multiple screens -> a semantic Theme variation;
- add a repeated multi-node control with behavior -> a component scene under `godot/ui/design_system/components/`;
- change systemic inventory/journal/skill/game rules -> **not** the design system;
- change gameplay input gating for a modal -> `PlayerControls`, not individual buttons.

A request such as “make all primary actions warmer” should not require editing inventory, settings and pause-menu scenes.

## Styling rule

Feature UI should normally contain:

```text
theme_type_variation = &"DsPrimaryButton"
```

not:

```text
theme_override_colors/font_color = ...
theme_override_styles/normal = ...
```

and not GDScript such as:

```gdscript
button.add_theme_color_override(...)
```

Local overrides remain available for a genuinely dynamic or isolated case, but they are an exception that should have a reason. If the same override appears twice, the design system probably lacks a semantic variation.

Do not create a second palette or spacing source in a GDScript singleton, JSON file or screen-local Resource.

## Runtime UI quality rule

Using project Theme variations is **necessary but not sufficient** for a runtime interface to satisfy the design system.

Any interface rendered inside the running game — including debug HUDs, developer overlays, test screens and internal tools — must also follow the project rules for information hierarchy, spacing, alignment, density, legibility, container-driven layout and responsive behavior. Calling an interface “debug” or “temporary” does not exempt it when it is part of the game viewport.

External engineering surfaces such as stdout/stderr, the Godot debugger, profiler output, CI logs and standalone artifact files are not in-game UI and do not need to imitate the game visual language.

Do not use a persistent runtime HUD as a formatted log sink. Prefer semantic fields and grouped read-only state; keep raw JSON/serialized evidence in debug artifacts unless inspecting the raw payload itself is the explicit UI task.

A runtime UI change is not complete from scene-tree or token review alone. Inspect the rendered result in the pinned Godot runtime and review hierarchy, density, alignment, readability and clipping/reflow at the logical baseline plus representative narrower/wider aspect ratios.

## Theme vocabulary

### Surfaces

- `DsCanvasPanel`: root opaque UI canvas when a screen intentionally covers gameplay.
- `DsSurfacePanel`: default major content surface.
- `DsRaisedPanel`: stronger visual elevation for the current/high-priority region.
- `DsInsetPanel`: secondary content nested within a surface.
- `DsHudPanel`: translucent gameplay HUD surface.

Avoid nesting a framed surface for every semantic group. Inside an existing surface, prefer spacing, headings and `HSeparator` unless another elevation level communicates something useful.

### Buttons

- `DsPrimaryButton`: one high-priority commit/continue action in a local action group.
- `DsSecondaryButton`: normal action.
- `DsGhostButton`: low-emphasis action where another filled surface is unnecessary.
- `DsDangerButton`: destructive/high-risk action.

All use the base Button focus style. Do not remove the focus StyleBox to make a screenshot cleaner.

### Typography

- `DsDisplayLabel`: sparse hero/display heading.
- `DsScreenTitleLabel`: screen title.
- `DsSectionTitleLabel`: panel/section heading.
- `DsBodyLabel`: normal explanatory/content text.
- `DsMutedLabel`: lower-priority metadata.
- `DsCaptionLabel`: compact supporting text.
- `DsAccentLabel`: selected/progress/eyebrow accent.
- `DsInfoLabel`: informational secondary accent.
- `DsDangerLabel`: error/danger/health emphasis.

At the 1920×1080 logical baseline, body is 20 px and the smallest committed text role is 18 px. If a feature needs text below this, first ask whether the information should be text at all or whether the layout is over-dense.

The project font family is intentionally not fixed yet. When selected, configure it centrally in the Theme rather than on every Label.

### Layout

- `DsScreenMargin`: normal 48 px logical screen margin.
- `DsCompactScreenMargin`: 32 px logical margin for intentionally denser layouts.
- `DsStack`: 16 px vertical rhythm.
- `DsStackLarge`: 24 px vertical rhythm.
- `DsRow`: 16 px horizontal rhythm.
- `DsGrid`: 16 px grid gaps.

Screen-specific minimum widths/heights are allowed when they express content constraints. They are not a replacement for containers.

### Status/progress

- `DsProgressHealth`: danger/health red.
- `DsProgressTeal`: informational/resource state.
- `DsProgressGold`: progression/selected objective.

Do not communicate a critical state by color alone. Keep the label/value/icon/state text meaningful without the hue.

## Design tokens for custom-drawn controls

Built-in Control nodes should prefer Theme variations. A custom-drawn Control may query semantic values from the `DesignTokens` theme type:

```gdscript
var accent := get_theme_color(&"accent_gold", &"DesignTokens")
var spacing := get_theme_constant(&"space_md", &"DesignTokens")
```

Available initial token families are:

```text
colors/canvas
colors/surface
colors/surface_raised
colors/text_primary
colors/text_secondary
colors/text_muted
colors/accent_gold
colors/accent_teal
colors/status_danger
colors/status_success

constants/space_xs
constants/space_sm
constants/space_md
constants/space_lg
constants/space_xl
constants/screen_margin
```

Keep token names semantic. Do not add `blue_3` or `inventory_border_color` unless the meaning really is that narrow.

## Resolution contract

The logical desktop baseline is 1920×1080. Project settings use:

```text
window/stretch/mode   = canvas_items
window/stretch/aspect = expand
```

Build screen structure with `Container`s and anchors. `expand` can expose more logical width/height at a different aspect ratio, so a robust screen must decide where expandable space goes through size flags/stretch ratios rather than assuming the viewport is always exactly 16:9.

For a normal RPG screen:

```text
full-rect root Control/PanelContainer
  -> DsScreenMargin
       -> VBox/HBox/Grid/etc.
```

Use `ScrollContainer` when content can legitimately exceed the available region. Do not shrink text below the design-system minimum just to avoid scrolling.

Do not branch layout by a list of resolutions. If a future console/handheld mode needs a genuinely different information architecture, make that a deliberate product capability.

A future UI-scale setting should adjust the presentation at a centralized root/window scale and be tested for reflow. Do not implement scaling by multiplying values in every scene.

## Focus and controller navigation

Every interactive screen must remain understandable without a mouse.

- Give the first meaningful interactive control focus when the screen opens when no existing focus should be restored.
- Keep the project focus StyleBox visible.
- Use normal Godot focus neighbors only when automatic geometric navigation is ambiguous.
- Modal UI must keep focus inside the modal while it owns interaction.
- Do not reuse built-in Godot `ui_*` actions as gameplay actions.

The design-system catalog calls `grab_focus()` on its primary action to demonstrate the expected opening state.

## HUD rule

Passive HUD should normally use `mouse_filter = MOUSE_FILTER_IGNORE` so presentation does not steal gameplay pointer events. Interactive HUD regions opt into mouse handling deliberately.

The current runtime debug HUD is intentionally implemented through `DsScreenMargin`, `DsHudPanel`, `DsStack` and semantic labels. It exists as a production example for future UI work, not as a special hardcoded exception.

## Visual direction

The current baseline is “cinematic dark fantasy with restrained modern chrome”:

- near-black blue/green canvas;
- two or three meaningful surface elevations;
- cool structural lines;
- warm gold for focus/selection/commitment;
- teal for informational/secondary state;
- red for danger/health;
- high-contrast neutral typography;
- modest 6–8 px corner radii;
- visible focus outlines;
- atmosphere/art may be rich while functional UI remains quiet.

The supplied mockups are mood references. Their most useful qualities are atmosphere, sparse accent color and clear large-scale composition. Their weakest tendency is excessive nested framing; do not reproduce that mechanically.

## Reference workflow for a new screen

1. Start from full-rect `Control`/`PanelContainer` and a screen-margin variation.
2. Build hierarchy with Containers.
3. Apply existing semantic Theme variations.
4. Add a new variation only if an existing semantic role cannot express the design.
5. Add a component scene only after a multi-node behavior/pattern is actually reused.
6. Establish keyboard/controller focus.
7. Test at 1920×1080 and at least one wider and one narrower aspect ratio before calling layout complete.
8. Review the screen with the global Theme changed, to ensure feature code did not silently depend on literal visual values.
9. Capture/review a rendered screenshot or equivalent live frame for hierarchy, density, alignment and readability; scene-tree compliance alone is not visual acceptance.

## Anti-patterns

- copied StyleBoxes in feature `.tscn` files;
- literal UI colors in feature scripts/scenes;
- screen-local font-size overrides for normal hierarchy;
- `UiColors.gd` or `UiMetrics.gd` duplicating Theme data;
- a custom Button subclass merely to paint a button;
- separate keyboard and gamepad menu implementations;
- manual offsets for dense resizable screen layout;
- hiding focus because mouse hover looks cleaner;
- framing every nested group;
- shrinking text to fit instead of fixing information architecture;
- dumping raw serialized/log output into a persistent runtime HUD when semantic presentation is sufficient;
- changing systemic gameplay state inside reusable visual components.
