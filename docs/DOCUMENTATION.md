# Documentation Policy

**Status:** ACTIVE

## Purpose

Repository documentation is a compact, reliable system of record for humans and coding agents. Its job is to eliminate rediscovery and ambiguity without becoming a second stale implementation.

The root [`AGENTS.md`](../AGENTS.md) is the short entry point. Detailed knowledge belongs in canonical documents under `docs/` and is discoverable through [`INDEX.md`](INDEX.md).

## Canonical ownership

One contract has one canonical home:

- exact agent operating procedure and commands -> [`engineering/AGENT_RUNBOOK.md`](engineering/AGENT_RUNBOOK.md);
- engineering/toolchain/dependency/platform rules -> [`engineering/DEVELOPMENT_RULES.md`](engineering/DEVELOPMENT_RULES.md);
- implemented repository architecture -> [`ARCHITECTURE.md`](ARCHITECTURE.md);
- documentation policy -> this file;
- product direction/roadmap while bootstrap work continues -> [`specs/PROJECT_SPEC.md`](specs/PROJECT_SPEC.md);
- actual player-visible behavior -> future `GAME.md`, but only after gameplay exists.

Other documents may summarize and link, but must not silently fork a canonical contract.

## One navigation root

`docs/INDEX.md` is the catalog for canonical documentation. Every canonical document must have:

- status;
- path;
- purpose;
- update trigger.

A new agent should be able to decide what to read from the index without scanning the repository.

## Progressive disclosure

Keep `AGENTS.md` compact. Put operational detail in the runbook and engineering detail in development rules.

Use nested `AGENTS.md` only when a subtree has durable, materially different rules. Do not use nested agent files as general documentation storage.

Create sub-indexes only when a directory becomes large enough that `docs/INDEX.md` no longer provides clear navigation.

## Document lifecycle

### ACTIVE

Current source of truth. Update it in the same bounded task when its real contract changes.

### TEMPORARY

A phase-bound document such as the condensed project specification.

Every temporary document must state:

- why it exists;
- what makes it obsolete;
- where durable knowledge moves before deletion.

When obsolete, delete it from the active tree and update `docs/INDEX.md`. Git history is the archive unless the user explicitly asks for a historical document.

### Decisions

Create a decision record only for a choice that is expensive or difficult to reverse and whose rationale will matter later. Do not create ADRs for routine implementation choices.

Decision records are historical rationale, not a second copy of current architecture.

## Truth vocabulary

Canonical documents describe current truth, not aspirations disguised as implementation.

Use:

- **must / must not** — hard current constraint;
- **default** — current policy that can be deliberately changed;
- **planned** — does not exist yet;
- **verified** — actually checked with evidence;
- **supported upstream, not verified here** — dependency/platform capability exists upstream but has not been exercised in the current environment.

Do not claim a tool, command, API, platform path or feature exists until repository code or a completed verification proves it.

## Operational documentation requirements

When an agent-facing command or lifecycle changes, update [`engineering/AGENT_RUNBOOK.md`](engineering/AGENT_RUNBOOK.md) in the same task.

The runbook must be executable guidance, not prose-only intent. For every canonical workflow it should state:

- exact command;
- prerequisites;
- what the command verifies;
- expected success signal;
- artifacts/evidence produced;
- bounded timeout/ownership expectations where processes are involved;
- common failure modes and what not to do.

This is specifically to ensure a fresh agent session does not have to rediscover how to build or operate the stack.

## Update triggers

Examples:

- new or renamed `tools/dev.py` command -> update agent runbook;
- compiler/CMake/dependency/platform policy change -> update development rules;
- repository dependency direction or verification lifecycle change -> update architecture;
- milestone/product invariant change -> update temporary project spec;
- first real player-visible gameplay -> create/index `GAME.md`;
- player-visible control or behavior later changes -> update `GAME.md`.

Do not perform broad documentation cleanup during unrelated gameplay tasks.

## Link/index automation

`python tools/dev.py docs-check` is the canonical documentation hygiene command. It verifies:

- local Markdown links resolve;
- every canonical document under `docs/` declares `ACTIVE` or `TEMPORARY` status;
- every canonical document is represented in `docs/INDEX.md`;
- the repository has no `.github/workflows` directory while CI is intentionally disabled.

The checker does not validate external URLs because the required development loop is network-free.

## Before completing a documentation task

1. run `python tools/dev.py docs-check`;
2. verify every touched command still matches the actual CLI help/implementation;
3. verify no deleted/renamed path remains indexed;
4. verify temporary docs still have a valid retirement condition;
5. verify no two canonical documents now own the same contract;
6. verify `README.md` and `AGENTS.md` point to the correct canonical entry points;
7. do not claim gameplay exists if only platform/foundation fixtures exist.

## Filenames and placement

- Use stable descriptive names, not dates/task numbers, for canonical docs.
- Use Markdown for hand-maintained repository docs.
- Keep temporary product/project contracts under `docs/specs/`.
- Keep engineering policy/runbooks under `docs/engineering/`.
- Add `docs/models/` and `docs/decisions/` only when the first real document is needed.

## Do not add

- empty placeholders;
- speculative future subsystem manuals;
- generated code descriptions that are easier to read from code itself;
- duplicate vendor-specific agent instructions unless a real tool requires them;
- stale completed task plans in the active reading path;
- `GAME.md` before gameplay exists;
- CI instructions while the repository intentionally has no CI.
