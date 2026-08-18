# Repository Agent Guide

This file is the entry point for coding agents. Keep it short. The canonical repository knowledge lives under `docs/`.

## Read first

Before changing anything:

1. Read [`docs/INDEX.md`](docs/INDEX.md).
2. Read only the canonical documents relevant to the current task.
3. Inspect the existing code, tests, schemas, and commands before assuming an API or contract exists.

For implementation work, [`docs/engineering/DEVELOPMENT_RULES.md`](docs/engineering/DEVELOPMENT_RULES.md) is mandatory.
For product direction and milestone constraints, consult [`docs/specs/PROJECT_SPEC.md`](docs/specs/PROJECT_SPEC.md) while it exists.
For documentation changes, follow [`docs/DOCUMENTATION.md`](docs/DOCUMENTATION.md).

## Hard project invariants

- `main` must remain runnable and playable once the playable path exists.
- The authoritative world lives in the C++23 Simulation Core.
- Presentation/input code is non-authoritative; it must never become a second source of gameplay truth.
- The required development loop must run without network access in the agent environment. Browser/WASM tooling is optional until explicitly reintroduced after local viability is proven.
- Gameplay work is vertical: rule -> protocol -> player experience -> proof.
- Prefer the smallest real causal model over a fake placeholder or speculative framework.
- Do not add abstractions, subsystems, dependencies, or refactors for hypothetical future use.
- Do not invent existing APIs, file formats, commands, events, or schemas. Read them first.
- Do not perform unrelated cleanup in a bounded task.
- Do not add GitHub Actions, CI pipelines, CI configuration, or CI-specific workflow unless the user explicitly requests CI in a later task.

## Bounded workflow

One development pass performs one bounded task:

1. Define `IN SCOPE` and `OUT OF SCOPE` internally.
2. Inspect only relevant code/tests/docs.
3. Make the minimal coherent change.
4. Self-review the diff.
5. Run the smallest sufficient local verification.
6. If gameplay changed, run exactly one bounded playtest through the canonical playtest entry point.
7. Commit/push only when permitted.
8. Report `VERIFIED / NOT VERIFIED / ASSUMPTIONS / BLOCKERS` and stop.

Do not start the next task until the user says `продолжай`.
After `продолжай`, audit the previous task first. If that audit finds a blocker, fix only the previous task and stop again.

If the same problem survives two meaningful attempts, stop with diagnostics instead of random retries.

## Git

- Trunk-first by default.
- One coherent bounded task should produce one meaningful commit where practical.
- Use a feature branch only for genuinely risky/long work or when explicitly requested.
- Commit messages describe behavior or repository capability, not process (`Refactor`, `Update files`, `Fix tests`).
- Never rewrite unrelated history or discard user changes.

## Documentation discipline

- `docs/INDEX.md` is the documentation catalog and navigation root.
- Every active canonical document must be indexed there.
- Do not duplicate the same rule in multiple canonical documents; link to the source of truth instead.
- Update documentation in the same task when a real contract changes.
- Temporary planning/spec documents must define a retirement condition and should be deleted when no longer needed; Git history is the archive.
- Do not create empty placeholder docs, speculative ADRs, or documentation for behavior that does not exist.

## Scoped AGENTS.md files

Add a nested `AGENTS.md` only when a subtree has durable rules that materially differ from the repository defaults. The nearest applicable `AGENTS.md` wins for that subtree. Do not create nested files merely to restate root rules.
