# simulation

Greenfield systemic RPG / world-simulation project.

The required development loop is native and network-free so coding agents can build, test and play it entirely inside the execution environment.

## Start here

```bash
python tools/dev.py doctor
python tools/dev.py check
python tools/play.py --scenario smoke
```

For manual play after building:

```bash
./build/native-debug/sim_cli
```

## Repository navigation

- [`AGENTS.md`](AGENTS.md) — entry point and hard guardrails for coding agents.
- [`docs/INDEX.md`](docs/INDEX.md) — canonical documentation index.
- [`docs/GAME.md`](docs/GAME.md) — what is actually playable now.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — implemented architecture.
- [`docs/engineering/DEVELOPMENT_RULES.md`](docs/engineering/DEVELOPMENT_RULES.md) — engineering and stack rules.
- [`docs/specs/PROJECT_SPEC.md`](docs/specs/PROJECT_SPEC.md) — temporary condensed project specification and roadmap.

The repository intentionally has **no CI**. Development evidence comes from explicit local build/test/playtest commands.
