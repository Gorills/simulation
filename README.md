# simulation

Greenfield systemic RPG / world-simulation project.

**Current stage:** repository/toolchain/graphical-platform foundation only. Gameplay has not started yet.

The required development loop is native and network-free. The repository vendors the small window/framebuffer dependency needed by the graphical stack, so a normal build does not fetch packages or SDKs.

## Foundation quick start

On the verified Linux agent host:

```bash
python tools/dev.py doctor
python tools/dev.py verify
```

For a faster non-graphical edit loop:

```bash
python tools/dev.py check
```

To run only the real headless graphical stack verification:

```bash
python tools/dev.py graphics-check
```

A successful graphical check opens a real native X11 window inside Xvfb, injects a real `D` key event, verifies framebuffer state, captures the actual window and writes `final.png` under `.cache/graphics-check/<run-id>/`.

There is intentionally **no game executable and no gameplay runner yet**. The platform smoke executable exists only to prove the graphical stack before gameplay work begins.

## Repository navigation

- [`AGENTS.md`](AGENTS.md) — mandatory agent entry point and hard guardrails.
- [`docs/INDEX.md`](docs/INDEX.md) — canonical documentation index.
- [`docs/engineering/AGENT_RUNBOOK.md`](docs/engineering/AGENT_RUNBOOK.md) — exact agent setup/run/verification procedure and failure triage.
- [`docs/engineering/DEVELOPMENT_RULES.md`](docs/engineering/DEVELOPMENT_RULES.md) — canonical stack and engineering rules.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — implemented foundation architecture and verification lifecycle.
- [`docs/specs/PROJECT_SPEC.md`](docs/specs/PROJECT_SPEC.md) — temporary condensed product specification and roadmap.

The repository intentionally has **no CI**. Development evidence comes from explicit local commands.
