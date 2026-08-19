# Agent notes

Canonical engineering contract: [`docs/specs/TZ.md`](docs/specs/TZ.md).  
Documentation map: [`docs/INDEX.md`](docs/INDEX.md).  
Stack how/how-not: [`docs/engineering/STACK.md`](docs/engineering/STACK.md).  
Cursor splits: [`.cursor/rules/`](.cursor/rules/).

**AI Layer is the control plane.** Do not invent a parallel `продолжай` / STOP / Task protocol, and do not reprint AI Layer bootstrap, `project_status`, `work_*`, `task_*`, or `epic_*` procedure in this repo.

This repository owns product, architecture, modeling, and verification evidence:

- C++23 Simulation Core is the only authoritative world.
- Godot 4 is presentation, input, and UI only.
- The GDExtension adapter is the only Godot ↔ simulation seam.
- Ordinary work stays a playable vertical slice, not a subsystem tunnel.

Stack details, forbidden imports, playtest entry, and first spine live in the TZ. How/how-not for C++, Godot, GDExtension, and tools: [`docs/engineering/STACK.md`](docs/engineering/STACK.md). Workflow, continuation, and Work/Task/Epic lifecycle live in AI Layer.
