# Model: first living need

Status: IMPLEMENTED — interactive-condition slice, Milestone 1 still in progress

## Purpose

Establish the smallest causal NPC behavior that is more than locomotion acceptance: an identity-resolved actor has authoritative need state, that state produces a concrete task intention, the task uses the already shared movement law, and Godot presents the resulting authoritative NPC rather than inventing one.

The current slice proves **need -> task -> travel -> shared-world interference -> satisfied location condition**. It does not claim the full Milestone 1 acceptance yet; offscreen continuation and a bounded real playtest remain.

## First need: assigned rest place

`RestNeedState` belongs to `ActorState` and contains:

- assigned local rest X coordinate in integer millimeters;
- assigned local rest Z coordinate in integer millimeters;
- caller/content-owned per-axis arrival tolerance in integer millimeters.

The NPC reaches the assigned place when its authoritative `SpatialState.position` is within that X/Z tolerance. Satisfaction is then derived from two authoritative facts:

1. the NPC is inside its assigned rest tolerance;
2. no **other** actor with exact `SpatialState` is inside that same X/Z tolerance.

There is no stored `is_satisfied` or `is_blocked` flag. Current world state determines the answer on every decision. An authoritative actor without exact spatial state does not occupy a microscopic local rest point merely by existing elsewhere in the simulation.

This is intentionally a first causal fixture, not a complete fatigue/sleep/home system. The model does **not** yet include fatigue accumulation, sleep duration, health effects, beds, household ownership, schedules, time-of-day preference or multiple rest opportunities.

The assigned local point also does not establish a production semantic home/location model. A later real location/content requirement may replace this acceptance representation with a place/entity reference plus reachability/access rules.

## Causal decision

`decide_npc_rest_need(world, actor)` is Core-owned, deterministic and read-only.

It:

1. requires a valid existing actor with `RestNeedState` and exact `SpatialState`;
2. reads only authoritative World state;
3. delegates local X/Z steering to the existing `decide_npc_local_move_toward_waypoint()` primitive;
4. chooses semantic `LocomotionPace::walk` for this ordinary rest travel task;
5. returns the existing actor-generic `ActorGroundedMoveIntent` shape;
6. reports `satisfied=true` only when the NPC is inside the assigned tolerance and no other exact-spatial actor occupies that same tolerance area;
7. reports `blocked_by_other_actor=true` when the NPC has reached the assigned tolerance but another exact-spatial actor currently occupies it.

A blocked decision emits zero movement just like an actor already at its local waypoint, but remains unsatisfied. This deliberately makes availability a current causal condition rather than a latched completion flag. If the other actor leaves through ordinary authoritative movement, the next read-only decision can become satisfied without a special unblock command.

The decision does **not** choose a numeric speed. `World` resolves the actor's current walking limit, acceleration and braking from authoritative locomotion capability. It also does not mutate position, choose collision outcomes, access Godot state, perform pathfinding, generate a schedule or introduce an NPC-specific movement law.

## Shared player interference/help

The first player-effect seam requires no new player-only action.

The controlled actor is an ordinary authoritative actor with exact spatial state and already moves through the same `World::advance_grounded_locomotion_tick()` law as NPCs. Therefore:

- entering the NPC's assigned rest tolerance can obstruct satisfaction;
- leaving that tolerance can remove the obstruction and allow satisfaction;
- the same causal rule applies if any other exact-spatial actor occupies or leaves the place.

Core exposes only a bounded read-only presence query for this concrete condition. It does not expose the actor container, create actor-body collision, introduce a reservation manager, or turn the current acceptance point into a generic place framework.

Occupancy is checked as a satisfaction condition once the NPC itself has reached its assigned tolerance. The NPC does not gain omniscient long-range avoidance behavior from this rule and no navigation/pathfinding semantics are implied.

## Application scenario

The first Milestone 1 application scenario contains two identity-resolved actors:

- `EntityId{1}` — the human-controlled actor at the origin;
- `EntityId{2}` — the rest-need NPC at `(3000, 0, -3000)` mm.

