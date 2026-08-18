# Project Specification — World Simulation + Playable Systemic RPG

**Status:** TEMPORARY

## Why this document exists

This is the condensed working contract extracted from the original greenfield technical brief. It keeps only project direction and constraints needed to guide development; detailed engineering rules live in [`../engineering/DEVELOPMENT_RULES.md`](../engineering/DEVELOPMENT_RULES.md).

### Retirement condition

Delete this file when all of the following are true:

1. durable product truth is represented by implemented behavior plus current `GAME.md`/`ARCHITECTURE.md`/`MODELING_POLICY.md` and relevant model contracts;
2. the active milestone/backlog no longer depends on this bootstrap roadmap;
3. no unique project invariant remains only in this file.

Before deletion, move only still-relevant durable knowledge to its canonical permanent document. Do not keep a stale archive copy in the active docs tree; Git history is the archive.

## 1. Product goal

Build a continuously playable systemic RPG in a living medieval magical world. Simulation, RPG interaction, economy, social institutions, politics, trade and magic deepen together through small causal vertical slices.

Core loop of causality:

```text
world cause
  -> authoritative simulation consequence
  -> player/NPC opportunity
  -> visible game feedback
  -> player choice
  -> persistent world change
```

The world exists independently of the player. NPCs, households, organizations and institutions act using information and constraints available to them. The player is an ordinary participant in the same world rules and can acquire roles through state, relationships, ownership, rights, obligations, knowledge and actions rather than a hardcoded class.

## 2. Non-negotiable invariants

### Playable main

Once a playable path exists, `main` stays runnable and playable. A bounded task that temporarily breaks it must restore it within the same task.

### Vertical capabilities

The normal unit of development is one player-observable capability crossing only the layers it needs:

```text
minimal authoritative C++ rule
  -> protocol command/result/projection/events
  -> client affordance/feedback
  -> targeted deterministic proof
  -> bounded keyboard/mouse playtest
```

Do not spend a sequence of tasks building simulation internals with no new observable player behavior.

### One authoritative world

C++23 Simulation Core owns world truth. TypeScript presents projections and sends intents; it does not implement an alternate economy, inventory, money, relationship, status or other gameplay truth.

### Fidelity ladder

Subsystems deepen only when their current level is causally real and player-visible:

```text
F0 absent
F1 minimal causal & playable
F2 richer constraints and consequences
F3 institutional/social feedback
F4 long-horizon feedback
F5 optimization/scale only if measured
```

Simulation fidelity must not run materially ahead of player-facing exposure.

### Bounded work

One agent pass performs one explicit bounded task and stops. No unrelated refactors, opportunistic cleanup, speculative frameworks, or automatic next subsystem.

## 3. World-model principles

### Player/NPC symmetry

If the player can own, trade, work, learn, join, obtain office, incur debt, break a rule or use magic, the permission and consequence should follow world state rather than `if (actor.is_player)` exceptions. NPC decision algorithms may differ, but constraints and consequences are shared.

### Opportunity-driven RPG

Roles emerge from compositional state such as skills, knowledge, property, capital, tools, rights/licenses, legal status, household ties, organizations, offices, reputation, relationships, debts/obligations, land, magic access and action history.

Do not use an authoritative `PlayerClass` enum as the source of permissions.

### Bounded rationality

NPCs act from available information: observation, memory, conversation, rumor, letters, announcements, markets, organizations or explicit magical information channels. They should not use hidden global truth without a deliberate reason.

### Physical + institutional economy

Production/trade causality distinguishes resources, labor/time, tools/skills, access rights, ownership/control, transport, storage/loss, obligations/extraction and actual exchange. Trade is a concrete transaction between actors/places/stocks, not a magical global price API.

### Politics from real authority

Politics appears through actual rights, restrictions, offices, power resources, appointment/removal, punishments, obligations, support/coalitions and information constraints. Start with one local institution, not a global state simulator.

### Social consequences must matter

A social dimension exists only if it changes future opportunity, behavior or cost. Avoid a universal decorative `-100..100` relationship score.

### Historical baseline and magic

Historical research defines the non-magical baseline for a concrete region/period/scenario. Load-bearing assumptions need appropriate sources, but research stops when the causal baseline, plausible range, uncertainty and simplification are sufficient for gameplay.

Magic is an explicit changed law with access, acquisition, cost, range, duration, reliability, failure modes, observability, countermeasures and downstream economic/social/political consequences. Magic must not be a universal explanation for missing modeling.

## 4. First playable vertical slice

A small village built around one shortage and one magical counterfactual.

World contains, minimally:

