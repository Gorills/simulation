# Model: first living need

Status: IMPLEMENTED — Milestone 1 acceptance candidate

## Purpose

Establish the smallest complete causal NPC behavior that is more than locomotion acceptance: an identity-resolved actor has authoritative need state, that state produces a concrete task intention, the task uses the shared movement law, other actors can change the outcome through the same world, presentation lifetime is not a prerequisite for causal progress, and the real client exposes the derived outcome without owning it.

The implemented vertical loop is **need -> task -> travel -> shared-world interference -> offscreen continuation -> read-only client outcome -> bounded interference/help evidence**.

This document may describe the candidate as complete because the branch is not accepted into `main` unless the exact-revision native and Godot evidence passes. [`../VERIFICATION.md`](../VERIFICATION.md) owns that evidence rule.

## First need: assigned rest place

`RestNeedState` belongs to `ActorState` and contains an assigned local X/Z rest point plus a caller/content-owned per-axis arrival tolerance in integer millimeters.

The NPC is eligible to satisfy the need when its authoritative `SpatialState.position` is inside that X/Z tolerance. Satisfaction is derived from two authoritative facts:

1. the NPC is inside its assigned rest tolerance;
2. no **other** actor with exact `SpatialState` is inside the same tolerance.

There is no stored `is_satisfied` or `is_blocked` flag. Current World state determines the result on every decision. An authoritative actor without exact spatial state does not occupy a microscopic local rest point merely by existing elsewhere in the simulation.

This is a first causal fixture, not a complete fatigue/sleep/home system. It does not model fatigue accumulation, sleep duration, health effects, beds, household ownership, schedules, time-of-day preference or alternate rest opportunities.

## Causal decision

`decide_npc_rest_need(world, actor)` is Core-owned, deterministic and read-only.

It requires an existing actor with valid `RestNeedState` and exact `SpatialState`, delegates local steering to `decide_npc_local_move_toward_waypoint()`, chooses semantic `LocomotionPace::walk`, and returns the existing actor-generic movement intent.

It reports:

- `satisfied=true` only when the NPC is inside the assigned tolerance and no other exact-spatial actor occupies it;
- `blocked_by_other_actor=true` when the NPC has reached the tolerance but another exact-spatial actor currently occupies it;
- ordinary travel intent while the NPC is still approaching the point.

A blocked decision emits zero movement but remains unsatisfied. If the other actor leaves through ordinary authoritative movement, the next read-only decision can become satisfied without an unblock command or stored reservation state.

The decision does not choose numeric movement speed, mutate position, resolve collision, access Godot, perform pathfinding, generate a schedule or introduce an NPC-specific movement law.

## Shared player interference/help

The controlled actor is an ordinary authoritative exact-spatial actor and already moves through `World::advance_grounded_locomotion_tick()`. Therefore entering the NPC's assigned tolerance can obstruct satisfaction, and leaving it can remove the obstruction. The same rule applies to any other exact-spatial actor.

Core exposes only the bounded read-only presence query needed by this condition. It does not expose actor storage, create actor-body collision, introduce a reservation/place manager or establish a production spatial index.

Occupancy is checked only once the NPC itself has reached its assigned tolerance. The rule does not grant long-range occupancy knowledge or pathfinding behavior.

## Application composition

The bounded Milestone 1 acceptance composition uses:

- `EntityId{1}` — human-controlled actor;
- `EntityId{2}` — the rest-need NPC.

This two-actor composition remains acceptance scaffolding under ADR 0009; it must not scale by accumulating more named IDs and feature branches.

Each application locomotion tick collects human control and the Core RestNeed decision into the same actor-keyed World batch. World resolves each actor's current locomotion capability and returns one authoritative ordered movement result.

Because occupancy derives from World spatial state, existing controlled locomotion changes the need outcome; there is no player-only mutation API.

## Read-only outcome projection

The client needs to distinguish the otherwise visually similar stationary outcomes `blocked` and `satisfied`, but it does not need the internal RestNeed state or mutation authority.

`LivingNeedProjection` is therefore a purpose-built read model containing only:

