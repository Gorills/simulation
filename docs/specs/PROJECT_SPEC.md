# Project Specification — World Simulation + Playable Systemic RPG

**Status:** TEMPORARY

## Why this document exists

This is the condensed working contract derived from the original greenfield brief. It preserves product direction and milestone constraints while detailed engineering/toolchain rules live in [`../engineering/DEVELOPMENT_RULES.md`](../engineering/DEVELOPMENT_RULES.md) and exact agent operation lives in [`../engineering/AGENT_RUNBOOK.md`](../engineering/AGENT_RUNBOOK.md).

### Current repository stage

The repository is currently in **Development Foundation** stage only.

Verified foundation now includes native C++23 build/test infrastructure and a real headless-verifiable graphical window/input/framebuffer path. Gameplay has **not** started. There is no Simulation Core, gameplay protocol, game executable, NPC system, economy, player inventory or gameplay runner yet.

Do not interpret the graphical diagnostic fixture as Milestone 0 gameplay.

### Retirement condition

Delete this temporary document when all are true:

1. durable product truth is represented by implemented behavior plus permanent docs such as future `GAME.md`, `ARCHITECTURE.md`, `MODELING_POLICY.md` and model contracts;
2. the active milestone/backlog no longer depends on this bootstrap roadmap;
3. no unique project invariant remains only in this file.

Move durable truth to the appropriate canonical docs before deletion. Git history is the archive.

## 1. Product goal

Build a continuously playable systemic RPG in which world simulation, economy, social relations, institutions/politics, trade and magic deepen together through small causal vertical slices.

The player should act inside the same world model that drives NPCs and institutions rather than interact with disconnected quest flags or decorative simulation.

Once a playable path exists, `main` should remain runnable/playable. Each new capability should prefer a small complete causal loop over a deep subsystem that the player cannot yet experience.

## 2. Core causality loop

A normal gameplay capability should eventually cross all four layers:

```text
RULE
  authoritative C++ world rule

CONTRACT
  explicit intent / command / result / events / projection boundary

EXPERIENCE
  visible and controllable behavior in the real graphical game executable

PROOF
  deterministic/targeted tests + bounded real graphical playtest evidence
```

Avoid a simulation waiting room in which backend fidelity advances far beyond player-facing exposure.

### Fidelity ladder

Use the minimum useful fidelity:

- **F0** — absent;
- **F1** — minimal causal/playable rule;
- **F2** — richer constraints/resources;
- **F3** — social/institutional consequences;
- **F4** — long-horizon dynamics;
- **F5** — measured optimization/detail.

Do not push simulation more than roughly one meaningful level ahead of player-facing exposure without a concrete reason.

## 3. Non-negotiable world-model principles

### One authoritative world

Future gameplay authority lives in the C++23 Simulation Core.

Presentation/input/platform code must not become a second truth for position, inventory, money, relationships, ownership, institutions, resources, status, skills, magic outcomes or other gameplay state.

### Determinism where load-bearing

For the same seed, initial state, content/protocol versions and command/step sequence, authoritative results must be reproducible within the declared contract.

Use explicit PRNG state when randomness is introduced. Do not make authoritative results depend on wall-clock time, frame rate, hidden RNG or thread scheduling.

Use integer/fixed/scaled integer representations by default for load-bearing accumulative values such as money, quantities, time, thresholds, rates and obligations.

### Intent, not desired state

Future command flow:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

A client asks to act; it does not set the result.

### No fake depth

Prefer the smallest real causal model to placeholders that imply depth without consequence.

Do not create an abstraction/framework merely because a future system might use it.

### Social consequences must matter

A social dimension exists only if it changes future opportunity, behavior, obligation or cost. Avoid a universal decorative relationship score.

### Historical baseline and magic

Historical research defines the non-magical baseline for a concrete region/period/scenario. Load-bearing assumptions need appropriate sources, but research stops when the causal baseline, plausible range, uncertainty and simplification are sufficient for gameplay.

Magic is an explicit changed law with access, acquisition, cost, range, duration, reliability, failure modes, observability, countermeasures and downstream economic/social/political consequences. Magic must not be a universal explanation for missing modeling.

## 4. Presentation/platform direction

The required player-facing direction is a **native graphical game**, not a terminal game and not a browser gate.

