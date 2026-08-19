# Development roadmap

This roadmap describes **product/build milestones**, not AI Layer Work/Task/Epic state. Current repository source and executable evidence remain authoritative.

## Current state

Milestone 0's executable spine is present in source: CMake presets, separated `sim_core`/`sim_protocol` targets, deterministic tests, pinned dependencies, the GDExtension adapter, a Godot 4.7 third-person presentation client, keyboard/mouse + gamepad controls, project-wide UI design system, collision-aware camera, and a bounded smoke-playtest supervisor.

The architecture foundation establishes stable positive `EntityId`, actor parity, protocol-owned human-control binding, separate `SimulationTick` / `WorldRevision`, `ObservedWorldProjection`, and one Godot `WorldPresentation` identity/presence owner. Godot is a presentation replica rather than world authority.

The historical/magic/causal-fidelity foundation, authoritative spatial representation contract, and grounded locomotion acceptance/test-arena contract are accepted. The Godot-free grounded locomotion transition proves deterministic flat movement, wall blocking/sliding semantics, walkable-versus-too-steep slope classification/traversal, ordinary 300 mm step-up versus 301 mm blocking-step semantics, and ledge support loss -> gravity-driven fall -> stable lower-plane landing against neutral Simulation-owned geometry. That transition is routed through an actor-generic `World` batch and semantic controlled-actor application protocol; each successful locomotion tick produces one canonical ordered authoritative sample batch, and the v5 GDExtension/Godot bridge consumes those batches on fixed physics ticks with tick/revision/epoch guards and presentation interpolation.

The controlled `CharacterBody3D` is now a presentation replica: its physics-root position/velocity come from Simulation samples rather than `move_and_slide()`, Godot gravity, local acceleration or local sprint. Facing/camera remain presentation concerns. Sprint/acceleration/deceleration are deliberately inactive until authoritative locomotion semantics define them.

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

The native grid-step remains **bootstrap transport evidence only**. Production third-person movement now follows semantic intent -> authoritative Simulation spatial result -> revisioned Godot samples/interpolation; the bootstrap grid cannot move the production spatial actor.

Acceptance:

> The game opens in the pinned Godot environment; the player can control the third-person presentation responsively with keyboard/mouse and gamepad; and separate smoke evidence proves the Godot -> GDExtension -> protocol -> C++ authoritative round-trip plus identity binding.

## Pre-spatial simulation model foundation — accepted

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

No empty `MagicSystem`, `SocialClass`, `FidelityManager`, universal modifier bus or dynamic aggregation framework is part of this foundation.

## Pre-Milestone 1 authoritative spatial bridge — in progress

The bridge is split deliberately so a guessed physics framework does not become the architecture.

### Stage A — durable spatial contract — accepted

Implemented:

1. exact identity-resolved Simulation spatial state uses signed 64-bit millimeters / millimeters-per-second;
2. axes match the Godot metric Y-up/right-handed 3D convention, while Godot types stay outside Simulation;
3. exact `SpatialState` is optional — authoritative existence does not imply every entity always has a microscopic 3D pose;
4. `SpatialEpoch` separates continuous motion from teleport/respawn/discontinuous relocation;
5. `ControlledActorSpatialProjection` carries identity, position, velocity, epoch, tick/revision and protocol version;
6. GDExtension translates millimeters to Godot meters;
7. `WorldPresentation` uses the authoritative sample for initial controlled-actor placement and resets interpolation after placement;
8. the bootstrap grid probe remains isolated and cannot move production `SpatialState`.

See [`decisions/0006-authoritative-spatial-contract.md`](decisions/0006-authoritative-spatial-contract.md) and [`models/spatial-location.md`](models/spatial-location.md).

### Stage B — grounded locomotion acceptance contract — accepted

The repository defines the minimum native/test-arena outcomes the solver must prove:

1. stable flat-ground movement/rest;
2. blocking head-on wall contact;
3. preserved tangential movement on an oblique wall;
4. a paired walkable/unwalkable slope fixture around the solver's explicit threshold;
5. a paired traversable/blocking step fixture around the solver's explicit threshold;
6. ledge -> falling -> landing behavior;
7. deterministic replay from identical initial state, fixture and ordered intent stream;
8. no Godot scene/collider as authoritative collision input;
9. ordinary movement remains in one `SpatialEpoch`;
10. the same world-rule transition remains usable for NPC intent later.

See [`models/grounded-locomotion.md`](models/grounded-locomotion.md).

### Stage C1 — flat/wall/slope native solver slices — implemented

The current implementation deliberately covers only what it can prove:

