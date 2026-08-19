# Development roadmap

This roadmap describes **product/build milestones**, not AI Layer Work/Task/Epic state. Current repository source and executable evidence remain authoritative.

## Current state

Milestone 0's executable spine is present in source: CMake presets, separated `sim_core`/`sim_protocol` targets, deterministic tests, pinned dependencies, the GDExtension adapter, a Godot 4.7 third-person presentation client, keyboard/mouse + gamepad controls, project-wide UI design system, collision-aware camera, and a bounded smoke-playtest supervisor.

The architecture foundation now also establishes stable `EntityId`, actor parity, a protocol-owned human-control binding, and separate `SimulationTick` / `WorldRevision`. ADR 0004 makes Simulation authoritative for entity existence/location and systemic world state; Godot is a presentation replica that will consume authoritative projections/samples.

The current `CharacterBody3D` motor remains a responsive presentation/prediction shell during that migration. It is not authoritative world location.

**Milestone 0 is not accepted merely because the files exist.** Acceptance still requires the applicable local build/Godot evidence in [`VERIFICATION.md`](VERIFICATION.md).

Capability acceptance follows [`VERIFICATION.md`](VERIFICATION.md).

## Milestone rule

Every milestone becomes playable. Simulation-only depth may not silently accumulate several milestones ahead of the player experience.

Systemic milestones must also preserve actor parity: a human-controlled actor enters the same world-rule path as an equivalent NPC rather than gaining a parallel player-only implementation.

## Milestone 0 — Toolchain & Playable Spine

Implemented path:

- explicit bootstrap in `tools/bootstrap.py`;
- C++23 native `sim_core` plus `sim_protocol` with one-way protocol -> domain dependency;
- CMake/Ninja presets and CTest;
- Godot 4.7 project using the baseline from [`engineering/VERSIONS.md`](engineering/VERSIONS.md);
- immutable GoogleTest and godot-cpp revisions in `cmake/Dependencies.cmake`;
- one native move command/result/projection round-trip retained as a small protocol/GDExtension smoke probe;
- stable actor `EntityId` and protocol-side controlled-actor binding for that probe;
- `SimulationTick` separated from `WorldRevision` so immediate actions do not pretend world time advanced;
- a real `CharacterBody3D` third-person **presentation/prediction** shell for the reference client;
- semantic InputMap actions for WASD/left-stick movement, mouse/right-stick camera and keyboard/gamepad sprint;
- separate tuneable control/camera and presentation-locomotion profiles;
- camera-relative analog presentation response with explicit acceleration/deceleration/direction-change and turn response;
- `SpringArm3D` camera collision;
- deterministic native domain/protocol tests;
- a read-only native actor projection exposed in the Godot debug surface;
- bounded `tools/play.py --scenario smoke` ownership/timeouts/artifacts;
- smoke assertions for `debug.json` plus `final.png`;
- a project-wide Godot UI Theme/design-system foundation at a 1920×1080 logical baseline.

The native grid-step probe is **not** the third-person locomotion contract. The production path must replace local-authority assumptions with semantic movement intent -> authoritative Simulation spatial result -> revisioned Godot presentation samples/interpolation. See ADR 0004 and [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md).

Pending/ongoing acceptance evidence remains defined by [`VERIFICATION.md`](VERIFICATION.md).

Acceptance:

> The game opens in the pinned Godot environment; the player can control the third-person presentation responsively with keyboard/mouse and gamepad; and separate smoke evidence proves the Godot -> GDExtension -> protocol -> C++ authoritative round-trip.

## Pre-Milestone 1 authority bridge

Before adding deep economy/social/politics/combat systems, complete the minimum production bridge needed to stop using the Godot transform as de facto actor location:

1. define the real authoritative spatial/location representation from actual terrain/navigation/determinism requirements;
2. send semantic controlled-actor movement intent through protocol;
3. produce ordered/revisioned authoritative movement/location samples;
4. materialize/update Godot representation by `EntityId`;
5. interpolate samples smoothly and add local prediction/reconciliation only if measured playtest need justifies it;
6. prove that leaving/rematerializing a representation does not create/delete the Simulation entity.

Do not turn this bridge into ECS, networking, sharding or regional simulation LOD work. It exists to establish one source of world truth before systemic features compound around the wrong owner.

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
- a non-magical alternative;
- an economic/social/institutional downstream effect.

Acceptance:

> Magic changes the authoritative system trajectory, not only presentation.

## Milestone 6 — Emergent Role

Enough compositional state exists that any suitably situated actor—including the human-controlled one—can occupy materially different roles such as worker, trader, bandit, apprentice or office-holder through world paths.

Acceptance:

> Roles emerge from state/rules without an authoritative hardcoded player class.

## Milestone 7 — Persistence & Repeated Play

- save/load;
- stable entity identities preserved;
- replay/determinism evidence;
- a substantial repeated session;
- gameplay fixes based on actual play.

Acceptance:

> The authoritative world preserves understandable consequences across sessions and the game supports continued play.

## Roadmap discipline

Do not insert a large subsystem milestone because its architecture is interesting. A new milestone must identify the new player-visible capability, the minimum causal world rule, the protocol/projection experience change and acceptance evidence.

Do not optimize the unseen world with regional LOD/sharding merely because Godot materializes only a subset. Presentation materialization and Simulation fidelity are separate concerns; optimize the latter only from profiling evidence.

Consequential roadmap changes should update product/modeling docs or an ADR when they change a durable contract.