The verified foundation uses:

- native C++23 executable;
- software RGB framebuffer;
- thin vendored Fenster OS window/input layer;
- Linux X11 path on the agent host;
- headless Xvfb automation for real key input and real window capture.

This is a presentation/platform choice, not gameplay architecture. The future renderer consumes projections from the authoritative core.

Browser/WASM may be reconsidered only as an optional adapter in a later explicit stack task. It must never become required until the agent environment can build and run it fully.

## 5. First intended playable vertical slice

A small village built around one shortage and one magical counterfactual.

World eventually contains, minimally:

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

The slice succeeds when a 10–20 minute player can understand who lives here, what happens without the player, what the problem is, what interventions are possible, why choices differ and what changed afterward.

## 6. Roadmap

### Foundation — Development/Graphics Stack

**Current stage. Gameplay is not part of this stage.**

Contains:

- network-free canonical development commands;
- C++23/CMake/Ninja/CTest foundation;
- debug and release presets;
- vendored graphical window/input dependency with provenance/integrity checks;
- real native graphical smoke under Xvfb;
- real OS input injection and real window capture;
- complete agent runbook/documentation;
- no CI.

**Done when:** a fresh agent can read repository instructions and run `python tools/dev.py verify` without rediscovering the stack, the command passes in the agent environment, and the graphical evidence is real native-window evidence.

After this foundation is accepted, stop. Do not automatically begin gameplay.

### Milestone 0 — Playable Spine

**Not started.** Begin only after an explicit later user instruction.

This is the first gameplay task and should remain one coherent vertical slice rather than separate backend/frontend waiting rooms:

1. C++23 authoritative `sim_core`;
2. explicit seeded deterministic initial state;
3. one minimal player intent such as movement;
4. typed protocol/result/projection boundary;
5. dependency-free native deterministic CTest proof;
6. native graphical game executable using the already verified platform/framebuffer stack;
7. real WASD window input mapped to intent;
8. rendering driven by authoritative projections, not client-only position;
9. machine-readable read-only debug evidence suitable for agent verification;
10. canonical bounded gameplay runner created only at this point;
11. real headless graphical run through the game executable;
12. actual screenshot after movement;
13. evidence that movement/state came from the authoritative C++ core;
14. bounded runner-owned cleanup.

**Done when:** the agent itself can run the canonical gameplay scenario in its environment, send real input to the real graphical game executable, inspect a real screenshot/debug projection and prove the outcome came from the authoritative C++ core.

After its report, stop before Milestone 1.

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

One magic capability with explicit access/cost, a non-magical alternative and downstream economic/social/institutional consequences.

**Done when:** magic changes the system trajectory, not merely VFX or a damage number.

### Milestone 6 — Emergent Role

Enough compositional state for at least two different careers/roles to emerge (for example worker/trader/apprentice/office-holder), with UI labels only as projections.

**Done when:** at least two careers arise from world rules without choosing a hardcoded player class.

### Milestone 7 — Persistence & Repeated Play

Save/load, replay determinism for a core scenario, 30–60 minute sessions and gameplay fixes driven by actual play.

**Done when:** consequences persist coherently and the game supports meaningful continued play.

## 7. Gameplay capability definition of done

Once gameplay exists, a capability is complete only when applicable evidence exists for all of these:

- authoritative C++ implementation;
- no presentation-side truth bypass;
- explicit protocol input/output;
- deterministic behavior covered where required;
- targeted regression test for load-bearing causality;
- player-facing feedback in the real graphical game;
- actual invocation in the game;
- real captured graphical frame/screenshot plus debug evidence of expected outcome;
- bounded clean test/playtest lifecycle;
- no unrelated refactor;
- permanent docs/model updated only if a real contract changed.

Compilation or unit tests alone do not prove a gameplay capability is done.

## 8. Things explicitly not to build early

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
- full event sourcing;
- GPU renderer migration without demonstrated need.

Any of these requires a demonstrated player-facing need first.

## 9. Immediate action after foundation completion

Stop and wait for explicit user direction.

Do not use “foundation finished” as permission to begin Milestone 0 automatically. The next gameplay pass must first audit the foundation and then implement only the Playable Spine if the user explicitly asks to start development.
