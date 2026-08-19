# Agent bootstrap

Use this file as the always-on project context. Keep it short. Load deeper documentation only for the current task.

## Before changing anything

1. Inspect the current source, tests, build files, lock files, and git diff that are relevant to the request.
2. Use [`docs/INDEX.md`](docs/INDEX.md) to select the canonical document for the concern.
3. Read only the relevant stack guide / model / ADR after that.
4. Never invent an API, target, path, installed tool, dependency version, or successful verification result.

Current executable behavior wins over prose. Accepted ADRs record intentional architectural choices. If documentation disagrees with current executable behavior, treat the documentation as stale and correct it in the same bounded change when relevant.

## Project invariants

- C++23 Simulation Core is the only authoritative world.
- Godot 4 is presentation, input, audio, and UI; it is not a second simulation.
- `src/sim` and `src/protocol` remain Godot-free.
- The GDExtension adapter is the only Godot ↔ protocol runtime seam.
- Gameplay work is a small playable vertical capability: rule → contract → experience → proof.
- Verification is local-only. Do not add project CI services, committed CI workflow files, or CI configuration; use the repository bootstrap, CMake/CTest presets, and bounded Godot playtests on the developer machine.
- Do not restore the deleted Fenster / TypeScript / WASM / Playwright architecture.
- Do not add speculative frameworks, abstractions, or subsystems without a current demonstrated need.
- Do not claim verification that was not actually run.

## AI Layer boundary

`Gorills/ai-layer` is the external development control plane.

- Keep this repository zero-footprint for AI Layer durable state: no `.ai-layer/`, copied Work/Task/Epic state, project registry, or AI Layer database.
- Do not duplicate AI Layer continuation, Work/Task/Epic lifecycle, or another STOP/`продолжай` protocol here.
- Do not make `ai-layer` a runtime or build dependency of the game.
- Repo files own product, architecture, modeling, engineering and verification constraints. AI Layer owns durable workflow, Project Map, Knowledge, Decisions storage and project skills outside the repository.

## Context routing

- Product/gameplay intent: [`docs/PRODUCT.md`](docs/PRODUCT.md)
- Runtime boundaries: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- Simulation/modeling policy: [`docs/MODELING.md`](docs/MODELING.md)
- Tests/playtest/evidence: [`docs/VERIFICATION.md`](docs/VERIFICATION.md)
- Milestones/current build direction: [`docs/ROADMAP.md`](docs/ROADMAP.md)
- Stack how/how-not: [`docs/engineering/STACK.md`](docs/engineering/STACK.md)
- Godot/GDExtension version policy: [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md)
- Consequential decisions: [`docs/decisions/`](docs/decisions/)
- Full documentation router: [`docs/INDEX.md`](docs/INDEX.md)

Host-specific rules and skills must point to these canonical sources rather than copy them.
