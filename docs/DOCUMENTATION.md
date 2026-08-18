# Documentation Policy

**Status:** ACTIVE

## Purpose

Repository documentation exists to give humans and agents a small, reliable system of record. It must reduce search and ambiguity, not create another layer of stale state.

The root `AGENTS.md` is a map and hard-guardrail file. Detailed knowledge belongs in canonical documents under `docs/` and is discoverable through [`INDEX.md`](INDEX.md).

## Documentation architecture

### 1. One navigation root

`docs/INDEX.md` is the catalog for active documentation.

Every canonical document must have an entry containing:

- status;
- path;
- purpose;
- update trigger.

An agent should be able to decide what to read from the index without scanning the whole repository.

### 2. One source of truth per contract

A rule should have one canonical home.

Examples:

- engineering/toolchain rules -> `engineering/DEVELOPMENT_RULES.md`;
- current playable behavior -> future `GAME.md`;
- implemented architecture -> future `ARCHITECTURE.md`;
- project roadmap while still needed -> `specs/PROJECT_SPEC.md`.

Other docs may summarize and link, but must not silently fork the contract.

If two documents disagree, fix the duplication rather than adding precedence prose everywhere.

### 3. Progressive disclosure

Keep root instructions compact. Put detail close to its domain and link to it.

Use nested `AGENTS.md` only when a subtree has durable, materially different operating rules. Do not use nested agent files as general documentation storage.

Create sub-indexes only when a directory becomes large enough that the global index no longer makes navigation obvious. Do not create index files for empty or tiny directories.

## Document lifecycle

### ACTIVE

Current source of truth. Update it in the same bounded task when its contract changes.

### TEMPORARY

A phase document such as a bootstrap specification or migration plan.

Every temporary document must state:

- why it exists;
- what makes it obsolete;
- where durable knowledge must move before deletion.

When the retirement condition is met, delete it from the active tree and update `docs/INDEX.md`. Git history is normally sufficient archival storage.

### Decisions

Create a decision record only for a choice that is expensive or difficult to reverse and whose rationale will matter later. Do not create ADRs for routine implementation choices.

Accepted decision records are historical evidence, not a second copy of current architecture. Current architecture belongs in `ARCHITECTURE.md` once that document exists.

## Required document qualities

Canonical documents must describe current truth, not aspirations disguised as implementation.

Use explicit language:

- **must / must not** for hard constraints;
- **default** for a rule that can be overridden by a deliberate decision;
- **planned** for behavior that does not exist yet;
- **verified** only when actually checked.

Do not claim tools, commands, files, APIs, or features exist until the repository proves they exist.

## Update rules for development tasks

Update docs only when the task changes a real contract.

Examples:

- new playable control -> update `GAME.md` when it exists;
- protocol ownership/boundary change -> update `ARCHITECTURE.md` when it exists;
- toolchain/dependency policy change -> update `engineering/DEVELOPMENT_RULES.md`;
- milestone/product invariant change -> update `specs/PROJECT_SPEC.md` while it exists;
- serious new causal mechanic -> add/update a model contract if the mechanic warrants one.

Do not perform broad documentation cleanup during unrelated gameplay tasks.

## Link and index discipline

Before finishing a documentation task:

1. verify every relative Markdown link touched by the task resolves;
2. verify every canonical doc is listed in `docs/INDEX.md`;
3. verify no indexed path points to a deleted/renamed file;
4. verify temporary docs still have a valid retirement condition;
5. verify the change did not create two canonical sources for the same rule.

## Filenames and placement

- Use stable descriptive names, not dates or task numbers, for canonical docs.
- Use Markdown for hand-maintained repository docs.
- Keep temporary product/project contracts under `docs/specs/`.
- Keep engineering rules under `docs/engineering/`.
- Add `docs/models/` and `docs/decisions/` only when the first real document of that type is needed.

## What not to document

Do not add:

- empty placeholders;
- speculative future subsystem manuals;
- generated descriptions of code that are easier to read from the code itself;
- duplicate agent instructions for specific vendors unless a real tool requires them;
- stale completed task plans kept in the active reading path;
- CI instructions or CI documentation while this repository intentionally has no CI.