- several households and NPCs;
- homes;
- one production place;
- one exchange/social place;
- one short resource chain;
- one local authority/institution;
- player character.

Player can eventually:

- move;
- talk;
- recognize a local problem;
- carry items;
- work/help;
- perform a simple trade/transfer;
- use one magic capability;
- cause a persistent consequence;
- see at least one social/institutional reaction.

NPCs can eventually:

- have a need/obligation;
- select a feasible task;
- travel;
- work;
- carry/consume/produce a resource;
- react to shortage;
- refuse impossible actions.

The slice succeeds when a 10–20 minute player can understand who lives here, what happens without the player, what the problem is, what interventions are possible, why choices differ, and what changed afterward.

## 5. Milestone roadmap

Every milestone remains playable.

### Milestone 0 — Toolchain & Playable Spine

- C++23 native core;
- CMake/Ninja presets;
- pinned Emscripten;
- same core builds native + WASM;
- strict TypeScript Canvas shell;
- protocol round-trip;
- WASD movement through authoritative command/state;
- singleton bounded `tools/play.py`;
- screenshot + read-only debug state;
- one deterministic native test.

**Done when:** the game opens in a browser, the player can move, and evidence proves browser movement/state comes from the real C++/WASM core rather than client-only state.

### Milestone 1 — Living Need

One NPC need -> one causal task -> travel/action -> visible result -> player can interfere/help.

**Done when:** the NPC acts because of world state and the player can change the outcome.

### Milestone 2 — Household Resource Loop

Household stock -> production -> consumption -> shortage -> player trade/gift/work response.

**Done when:** the village can develop a problem without the player and the player can resolve it by different causal paths.

### Milestone 3 — Social Consequence

One meaningful trust/obligation/reputation dimension changes a future opportunity and the relevant NPC remembers the event.

**Done when:** tomorrow's available action differs because of yesterday's behavior.

### Milestone 4 — First Institution / Politics

One office/authority -> one real permission/obligation -> one way to gain/lose/influence it -> visible distributional consequence.

**Done when:** the player can participate in power through world state rather than a quest flag.

### Milestone 5 — First Magic Counterfactual

One magic capability with explicit access/cost, a non-magical alternative, and downstream economic/social/institutional consequences.

**Done when:** magic changes the system trajectory, not merely VFX or a damage number.

### Milestone 6 — Emergent Role

Enough compositional state for at least two different careers/roles to emerge (for example worker/trader/apprentice/office-holder), with UI labels only as projections.

**Done when:** at least two careers arise from world rules without choosing a hardcoded player class.

### Milestone 7 — Persistence & Repeated Play

Save/load, replay, native/WASM parity for a core scenario, 30–60 minute sessions, and gameplay fixes driven by actual play.

**Done when:** consequences persist coherently and the game supports meaningful continued play.

## 6. Capability definition of done

A gameplay capability is complete only when applicable evidence exists for all of these:

- authoritative C++ implementation;
- no client-side truth bypass;
- explicit protocol input/output;
- deterministic behavior covered where required;
- targeted regression test for load-bearing causality;
- player-facing feedback;
- actual invocation in the game;
- screenshot/debug evidence of expected outcome;
- no unrelated refactor;
- permanent docs/model updated only if a real contract changed.

Compilation or unit tests alone do not prove a gameplay capability is done.

## 7. Things explicitly not to build early

Do not start with:

- combat framework;
- 3D engine migration;
- giant procedural world;
- large profession catalog or universal skill tree;
- MMO/network architecture;
- LLM NPCs or universal GOAP planner;
- detailed genetics/full-body metabolism;
- global political/religion simulator;
- universal magic ontology;
- weather physics;
- microservices/database cluster;
- custom scripting/plugin framework;
- ECS framework;
- full event sourcing.

Any of these requires a demonstrated player-facing need first.

## 8. Immediate first development task after repository preparation

The first gameplay development task is the **Playable Spine** and should remain one coherent vertical task:

1. minimal CMake project;
2. C++23 `sim_core`;
3. explicit seeded deterministic state;
4. one minimal intent such as movement;
5. native GoogleTest/CTest proof;
6. pinned Emscripten bootstrap;
7. the same C++ core compiled to WASM;
8. minimal protocol adapter;
9. strict TypeScript client;
10. Canvas scene and WASD;
11. read-only `window.__GAME_DEBUG__`;
12. singleton/watchdog `tools/play.py`;
13. real Chromium smoke run and screenshot;
14. evidence that authoritative state came from C++/WASM rather than client-only state.

Do not split this into a long simulation/tooling waiting room. After its report, stop before Milestone 1 work.
