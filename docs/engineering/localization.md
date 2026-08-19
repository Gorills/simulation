# Localization

This document owns the Godot presentation localization contract.

## Supported locales

The initial supported locales are:

- `ru` — default and fallback locale;
- `en` — second fully supported locale.

A fresh run without an explicit saved/runtime choice starts in Russian. The operating-system locale does not override this project rule. If product requirements later add an `automatic` language mode, add it deliberately rather than silently changing startup behavior.

## Ownership

Localization is a presentation concern under `godot/`.

```text
Simulation / protocol -> semantic data, identifiers, quantities, events
Godot presentation    -> localized player-facing wording
```

Simulation Core and protocol must not emit locale-specific gameplay text. Domain/protocol values remain language-independent so NPC logic, determinism, persistence and external clients do not depend on a UI language.

The runtime locale owner is:

```text
godot/scripts/localization/localization.gd
```

Feature screens should use `Localization.set_locale(...)` when a future settings UI changes language. Do not scatter direct locale policy or `if locale == ...` branches across screens.

## Catalog source of truth

Runtime UI translations live in gettext catalogs:

```text
godot/localization/ru.po
godot/localization/en.po
```

Use stable semantic identifiers as `msgid`, for example:

```text
UI_INVENTORY_TITLE
UI_TRADE_BUY
UI_DEBUG_REVISION
```

Do not use English or Russian prose as the durable key. Wording may change without forcing every scene/script reference to change.

Gettext is used from the start because Russian requires real plural forms and future RPG UI will need pluralization/context. Use Godot `tr_n()` for quantities and translation context where the same wording has different meanings. Do not implement Russian declension with screen-local conditionals.

## What must be localized

Localize text rendered as part of the running game, including:

- menus, HUDs, dialogs, tooltips and settings;
- tutorial/control hints;
- player-visible item/actor/action/status wording;
- runtime debug/developer UI shown inside the game viewport.

Do not localize machine contracts merely for appearance:

- protocol/domain identifiers;
- JSON field names and structured smoke evidence;
- stdout/stderr and CI diagnostics;
- internal error codes;
- file names or stable scenario IDs.

A debug HUD may display localized labels while its `debug.json` artifact remains machine-oriented.

## Static and dynamic text

For static `Label`/`Button` text, store a semantic key in the scene and rely on Godot Control auto-translation:

```text
text = "UI_INVENTORY_TITLE"
```

For strings assigned from GDScript, translate the semantic key explicitly:

```gdscript
status_label.text = tr(&"UI_STATUS_READY")
```

Values such as entity IDs, simulation ticks and metric units are data, not translation keys. A semantic display value such as an input-device name is localized.

## Runtime selection

`Localization` owns:

- `DEFAULT_LOCALE = "ru"`;
- the supported locale list;
- locale normalization/validation;
- runtime switching through `set_locale()`;
- the `locale_changed` signal for dynamic text that needs an immediate refresh.

The repository development/playtest entry points accept `--locale ru|en`. This is a deterministic verification override, not a saved player preference system.

Persisted language preferences and a settings-screen selector are a separate player-settings capability. When added, missing/corrupt preferences fall back to `ru`.

## Layout and fonts

Localization is part of UI layout correctness. Do not tune a screen only for one language or solve longer translations by shrinking normal text below the design-system contract.

Use Containers, wrapping and scrolling where appropriate. A font used by the project must cover every glyph required by all supported locales. Russian Cyrillic coverage is therefore a release requirement; when a project font family is selected, configure/fallback it centrally in the Theme rather than per screen.

## Verification

Every localization-affecting change must at minimum prove:

1. `python tools/check_localization.py` passes;
2. the affected UI renders in `ru` and `en` under the pinned Godot runtime;
3. no missing glyphs, raw `UI_*` keys, clipping or destructive reflow are visible;
4. player-visible dynamic strings change with the locale, not only static scene labels.

For the current smoke scene:

```bash
python tools/dev.py play --scenario smoke --locale ru
python tools/dev.py play --scenario smoke --locale en
```

The smoke artifact records the active locale and translated probe strings while retaining language-independent simulation/protocol evidence.

For material UI changes, also use Godot pseudolocalization to exercise expansion/missing-key pressure. The project keeps pseudolocalization disabled for ordinary runs; it is an engineering verification mode, not a player locale.

## Catalog integrity

`tools/check_localization.py` enforces that:

- `ru` and `en` contain the same semantic key set;
- translations are non-empty;
- runtime `UI_*` references in Godot scenes/scripts resolve to the catalogs;
- stale catalog keys are surfaced instead of silently accumulating.

The checker is part of `tools/dev.py check`, so normal native/sanitize CI also protects catalog integrity even though rendering still requires Godot verification.

## Anti-patterns

- player-facing prose hardcoded in feature GDScript;
- English prose used as the localization key;
- `if locale == "ru"` wording branches in screens;
- localized strings emitted by Simulation Core/protocol;
- one locale added without the other supported locale;
- a fixed-width layout accepted only in English;
- transliterating missing Cyrillic glyphs instead of fixing font coverage;
- treating fallback locale as equivalent to the project's explicit startup-language policy.
