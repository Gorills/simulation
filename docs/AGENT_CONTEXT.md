# Agent context architecture

This document explains **how project context is packaged for coding agents**. It is maintainer guidance, not a second set of project rules.

## Design goal

The repository must work well with both weak and strong models:

- a weak model receives a small set of unambiguous invariants and a deterministic route to the next source;
- a strong model can progressively load deeper product, architecture, modeling, engineering, research and ADR context without receiving unrelated material every turn;
- every coding model evaluates whether a requested change is coherent **before** production implementation instead of assuming the user supplied the correct dependency order;
- no host-specific instruction file becomes an independent source of product truth;
- AI Layer remains an external zero-footprint control plane.

## Context layers

| Layer | Location | Loaded when | Owns |
| --- | --- | --- | --- |
| universal bootstrap | `AGENTS.md` | project/session startup where supported | short invariants, change-admission route, source routing, AI Layer boundary |
| change admission | `docs/CHANGE_ADMISSION.md` | before production implementation | readiness/dependency/conflict judgment; no task state |
| host compatibility shims | `CLAUDE.md`, `GEMINI.md` | relevant host startup | import the universal bootstrap; no duplicated rules |
| path-scoped editor hints | `.cursor/rules/*.mdc` | matching files | short reminders + canonical doc pointer |
| canonical project docs | `docs/*.md`, `docs/engineering/*`, ADRs | on demand | product/architecture/modeling/verification/stack facts |
| mechanic contracts | `docs/models/*` | when the mechanic is affected | serious implemented/modeling subsystem contract |
| repeatable workflows | AI Layer project skills / host-native skills | when relevant | procedural skill instructions |
| hard enforcement | build, tests, linters, architecture gates, bounded local playtests | execution | mechanically provable invariants |

Do not copy the same explanation across context layers. A short bootstrap/rule may repeat a high-cost invariant, but the canonical explanation still has one owner.

## Why the root bootstrap is small

OpenAI's own agent-first repository guidance reports that a giant `AGENTS.md` failed because it crowded out task/code context, diluted priorities, went stale and was hard to verify. Their replacement pattern is a short `AGENTS.md` used as a map into a structured repository knowledge base. That is the primary design precedent here.

The other major hosts converge on the same direction:

- Cursor supports `AGENTS.md` for simple repository-wide instructions and `.cursor/rules` for scoped/relevance-based rules.
- Claude Code recommends concise always-loaded project instructions (roughly under 200 lines) and moving narrow or multi-step material into path-scoped rules or skills.
- Gemini CLI uses hierarchical `GEMINI.md` context and supports `@file` imports.
- GitHub Copilot supports `AGENTS.md` as shared agent instructions in multiple agent/chat/review surfaces, with separate path-specific mechanisms when needed.

Therefore:

> Always-on context routes; it does not try to teach the whole project.

Do not put the full C++ style guide, Godot practices, roadmap, research corpus, change-admission examples or playtest manual in `AGENTS.md`.

## Change admission is repository policy, not workflow state

A coding model must not assume that a user request names the correct implementation order. The repository therefore owns one cross-host admission policy in [`CHANGE_ADMISSION.md`](CHANGE_ADMISSION.md).

That policy belongs here because it protects repository correctness:

- accepted architecture and model boundaries must not be bypassed by an eager implementation;
- missing load-bearing prerequisites must not be replaced by hardcoded fake state;
- a user may reprioritize product outcomes, but reprioritization does not erase dependencies;
- weak and strong models should reach the same basic verdict from the same repository facts.

This does **not** make the repository a workflow engine. Change admission stores no current task, queue, continuation state, Work/Epic lifecycle or project database. AI Layer may own which request is active and its durable workflow; the repository only answers whether implementing that request against the current codebase is coherent now.

## Cross-host compatibility

### Codex / tools that understand `AGENTS.md`

Read `AGENTS.md` directly. Codex supports hierarchical `AGENTS.md` scoping, but do not introduce nested files merely because the mechanism exists. Add one only when a real subtree has stable scoped invariants and the hosts used for that subtree will interpret it consistently; otherwise prefer canonical docs plus each host's path-scoped mechanism.

