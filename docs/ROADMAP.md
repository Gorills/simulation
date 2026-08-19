# Development roadmap

This roadmap describes **product/build milestones**, not AI Layer Work/Task/Epic state. Current repository source and executable evidence remain authoritative.

## Current state

Milestone 0's executable spine is present in source: CMake presets, separated `sim_core`/`sim_protocol` targets, deterministic tests, pinned dependencies, the GDExtension adapter, a Godot 4.7 third-person presentation client, keyboard/mouse + gamepad controls, project-wide UI design system, collision-aware camera, and a bounded smoke-playtest supervisor.

The architecture foundation establishes stable positive `EntityId`, actor parity, protocol-owned human-control binding, separate `SimulationTick` / `WorldRevision`, `ObservedWorldProjection`, and one Godot `WorldPresentation` identity/presence owner. Godot is a presentation replica rather than world authority.

The current `CharacterBody3D` motor remains a responsive presentation/prediction shell. It is not authoritative world location.

**Milestone 0 is not accepted merely because the files exist.** Acceptance remains defined by [`VERIFICATION.md`](VERIFICATION.md).

## Milestone rule

Every milestone becomes playable. Simulation-only depth may not silently accumulate several milestones ahead of player experience.

Systemic milestones preserve actor parity: a human-controlled actor enters the same world-rule path as an equivalent NPC.

## Milestone 0 — Toolchain & Playable Spine

Implemented path includes:

- explicit local bootstrap/toolchain;
- C++23 `sim_core` + `sim_protocol` one-way dependency;
- CMake/Ninja/CTest and immutable dependency pins;
- Godot 4.7 third-person presentation client;
- semantic keyboard/mouse/gamepad controls and collision-aware camera;
- project-wide responsive UI design system at a 1920×1080 logical baseline;
- stable actor `EntityId` and protocol-side controlled-actor binding;
- `SimulationTick` separated from `WorldRevision`;
- `ObservedWorldProjection` carrying authoritative identity/presence without bootstrap grid coordinates;
- `WorldPresentation` / `EntityBinding` as the Godot presentation identity owner;
- deterministic native tests;
- bounded local smoke evidence and screenshot artifacts.

The native grid-step remains **bootstrap transport evidence only**. Production third-person locomotion must use semantic intent -> authoritative Simulation spatial result -> revisioned Godot samples/interpolation.

Acceptance:

> The game opens in the pinned Godot environment; the player can control the third-person presentation responsively with keyboard/mouse and gamepad; and separate smoke evidence proves the Godot -> GDExtension -> protocol -> C++ authoritative round-trip plus identity binding.

## Pre-spatial simulation model foundation

**This foundation comes before the authoritative spatial contract.** Spatial work must not accidentally freeze simplistic assumptions about what an actor, role, institution or world-resolution unit means.

Established contracts:

1. use a sourced comparative non-magical baseline around **1180–1230**, narrowing region/mechanic research when necessary;
2. model social position/roles compositionally from property, rights, skills, relationships, office, institutions, resources, coercive power, magic and history rather than one authoritative class/rank enum;
3. treat magic as a permanent causal counterfactual: every serious model records which mundane constraints future magic could alter;
4. let institutions react to disruptive actors through actual capabilities, without hardcoded anti-magic/anti-player plot armor;
5. choose identity-resolved, aggregate-resolved or deferred detail according to causal/gameplay relevance rather than camera distance;
6. preserve conserved state, important identities, rights/debts/obligations and observed history across any future representation-resolution change;
7. evaluate realism through observable causal patterns and trajectories, not raw model-variable count;
8. keep adaptive **model** fidelity separate from speculative performance architecture such as regional LOD/ECS/sharding.

Canonical sources:

- [`research/high-medieval-baseline-c1200.md`](research/high-medieval-baseline-c1200.md)
- [`decisions/0005-historical-counterfactual-and-causal-fidelity.md`](decisions/0005-historical-counterfactual-and-causal-fidelity.md)
- [`MODELING.md`](MODELING.md)