- the living-need NPC `EntityId`;
- derived status: `traveling`, `blocked` or `satisfied`;
- current `SimulationTick`;
- current `WorldRevision`;
- protocol version.

The projection is recomputed from the Core RestNeed decision on read. It does not persist a second need state, expose the target/tolerance, or create a generic task/need DTO family.

The GDExtension translates the enum into stable presentation strings and Godot renders a localized read-only HUD row. Godot does not calculate whether the place is occupied or whether the need is satisfied.

This projection is an additive read on protocol v6. Existing commands/results/projections keep their current shapes, so no incompatible protocol-version bump is introduced.

## Offscreen continuation and materialization

Authoritative existence, observation and Godot materialization are separate.

The bounded `offscreen` Godot scenario proves this sequence:

1. the NPC is observed/materialized and receives an authoritative movement sample;
2. `WorldPresentation` removes only the NPC presentation binding/node while the entity remains observed and authoritative;
3. another Simulation locomotion tick runs the same RestNeed decision and moves the NPC while no NPC Godot node exists;
4. the authoritative movement batch is accepted because the sample belongs to an observed entity, even though there is no presentation transform to update;
5. a fresh observed-world projection creates a new hidden NPC shell;
6. the next authoritative movement sample supplies its current position/velocity/continuity state and makes the shell visible.

A missing non-controlled Godot node therefore cannot stop, delete or author the NPC's causal state. The controlled actor remains materialized in this local-client scenario, and samples for entities outside the observed set remain invalid.

This does **not** introduce a scheduler, regional simulation system, semantic travel abstraction or time acceleration. The existing RestNeed/locomotion causal path already advances without a presentation node, so no broader offscreen infrastructure is justified yet.

## Bounded interference/help experience

The `rest_interference` scenario uses the real Godot -> GDExtension -> protocol -> Simulation path and does not add a scenario-only world setter.

It proves this causal sequence:

1. `LivingNeedProjection` begins `traveling`;
2. the controlled actor moves toward the assigned rest location through the same semantic movement command used by normal control;
3. when the NPC reaches the place while the controlled actor occupies it, Core derives `blocked`;
4. Godot displays the localized read-only blocked status and captures rendered evidence;
5. the controlled actor leaves through ordinary authoritative movement;
6. the next eligible RestNeed decision becomes `satisfied`;
7. Godot displays the localized satisfied status and the final evidence aligns need status, world tick/revision and authoritative movement presentation.

The scripted input is only bounded acceptance automation. It does not bypass the normal movement/world-rule path and does not claim subjective keyboard/gamepad feel.

## Persistence

`RestNeedState`, actor locomotion capability and fixed-step continuation remain snapshot truth because they affect future authoritative decisions/results.

Satisfaction, blocking, the read-only projection and presentation materialization are not persisted as duplicate flags. They are derived from authoritative world state or presentation policy after restore. The experience slice adds no persistent state and requires no snapshot schema change.

## Milestone 1 acceptance

Milestone 1 is accepted when the exact revision passes the required native/protocol and Godot scenarios proving:

- need-driven deterministic NPC intent from authoritative state;
- human/NPC parity through shared movement rules;
- another actor can change the need outcome through ordinary world movement;
- the causal path continues without an NPC Godot node;
- the client exposes authoritative `traveling` / `blocked` / `satisfied` status without owning the mechanic;
- the bounded real-client interference/help loop reaches `blocked` and then `satisfied`.

Acceptance means the first living-need vertical capability is complete. It does **not** imply the current rest-point fixture is a production needs/planning/home framework.

## Deliberately unresolved

This first need does not provide:

- fatigue growth or recurring activation;
- an authoritative rest-duration process;
- alternate rest sites or option evaluation;
- actor-body collision or exclusive reservation semantics;
- pathfinding/navigation around real obstacles;
- generic offscreen scheduler/time acceleration;
- generic needs/tasks/planner abstractions;
- stamina/load/wound/progression/magic locomotion formulas.

Add those only when a concrete later capability requires them.

## Next bounded work

After Milestone 1 acceptance, re-admit the first **Milestone 2 — Household Resource Loop** vertical slice from the current product/roadmap contracts rather than extending RestNeed infrastructure for hypothetical future needs.