### Cursor

Cursor understands a root `AGENTS.md` for straightforward project instructions. `.cursor/rules` is reserved here for **path-scoped** reminders. There is deliberately no always-applied `project-contract.mdc`: that would duplicate the universal bootstrap.

### Claude Code

`CLAUDE.md` imports `AGENTS.md` using Claude's supported import syntax. Do not expand it into another project manual. Personal project notes belong in `CLAUDE.local.md` and are gitignored.

If future Claude-only path rules provide real value, keep them thin and route to the same canonical project docs rather than copying Cursor rules verbatim.

### Gemini CLI

`GEMINI.md` imports `AGENTS.md` using Gemini's supported `@file` syntax. Do not maintain separate Gemini policy text.

### GitHub Copilot / other hosts

Prefer shared `AGENTS.md` support when the active surface supports it. Do not create `.github/copilot-instructions.md` just to duplicate the same repository-wide rules. A host-specific instruction file is justified only for a measured capability gap or genuinely host-specific behavior.

## Skills belong outside the repository with AI Layer

AI Layer's target-project architecture keeps project workflow state and project-specific native skills under the machine AI Layer home, materializing namespaced skills into host-native catalogs.

Therefore this repository does **not** commit parallel copies under `.claude/skills`, `.agents/skills`, `.github/skills`, or another host catalog merely to mirror AI Layer.

A project skill is appropriate when content is:

- a repeatable multi-step procedure;
- useful only for a class of tasks;
- too detailed for always-on context;
- stable enough to reuse;
- better loaded progressively by host skill selection.

Likely project skills once executable workflows exist:

- native C++ verification;
- Godot/GDExtension build-load smoke test;
- bounded gameplay playtest and evidence capture;
- simulation determinism investigation;
- mechanic/model research with source-quality checks.

A skill points back to repository-owned executable commands and canonical docs. It does not redefine product invariants, change-admission verdicts, or invent its own Work/Task lifecycle.

## When prose is not enough

Agent instructions are guidance, not enforcement. When a rule is mechanically testable, move the guarantee into executable infrastructure:

- forbidden dependency edges → CMake target graph / architecture test;
- formatting/style → formatter/linter;
- deterministic behavior → native deterministic tests;
- one supported playtest entry → executable supervisor/tooling;
- dependency versions → lock/build files;
- required verification → repository-owned local verification gate.

Change admission itself is partly judgment and cannot be reduced to a linter: whether a requested capability is premature depends on product/model/architecture context. Mechanically testable consequences of an accepted decision should still be encoded in code/build/tests rather than repeated as prose.

## Rule maintenance checklist

Before adding or expanding an agent rule:

1. Is the fact already owned by a canonical document or executable file?
2. Must every task know it? If not, scope it.
3. Is it a repeatable procedure? If yes, prefer a skill.
4. Can it be enforced mechanically? If yes, add a local gate rather than relying on prose.
5. Does it duplicate AI Layer workflow/state? If yes, do not add it.
6. Does it name a version/API/tool that can drift? Route to the lock/version owner.
7. Is it specific and testable enough that two agents should interpret it the same way?
8. Is it host-specific? If not, keep it out of host-specific files.

## Primary references

- OpenAI, agent-first repository context / short `AGENTS.md` as map: <https://openai.com/index/harness-engineering/>
- OpenAI Codex / `AGENTS.md`: <https://openai.com/index/introducing-codex/>
- Cursor Rules / scoped project rules and `AGENTS.md`: <https://docs.cursor.com/context/rules-for-ai>
- Claude Code project instructions / scoped rules: <https://code.claude.com/docs/en/memory>
- Claude Code skills vs always-loaded instructions: <https://code.claude.com/docs/en/features-overview>
- Gemini CLI `GEMINI.md` hierarchy and imports: <https://geminicli.com/docs/cli/gemini-md/>
- GitHub Copilot custom-instruction support: <https://docs.github.com/en/copilot/reference/custom-instructions-support>
- AI Layer target-project footprint and skills boundary: `Gorills/ai-layer` current `ARCHITECTURE.md` and `README.md`
