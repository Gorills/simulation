# Game UI Design Sources

**Status:** ACTIVE

This file records the external material reviewed while creating the repository-owned UI/UX design policy. It is provenance, not a second rulebook.

The canonical project rules are:

- [`GAME_UI_DESIGN_SKILL.md`](GAME_UI_DESIGN_SKILL.md) — design workflow;
- [`DESIGN_SYSTEM.md`](DESIGN_SYSTEM.md) — durable approved design memory;
- [`UI_UX_RULES.md`](UI_UX_RULES.md) — usability/accessibility/craft floor;
- [`DESIGN_REVIEW.md`](DESIGN_REVIEW.md) — bounded visual review and acceptance.

Do not install these external skills during ordinary project work. Revisit upstream only in a separate bounded design-foundation task or when a concrete requirement exposes a gap. Upstream licenses and terms remain authoritative for upstream material; this repository does not vendor the reviewed skill files.

## Discovery source

The initial comparison list came from:

- Pasquale Pillitteri, “20 лучших скиллов Claude Code для UI/UX дизайна: полное руководство”, published 2026-04-15: https://pasqualepillitteri.it/ru/news/888/claude-code-18-luchshikh-skill-dlya-ui-ux-dizayna

The article was used as a discovery index only. Rules were checked against the primary repositories or first-party guidance before being adopted.

## Reviewed design-skill sources

### Anthropic Frontend Design

- Repository: https://github.com/anthropics/claude-code
- Reviewed revision: `354757e5b2d9aa1ebb62e5d05ecd384f0e11c0f7`
- Source path: `plugins/frontend-design/skills/frontend-design/SKILL.md`
- Retained ideas: start from subject/audience/task; intentional distinctive direction; typography and structure as design material; one justified signature/risk; motion with purpose; bounded screenshot critique; avoid statistical/default AI aesthetics.
- Adaptation: web/hero/CSS-specific instructions were not adopted as native-game rules.

### Interface Design

- Repository: https://github.com/Dammyjay93/interface-design
- Reviewed revision: `2f9be3206855bcb2d1d0af262c8bae25cba6658d`
- Source path: `.claude/skills/interface-design/SKILL.md`
- Retained ideas: persistent design memory across sessions; explicit intent before code; domain exploration; focal hierarchy; systematic tokens; spacing/depth/typography consistency; reuse established patterns rather than inventing a parallel system.
- Adaptation: HTML/component-library/browser-specific guidance was excluded.

### Impeccable

- Repository: https://github.com/pbakaus/impeccable
- Reviewed revision: `f88b2837a7d7c3182e46307bbbb091a1ed547571`
- Primary reviewed paths: `skill/SKILL.src.md`, `skill/reference/craft-floor.md`
- Retained ideas: a hard craft floor separated from visual direction; explicit critique/audit/polish modes; honest copy/data; state coverage; anti-slop review; bounded inspection/fix/confirmation rather than endless visual loops.
- Adaptation: browser tooling, React/CSS implementation details, installation hooks, and web-responsive rules were excluded.

### UX Heuristics

- Repository: https://github.com/wondelai/skills
- Reviewed revision: `6bac1534f9f256a56fc2b4dd0e70b9a692758966`
- Source path: `ux-heuristics/SKILL.md`
- Retained ideas: Nielsen-style heuristic review, recognition over recall, system-status visibility, control/freedom, error prevention/recovery, severity-based findings, dark-pattern rejection.
- Adaptation: website-specific navigation/form/browser examples were generalized to native game UI.

### Taste Skill

- Repository: https://github.com/Leonxlnx/taste-skill
- Reviewed revision: `dfb6f9f9e93a39f673b1827c0889cc28326d1800`
- Source path reviewed from the `skills/taste-skill` family.
- Retained idea: three explicit design-intent axes — `DESIGN_VARIANCE`, `MOTION_INTENSITY`, `VISUAL_DENSITY` — as durable coordinates rather than session-specific taste.
- Adaptation: preset values, React/Tailwind stack choices, font lists, web breakpoints, and aesthetic catalogs were not adopted.

### Hallmark

- Repository: https://github.com/Nutlope/hallmark
- Reviewed revision: `13ac0ec7e148655948100b6396439e481361d690`
- Source path: `skills/hallmark/SKILL.md`
- Retained ideas: structural variety matters as much as palette variation; inspect established project visual truth before redesigning; no fabricated metrics/content; locked tokens over mid-render improvisation; anti-template self-review.
- Adaptation: page/hero/footer/theme-catalog/mobile-web rules were excluded.

## Primary usability and accessibility sources

### Microsoft Xbox Accessibility Guidelines

Reviewed as first-party game-accessibility guidance:

- Guidelines index: https://learn.microsoft.com/en-us/xbox/accessibility/guidelines
- XAG 101 — Text display: https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/101
- XAG 102 — Contrast: https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/102

At review time Microsoft’s guidance included the contrast baselines summarized in `UI_UX_RULES.md`: 4.5:1 for standard important text/visual elements, 3:1 for large-scale elements, and 3:1 for inactive-element text. Microsoft’s broader XAG set also explicitly separates UI navigation, focus handling, UI context, errors/destructive actions, time limits, visual distraction/motion, input, narration, and other accessibility concerns.

These are a design baseline, not a claim that this project is Xbox-certified or has implemented every XAG. Recheck current target-platform requirements when those systems become real scope.

### Nielsen Norman Group

- “10 Usability Heuristics for User Interface Design”: https://www.nngroup.com/articles/ten-usability-heuristics/

Used as the conceptual basis for heuristic categories such as visibility of system status, match with the real world, control/freedom, consistency, error prevention/recovery, recognition over recall, efficiency, and minimalist/relevant presentation.

## What was deliberately not transferred

The repository policy intentionally does **not** import:

- React, Next.js, Tailwind, DOM, CSS, responsive-web, browser-history, or web-component implementation rules;
- SaaS/landing-page/hero/footer macrostructures;
- automatic font, palette, style-catalog, glassmorphism, brutalism, “dark tech”, or other aesthetic presets;
- package-install commands, Claude/Codex plugin layouts, hooks, or agent-specific skill directories;
- claims that a third-party design system/library is present when it is not;
- fabricated sample metrics/content intended only to make screenshots attractive;
- any rule that conflicts with authoritative simulation state, accessibility, player control/recoverability, the explicit user brief, or the native/network-free stack.

## Update policy

Do not continuously chase upstream changes. Update this provenance and the repo-owned policy only when:

1. a bounded foundation task explicitly reviews newer upstream material;
2. a real UI requirement exposes a missing rule;
3. a cited first-party accessibility/platform requirement materially changes;
4. an adopted rule is found to be wrong or unsuitable for this project.

When updating, record the exact reviewed revision/date and modify the canonical owner document rather than pasting a second copy of the same rule here.