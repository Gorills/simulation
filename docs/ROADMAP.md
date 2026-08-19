# Development roadmap

This roadmap describes **product/build milestones**, not AI Layer Work/Task/Epic state. Current repository source and executable evidence remain authoritative.

## Current state

Milestone 0's executable spine is now present in source: CMake presets, separated `sim_core`/`sim_protocol` targets, deterministic GoogleTest coverage, pinned dependencies, the GDExtension adapter, a Godot 4.7 third-person client, keyboard/mouse + gamepad InputMap controls, collision-aware camera, and a bounded smoke-playtest supervisor.

**Milestone 0 is not accepted merely because the files exist.** Acceptance still requires a complete pinned dependency build, Godot 4.7.1 extension load, real third-person control playtesting, and the smoke artifact round-trip described below.

Capability acceptance follows [`VERIFICATION.md`](VERIFICATION.md).

## Milestone rule

Every milestone becomes playable. Simulation-only depth may not silently accumulate several milestones ahead of the player experience.

## Milestone 0 — Toolchain & Playable Spine

Implemented path:

- explicit bootstrap in `tools/bootstrap.py`;
- C++23 native `sim_core` plus `sim_protocol` with one-way protocol -> domain dependency;
- CMake/Ninja presets and CTest;
- Godot 4.7 project using the baseline from [`engineering/VERSIONS.md`](engineering/VERSIONS.md);
- immutable GoogleTest and godot-cpp revisions in `cmake/Dependencies.cmake`;
- one native move command/result/projection round-trip retained as a small protocol/GDExtension smoke probe;
- a real `CharacterBody3D` third-person locomotion shell for the reference client;
- semantic InputMap actions for WASD/left-stick movement, mouse/right-stick camera and keyboard/gamepad sprint;
- separate tuneable control/camera and locomotion profiles;
- camera-relative analog movement with explicit acceleration/deceleration and turn response;
- `SpringArm3D` camera collision and physics-interpolated target following;
- deterministic native domain/protocol tests;
- a read-only native projection exposed in the Godot debug surface;
- bounded `tools/play.py --scenario smoke` ownership/timeouts/artifacts;
- smoke assertions for `debug.json` plus `final.png`.

The native grid-step probe is **not** the third-person locomotion contract. Godot owns the immediate local kinematic shell; systemic world outcomes remain native. Semantic location/interaction boundaries become explicit protocol capabilities when gameplay requires them.

Pending acceptance evidence:

1. acquire the pinned dependencies in a network-enabled local environment;
2. build and run CTest through the normal presets;
3. build the GDExtension against the pinned godot-cpp revision targeting API 4.7;
4. load it in the pinned Godot 4.7.1 engine;
5. play the third-person client with keyboard/mouse and with a gamepad, verifying movement response, full analog range, look direction/inversion defaults, sprint, slope/collision behavior and SpringArm camera collision;
6. run `tools/play.py --scenario smoke` and retain the generated native projection/screenshot evidence.

Acceptance:

> The game opens in the pinned Godot environment; the player can move and control the third-person camera responsively with keyboard/mouse and gamepad; and separate smoke evidence proves the Godot -> GDExtension -> protocol -> C++ round-trip.

## Milestone 1 — Living Need

- one NPC need;
- one causal task;
- travel/action;
- visible result;
- player can interfere or help.

Acceptance:

> An NPC acts because of world state, and the player can change the outcome.

## Milestone 2 — Household Resource Loop

- household stock;
- production and consumption;
- shortage;
- player trade/gift/work response.

Acceptance:

> The village can develop a problem without the player, and the player can address it through more than one real path.

## Milestone 3 — Social Consequence

- one useful trust/obligation/reputation dimension;
- interaction changes a future opportunity;
- an NPC remembers a relevant event.

Acceptance:

> A later action becomes available, unavailable or differently costly because of earlier behavior.

## Milestone 4 — First Institution / Politics

- one office or authority;
- one real permission/obligation;
- one way to gain, lose or influence it;
- a visible distributional consequence.

Acceptance:

> The player can participate in power through world state rather than a quest flag.

## Milestone 5 — First Magic Counterfactual

- one magic capability;
- explicit access and cost;
- a non-magical alternative;
- an economic/social/institutional downstream effect.

Acceptance:

> Magic changes the system trajectory, not only presentation.

## Milestone 6 — Emergent Role

Enough compositional state exists that the player can become at least two materially different roles such as worker, trader, apprentice or office-holder through different world paths.

Acceptance:

> Roles emerge from state/rules without an authoritative hardcoded player class.

## Milestone 7 — Persistence & Repeated Play

- save/load;
- replay/determinism evidence;
- a substantial repeated session;
- gameplay fixes based on actual play.

Acceptance:

> The world preserves understandable consequences and the game supports continued play.

## Roadmap discipline

Do not insert a large subsystem milestone because its architecture is interesting. A new milestone must identify the new player-visible capability, the minimum causal world rule, the protocol/experience change and acceptance evidence.

Consequential roadmap changes should update product/modeling docs or an ADR when they change a durable contract.