Acceptance:

> A new serious simulation mechanic can state its historical baseline, causal entities/state, identity-vs-aggregate-vs-deferred resolution, magic sensitivity, observable criteria and deliberate omissions without introducing a fake universal medieval/magic/fidelity framework.

No empty `MagicSystem`, `SocialClass`, `FidelityManager`, universal modifier bus or dynamic aggregation framework is part of this stage.

## Pre-Milestone 1 authoritative spatial bridge — next after model foundation

After the historical/magic/fidelity foundation is accepted, complete the minimum production bridge needed to stop using the Godot transform as de facto actor location:

1. define the real authoritative spatial/location representation from actual terrain/navigation/determinism requirements;
2. send semantic controlled-actor movement intent through protocol;
3. produce ordered/revisioned authoritative movement/location samples;
4. update Godot representation by `EntityId` through the existing presentation owner;
5. interpolate samples smoothly and add local prediction/reconciliation only if measured playtest need justifies it;
6. prove that leaving/rematerializing a representation does not create/delete the Simulation entity.

Do not turn this bridge into ECS, networking, sharding or automatic regional simulation LOD work.

## Milestone 1 — Living Need

- one NPC need;
- one causal task;
- authoritative travel/action;
- visible Godot result from Simulation projection/state;
- player can interfere or help through the same actor/world-rule layer;
- the NPC continues to exist/act when its Godot representation is absent.

Acceptance:

> An NPC acts because of authoritative world state, and the player can change the outcome through the same causal system rather than a player-only shortcut.

## Milestone 2 — Household Resource Loop

- household stock;
- production and consumption;
- shortage;
- player/NPC trade, gift or work through shared transaction/world rules;
- shop/inventory UI displays projections rather than owning stock/money.

Acceptance:

> The village can develop a problem without the player, and the player can address it through more than one real path while all resource transfers remain authoritative Simulation state.

## Milestone 3 — Social Consequence

- one useful trust/obligation/reputation dimension;
- interaction changes a future opportunity;
- an NPC remembers a relevant event;
- Godot only displays the relationship/reaction projection.

Acceptance:

> A later action becomes available, unavailable or differently costly because of earlier authoritative social state.

## Milestone 4 — First Institution / Politics

- one office or authority;
- one real permission/obligation;
- one way any qualified actor can gain, lose or influence it;
- a visible distributional consequence.

Acceptance:

> The player can participate in power through the same world state/rules available to simulated actors rather than a quest flag or Godot-owned state.

## Milestone 5 — First Magic Counterfactual

- one magic capability;
- explicit access and cost;
- a non-magical alternative/baseline;
- one load-bearing mundane constraint changed by magic;
- economic/social/institutional downstream effects through existing causal state.

Acceptance:

> Magic changes the authoritative system trajectory and institutions/actors react through actual capabilities; it is not only presentation or a subsystem multiplier.

## Milestone 6 — Emergent Role

Enough compositional state exists that any suitably situated actor — including the human-controlled one — can occupy materially different roles such as worker, trader, bandit, apprentice or office-holder through world paths.

Acceptance:

> Roles emerge from state/rules without an authoritative hardcoded player/social class.

## Milestone 7 — Persistence & Repeated Play

- save/load;
- stable entity identities preserved;
- replay/determinism evidence;
- a substantial repeated session;
- gameplay fixes based on actual play.

Acceptance:

> The authoritative world preserves understandable consequences across sessions and the game supports continued play.

## Roadmap discipline

Do not insert a large subsystem milestone because its architecture is interesting. A new milestone must identify the new player-visible capability, minimum causal world rule, protocol/projection change and acceptance evidence.

Do not optimize the unseen world with regional LOD/sharding merely because Godot materializes only a subset. Presentation materialization, causal model resolution and runtime performance architecture are three different concerns.

Consequential roadmap changes update product/modeling docs or an ADR when they change a durable contract.
