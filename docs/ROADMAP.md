# Development roadmap

This roadmap describes **product/build milestones**, not AI Layer Work/Task/Epic state. Current repository source and executable evidence remain authoritative.

## Current state

The repository is greenfield: architecture, product/modeling/verification contracts and stack guidance exist, but the runtime CMake/Godot implementation has not landed yet.

The next implementation milestone is **Milestone 0 — Toolchain & Playable Spine**.

## Milestone rule

Every milestone becomes playable. Simulation-only depth may not silently accumulate several milestones ahead of the player experience.

Capability acceptance follows [`VERIFICATION.md`](VERIFICATION.md).

## Milestone 0 — Toolchain & Playable Spine

Deliver the smallest real native-to-Godot path:

- explicit local bootstrap for missing build tools;
- C++23 native `sim_core`;
- CMake/Ninja presets and CTest;
- a Godot 4.7 project using the exact engine baseline from [`engineering/VERSIONS.md`](engineering/VERSIONS.md);
- an immutable godot-cpp v10 revision targeting API 4.7, verified by build/load rather than prose;
- one protocol command/result/projection round-trip;
- InputMap movement that produces an authoritative command;
- one deterministic native test;
- one read-only debug projection;
- one bounded playtest entry/supervisor;
- one screenshot/debug artifact proving the player-visible state came from C++ rather than GDScript truth.

Acceptance:

> The game opens in the pinned Godot environment, the player can move, and evidence proves the movement/state round-trip through the authoritative C++ core.

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