1. neutral static environment data uses bounded `GroundPatch` support plus axis-aligned `VerticalBarrier` blockers;
2. the body is an upright capsule matching the existing playable shell: **380 mm radius / 1800 mm height**;
3. fixed movement uses the existing project **60 Hz** baseline;
4. the authoritative speed fixture uses the existing playable **5800 mm/s** walk-speed baseline;
5. `PlanarMoveIntent` is bounded analog intent, never final displacement/state;
6. integer integration retains per-axis remainder so millimeter motion does not lose fractional distance every tick;
7. flat support, exact one-second replay, stable head-on wall blocking, oblique tangential progress and zero-input rest are native-test contracts;
8. linear `GroundPatch` ramps are classified by integer rise/run against the existing project-owned **50°** presentation baseline (represented authoritatively as **1192 mm rise / 1000 mm run**);
9. walkable ramps derive authoritative Y from the support surface while preserving deterministic continuous movement;
10. too-steep ramps block the gradient component while preserving supported tangential motion;
11. zero input on walkable slopes does not create implicit downhill creep;
12. repeated slope replay is deterministic and remains in the same `SpatialEpoch`.

The current environment vectors are a tiny acceptance-fixture representation. Their linear scans are **not** the production large-world spatial index; before real-location scale, local geometry lookup must be bounded and measured under [`PERFORMANCE.md`](PERFORMANCE.md).

These body/timing/speed/slope values are repository migration baselines taken from the already playable client, not copied engine defaults. They remain reviewable as authoritative movement becomes playable.

### Stage C2 — next grounded solver slices — in progress

Implemented:

1. ordinary step-up versus blocking-step semantics use an explicit **300 mm** `max_step_up` baseline; the neutral paired fixture proves **300 mm traverses / 301 mm blocks**, preserves tangential movement when the entry axis is blocked, preserves `SpatialEpoch`, and replays deterministically;
2. ledge support loss, gravity/falling and stable landing use a non-magical **9807 mm/s²** gravity baseline, preserve takeoff planar velocity without inventing air-control semantics, carry fractional vertical integration deterministically, prevent downward support snap/tunneling, clear vertical velocity/remainders on landing, preserve `SpatialEpoch`, and replay deterministically;
3. the solver is exposed through one actor-keyed `World` locomotion batch and a semantic controlled-actor protocol path: the full batch validates before mutation, advances `SimulationTick` / `WorldRevision` once regardless of actor count, persists fixed-step continuation through `WorldSnapshot` schema v2, and keeps controller intent submission separate from authoritative time advancement. No final-position/velocity setter exists. The temporary protocol fixture uses Simulation-owned flat acceptance geometry rather than Godot colliders;
4. every successful World locomotion batch returns post-transition `GroundedLocomotionSample` values with one shared post-transition tick/revision and canonical ascending `EntityId` ordering, independent of intent-source collection order. The application protocol maps that result to one `AuthoritativeMovementSampleBatch`; repeated batches are ordered by increasing locomotion tick, while revision locates them relative to other authoritative World mutations. No extra presentation sequence counter is introduced;
5. protocol v5 exposes semantic move-intent submission plus authoritative sample-batch advancement through `SimFacade`. `WorldPresentation` requires consecutive locomotion ticks, strictly newer revisions, matching protocol version, strictly ascending observed `EntityId` samples and a controlled-actor sample before applying the batch. Same-epoch samples update the physics-root transform on Godot physics ticks and use Godot's enabled physics interpolation for rendered smoothing; epoch changes reset interpolation. The old local `move_and_slide()`/gravity/acceleration/sprint position path has been removed, so Godot no longer creates a competing authoritative location outcome.

Still required before Stage C2 is complete:

6. add local prediction **only if** measured playtest latency shows a real need; this is optional, not a prerequisite by default;
7. prove an actual NPC decision source feeds equivalent intent through the same actor-generic `World` transition without a player-only world path.

The initial step threshold is a project-owned migration/acceptance value, not an engine default. It is numerically aligned with the existing playable profile's 0.3 m local floor-contact reach while retaining separate step and snap semantics, and it remains subject to later player-facing feel verification.

The gravity baseline is the nearest integer millimeter representation of standard gravity **9.80665 m/s²**. It is a configurable non-magical physical baseline, not a controller-engine default; future magic or world conditions may alter gravity only through explicit authoritative rules.

The current flat protocol integration context is deliberately temporary Stage C2 test data. It proves semantic command -> shared World transition -> authoritative `SpatialState`; it does **not** declare the visible Godot scene floor to be authoritative Simulation content. The visible Godot floor/collider now serves presentation/camera demonstration only and cannot resolve authoritative locomotion.

Do not build the visual first content location before the neutral fixture/solver stages prove the collision representation. Do not turn this bridge into ECS, networking, sharding, a general rigid-body engine or automatic regional simulation LOD work.

Acceptance for the full bridge:

> The controlled actor's authoritative movement and collision are decided by Godot-free Simulation state/rules, Godot renders ordered samples smoothly by `EntityId`, and local presentation state cannot create an authoritative location outcome.

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
