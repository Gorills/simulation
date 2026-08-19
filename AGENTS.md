# Repository Agent Guide

This file is the mandatory entry point for coding agents. Keep it compact: the detailed operating procedure lives in canonical documents under `docs/`.

## Mandatory read order

Before changing anything:

1. Read [`docs/INDEX.md`](docs/INDEX.md).
2. For any code/tooling task, read [`docs/engineering/AGENT_RUNBOOK.md`](docs/engineering/AGENT_RUNBOOK.md) and follow its exact commands instead of rediscovering the stack.
3. Read [`docs/engineering/DEVELOPMENT_RULES.md`](docs/engineering/DEVELOPMENT_RULES.md).
4. Read [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) before changing repository structure, CMake, graphics/platform code, or verification tooling.
5. For native window/input/platform work, read [`docs/engineering/PLATFORM_CAPABILITIES.md`](docs/engineering/PLATFORM_CAPABILITIES.md). The currently vendored Fenster path is a proven **diagnostic fixture**, not yet the selected production game window/input contract.
6. For any player-visible UI/design task, read the design foundation in this order before editing: [`docs/design/GAME_UI_DESIGN_SKILL.md`](docs/design/GAME_UI_DESIGN_SKILL.md) -> [`docs/design/DESIGN_SYSTEM.md`](docs/design/DESIGN_SYSTEM.md) -> [`docs/design/UI_UX_RULES.md`](docs/design/UI_UX_RULES.md) -> [`docs/design/DESIGN_REVIEW.md`](docs/design/DESIGN_REVIEW.md). Source provenance is in [`docs/design/SOURCES.md`](docs/design/SOURCES.md); do not re-install/research external skill packs during ordinary UI work.
7. Read [`docs/specs/PROJECT_SPEC.md`](docs/specs/PROJECT_SPEC.md) only when product/gameplay direction is relevant.
8. For documentation changes, follow [`docs/DOCUMENTATION.md`](docs/DOCUMENTATION.md).

Inspect the existing code/tests/commands before assuming an API or contract exists.

## Current repository stage

This repository is currently a **development foundation**, not a game implementation.

- The native graphical diagnostic stack is implemented and locally verifiable.
- The production native window/input capability contract is documented, but production platform selection is still pending. Do not build gameplay/UI directly on Fenster or OS-specific handles.
- The UI/UX design workflow, persistent design-memory contract, usability/accessibility floor, review procedure, and source provenance are documented, but there is intentionally no game UI implementation yet.
- There is intentionally no gameplay, Simulation Core, protocol, game executable, `GAME.md`, or gameplay playtest runner yet.
- `src/platform/graphics_smoke.cpp` is a stack diagnostic fixture, **not gameplay or UI design evidence**.
- Before the first gameplay/UI implementation, the production platform selection blocker in `docs/engineering/PLATFORM_CAPABILITIES.md` must be resolved by a locally proven candidate.

## Hard invariants

- The required agent development loop must run without network access.
- Required dependencies must not be downloaded during normal configure/build/test/verification.
- Third-party source used by the required path is vendored and integrity-checked.
- The currently vendored Fenster dependency is diagnostic-only until a production platform replacement passes the documented capability acceptance test.
- Future gameplay/UI code must depend on a project-owned platform contract, never directly on X11/Win32/Cocoa handles or Fenster internals.
- The planned authoritative world belongs in a future C++23 Simulation Core; presentation/platform code must never become gameplay authority.
- Future gameplay work is vertical: rule -> protocol -> graphical player experience -> proof.
- Future player-visible UI work must follow the repo-owned design foundation and persist durable approved choices only in `docs/design/DESIGN_SYSTEM.md`.
- Do not install external UI/UX skills or create agent-specific skill copies as part of ordinary UI work; reviewed guidance has been adapted into repository-owned policy.
- Do not add abstractions, dependencies, subsystems, or refactors for hypothetical future use.
- Do not invent commands, files, events, schemas, or APIs. Read them first.
- Do not perform unrelated cleanup in a bounded task.
- Do not add GitHub Actions, CI pipelines, CI configuration, or CI-specific workflow unless the user explicitly requests CI in a later task.

## Required first commands

For a fresh agent session on the Linux agent host:

```bash
python tools/dev.py doctor
python tools/dev.py verify
```

`verify` is the canonical full foundation acceptance command. Details, expected output, artifacts, and failure triage are in [`docs/engineering/AGENT_RUNBOOK.md`](docs/engineering/AGENT_RUNBOOK.md).

## Bounded workflow

One development pass performs one bounded task:

1. Audit the previous task first after the user says `продолжай`.
2. Define `IN SCOPE` / `OUT OF SCOPE` internally.
3. Inspect only relevant code/tests/docs.
4. Make the minimal coherent change.
5. Self-review the diff.
6. Run the smallest sufficient local verification from the runbook.
7. When real player-visible UI exists and changes, apply the bounded review/evidence procedure in `docs/design/DESIGN_REVIEW.md`; when player-visible gameplay exists later and changes, run exactly one canonical bounded gameplay playtest.
8. Commit/push only when permitted.
9. Report `VERIFIED / NOT VERIFIED / ASSUMPTIONS / BLOCKERS` and stop.

Do not start the next task until the user says `продолжай`.

If the same problem survives two meaningful attempts, stop with diagnostics instead of random retries.

## Git and documentation

- Trunk-first by default; one coherent bounded task should produce one meaningful commit where practical.
- Never rewrite unrelated history or discard user changes.
- `docs/INDEX.md` is the documentation catalog and navigation root.
- Every active canonical document must be indexed there.
- Do not duplicate the same rule across canonical docs; link to its source of truth.
- Temporary docs must state retirement conditions and should be deleted when obsolete; Git history is the archive.
- Add nested `AGENTS.md` only when a subtree has durable rules that materially differ from this file.