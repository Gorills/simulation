# Model: first living need

Status: IMPLEMENTED — causal/offscreen slices complete, Milestone 1 still in progress

## Purpose

Establish the smallest causal NPC behavior that is more than locomotion acceptance: an identity-resolved actor has authoritative need state, that state produces a concrete task intention, the task uses the shared movement law, other actors can change the outcome through the same world, and presentation lifetime is not a prerequisite for causal progress.

The implemented slices now prove **need -> task -> travel -> shared-world interference -> offscreen continuation -> rematerialization**. Milestone 1 still needs player-visible need outcome feedback plus one bounded real-client interference/help scenario before acceptance.

## First need: assigned rest place

`RestNeedState` belongs to `ActorState` and contains an assigned local X/Z rest point plus a caller/content-owned per-axis arrival tolerance in integer millimeters.

The NPC is eligible to satisfy the need when its authoritative `SpatialState.position` is inside that X/Z tolerance. Satisfaction is derived from two authoritative facts:

1. the NPC is inside its assigned rest tolerance;
2. no **other** actor with exact `SpatialState` is inside the same tolerance.

There is no stored `is_satisfied` or `is_blocked` flag. Current World state determines the result on every decision. An authoritative actor without exact spatial state does not occupy a microscopic local rest point merely by existing elsewhere in the simulation.

This is a first causal fixture, not a complete fatigue/sleep/home system. It does not yet model fatigue accumulation, sleep duration, health effects, beds, household ownership, schedules, time-of-day preference or alternate rest opportunities.

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

## Application scenario

The bounded Milestone 1 acceptance composition currently uses:

- `EntityId{1}` — human-controlled actor;
- `EntityId{2}` — the rest-need NPC.

This two-actor composition remains acceptance scaffolding under ADR 0009; it must not scale by accumulating more named IDs and feature branches.

Each application locomotion tick collects human control and the Core RestNeed decision into the same actor-keyed World batch. World resolves each actor's current locomotion capability and returns one authoritative ordered movement result.

Because occupancy derives from World spatial state, existing controlled locomotion already changes the need outcome; no player-only mutation API is required.

## Offscreen continuation and materialization

Authoritative existence, observation and Godot materialization are separate.

The bounded `offscreen` Godot scenario proves this sequence:

1. the NPC is observed/materialized and receives an authoritative movement sample;
2. `WorldPresentation` removes only the NPC presentation binding/node while the entity remains observed and authoritative;
3. another Simulation locomotion tick runs the same RestNeed decision and moves the NPC while no NPC Godot node exists;
4. the authoritative movement batch is still accepted because the sample belongs to an observed entity, even though there is no presentation transform to update;
5. a fresh observed-world projection creates a new hidden NPC shell;
6. the next authoritative movement sample supplies its current position/velocity/continuity state and makes the shell visible.

A missing non-controlled Godot node therefore cannot stop, delete or author the NPC's causal state. The controlled actor remains materialized in this local-client scenario, and samples for entities outside the observed set remain invalid.

This slice deliberately does **not** introduce a scheduler, regional simulation system, semantic travel abstraction or time acceleration. The existing RestNeed/locomotion causal path already advances without a presentation node, so no broader offscreen infrastructure is justified yet.

## Persistence

`RestNeedState`, actor locomotion capability and fixed-step continuation remain snapshot truth because they affect future authoritative decisions/results.

Satisfaction, blocking and presentation materialization are not persisted as duplicate flags. They are derived from authoritative world state or presentation policy after restore. This slice adds no new persistent state and requires no snapshot schema change.

## Current evidence

Native evidence covers deterministic RestNeed decision, semantic walk intent, derived satisfaction/blocking, shared actor interference through ordinary locomotion, and snapshot/restore of the causal state.

Godot boundary evidence now additionally proves that a living-need NPC continues authoritative travel while its presentation node is absent and rematerializes from fresh observation plus a later authoritative sample.

The remaining M1 gap is **experience exposure**: blocked and satisfied are both currently visually stationary outcomes. The real client therefore needs a purpose-built read-only need-status projection/feedback and one bounded scenario in which the controlled actor obstructs the place, the client observes `blocked`, the actor leaves through ordinary movement, and the client observes `satisfied`.

## Deliberately unresolved

This first need does not yet provide:

- fatigue growth or recurring activation;
- an authoritative rest-duration process;
- alternate rest sites or option evaluation;
- actor-body collision or exclusive reservation semantics;
- pathfinding/navigation around real obstacles;
- generic offscreen scheduler/time acceleration;
- generic needs/tasks/planner abstractions;
- stamina/load/wound/progression/magic locomotion formulas.

Add those only when a concrete later capability requires them.

## Next bounded task

Expose the derived RestNeed outcome through a purpose-built read-only protocol/Godot status and run a bounded real-client **interference/help** scenario that proves `traveling -> blocked -> satisfied` through the existing shared movement law.