The NPC's assigned rest point remains `(-3000, -3000)` mm with a 150 mm per-axis tolerance.

Every application locomotion tick:

1. samples/stores the human-controlled semantic movement direction + pace;
2. asks Core for the NPC rest-need decision;
3. submits both actor-keyed intents to one `World::advance_grounded_locomotion_tick()` batch;
4. lets `World` resolve each actor's own locomotion capability before the shared solver;
5. receives one canonical `EntityId`-ordered authoritative movement sample batch.

Because occupancy derives from World spatial state, the existing controlled movement path is already sufficient to change the need outcome; no protocol version or Godot mutation API is added for this slice.

The current default actor capability uses a **1000 mm/s walk** ceiling with **6000 mm/s² acceleration** and **8000 mm/s² braking**. These are first project feel baselines, not biological constants. The previous 5800 mm/s value remains only the current `sprint` ceiling; RestNeed travel no longer uses it.

In the unblocked deterministic acceptance scenario the NPC accelerates into ordinary walking, enters the configured rest tolerance, then the need decision emits zero directional intent and authoritative braking settles the actor at `x=-2912 mm` on tick **364**. That final position is 88 mm from the target X and therefore still inside the 150 mm tolerance. No hidden arrival flag, client speed, or Godot collision result participates.

## Persistence

`RestNeedState` changes future authoritative decisions, so it remains snapshot truth.

The occupancy/blocking result is **not** persisted separately. It is derived from the current exact-spatial state of actors after restore, so a stored blocker flag could only duplicate and drift from authoritative location truth.

The rest need itself was introduced with snapshot schema v3. The current `WorldSnapshot` schema remains v4 because actor locomotion capability and expanded fixed-step continuation remainders also affect future movement. This interference slice adds no new persistent state and therefore requires no snapshot schema change.

No disk/save migration policy is introduced; that remains Milestone 7 work.

## Observation and Godot materialization

`ObservedWorldProjection` remains identity/presence-only and currently observes both actors in the first application scenario. The observation DTO is unchanged by rest-place occupancy.

`WorldPresentation` materializes non-controlled observed IDs using the generic NPC presentation shell. All bound observed actors use the same authoritative movement sample ordering/epoch validation. Godot never copies the NPC presentation transform back into Simulation and never decides whether the rest place is occupied.

The next Milestone 1 slice must prove that this need/task continues when the NPC presentation is absent; presentation lifetime must not become a hidden prerequisite for the occupancy or movement rule.

## Current evidence

Native evidence covers:

- deterministic rest-need decision from authoritative state;
- semantic `walk` pace selected by the need/task rather than a numeric speed;
- satisfaction from authoritative position when the assigned tolerance is free;
- an exact-spatial second actor inside the same tolerance blocks satisfaction without mutating World during decision evaluation;
- the blocking actor can leave through the ordinary shared locomotion law and the next decision becomes satisfiable without a special player/NPC command;
- snapshot/restore preservation of persistent rest-need and locomotion state while occupancy remains derived;
- human and NPC movement entering the same actor-generic World transition.

Godot smoke continues to prove the existing Simulation -> protocol -> GDExtension -> presentation movement boundary. A dedicated bounded playtest of the interference/help loop remains required before Milestone 1 acceptance.

## Deliberately unresolved

This first need does not yet provide:

- fatigue growth or recurring need activation;
- an authoritative rest process over simulation time;
- alternate rest sites or option evaluation;
- actor-body collision or exclusive physical reservation semantics;
- pathfinding/navigation around real location obstacles;
- offscreen acceptance evidence for the need loop;
- dedicated need-status UI or NPC animation/facing/identity UI;
- general-purpose need/task/planner abstractions;
- stamina, carried-load, wound, progression or magic formulas for locomotion capability.

Those locomotion factors now have one authoritative resolution seam in `World`, but their effects must be implemented only when a real mechanic introduces the corresponding state.

## Next bounded task

Prove **offscreen continuation**: the authoritative RestNeed decision and movement must continue while the NPC has no Godot representation, without adding a generic scheduler or making presentation observation control world existence.
