# Model: first living need

Status: IMPLEMENTED — first bounded Milestone 1 slice

## Purpose

Establish the smallest causal NPC behavior that is more than locomotion acceptance: an identity-resolved actor has authoritative need state, that state produces a concrete task intention, the task uses the already shared movement law, and Godot presents the resulting authoritative NPC rather than inventing one.

This slice deliberately proves **need -> task -> travel -> satisfied location condition** only. It does not claim the full Milestone 1 acceptance yet.

## First need: assigned rest place

`RestNeedState` belongs to `ActorState` and contains:

- assigned local rest X coordinate in integer millimeters;
- assigned local rest Z coordinate in integer millimeters;
- caller/content-owned per-axis arrival tolerance in integer millimeters.

The need is satisfied when the actor's authoritative `SpatialState.position` is within that X/Z tolerance. There is no separate `is_satisfied` Boolean: location truth itself determines whether the need is currently satisfied.

This is intentionally a first causal fixture, not a complete fatigue/sleep/home system. The model does **not** yet include fatigue accumulation, sleep duration, health effects, beds, household ownership, schedules, time-of-day preference or multiple rest opportunities.

The assigned local point also does not establish a production semantic home/location model. A later real location/content requirement may replace this acceptance representation with a place/entity reference plus reachability rules.

## Causal decision

`decide_npc_rest_need(world, actor)` is Core-owned, deterministic and read-only.

It:

1. requires a valid existing actor with `RestNeedState` and exact `SpatialState`;
2. reads only authoritative World state;
3. delegates local X/Z steering to the existing `decide_npc_local_move_toward_waypoint()` primitive;
4. returns the existing `ActorGroundedMoveIntent` shape;
5. reports `satisfied=true` only when that movement intent is zero because authoritative position is already within the configured rest-point tolerance.

The decision does not mutate position, choose collision outcomes, access Godot state, perform pathfinding, generate a schedule or introduce an NPC-specific movement law.

## Application scenario

The first Milestone 1 application scenario contains two identity-resolved actors:

- `EntityId{1}` — the human-controlled actor at the origin;
- `EntityId{2}` — the rest-need NPC at `(3000, 0, -3000)` mm.

The NPC's assigned rest point is `(-3000, -3000)` mm with a 150 mm per-axis tolerance.

Every application locomotion tick:

1. samples/stores the human-controlled semantic movement intent;
2. asks Core for the NPC rest-need decision;
3. submits both actor-keyed intents to one `World::advance_grounded_locomotion_tick()` batch;
4. receives one canonical `EntityId`-ordered authoritative movement sample batch.

At the current 5800 mm/s, 60 Hz acceptance baseline, the NPC reaches `x=-2896 mm` after 61 moving ticks. On the next decision the remaining 104 mm X distance is inside the 150 mm tolerance, so tick 62 emits zero NPC movement and the NPC remains stopped. No hidden arrival flag or Godot collision result participates.

## Persistence

`RestNeedState` changes future authoritative decisions, so it is snapshot truth.

`WorldSnapshot` schema **v3** adds the optional actor rest-need state and validates it on spawn/restore. Negative arrival tolerance is invalid. Snapshot/restore remains value-based and atomic on validation failure.

No disk/save migration policy is introduced; that remains Milestone 7 work.

## Observation and Godot materialization

`ObservedWorldProjection` remains identity/presence-only and now observes both actors in the first application scenario. Its DTO shape and protocol v5 do not change.

`WorldPresentation` materializes non-controlled observed IDs using the generic `npc_presentation.tscn` shell. The shell starts hidden at no assumed authoritative position. It becomes visible only when the first authoritative movement sample for that `EntityId` arrives; that sample supplies position, velocity and `SpatialEpoch`.

All bound observed actors use the same sample ordering/epoch validation. Godot never copies the NPC presentation transform back into Simulation.

## Current evidence

Native evidence covers:

- deterministic rest-need decision from authoritative state;
- zero intent when authoritative position already satisfies the configured tolerance;
- no World mutation from decision evaluation;
- snapshot/restore preservation of rest-need state;
- rejection of malformed rest-need state without partial mutation;
- human + NPC intents in the same protocol/World locomotion batch;
- exact deterministic NPC arrival/stopping behavior.

Godot smoke evidence for this slice must additionally prove:

- observed IDs are `[1, 2]`;
- bound/visible IDs become `[1, 2]` only through authoritative observation/sample application;
- the first NPC sample is `(2.904, 0, -3.0)` meters with `(-5.8, 0, 0)` m/s velocity;
- duplicate movement batches remain rejected;
- ru/en HUD rendering remains intact.

## Deliberately unresolved

This first need does not yet provide:

- fatigue growth or recurring need activation;
- an authoritative rest process over simulation time;
- alternate rest sites or option evaluation;
- pathfinding/navigation around real location obstacles;
- player help/interference with the need outcome;
- offscreen acceptance evidence for the need loop;
- NPC animation/facing/identity UI;
- general-purpose need/task/planner abstractions.

Add those only when the next Milestone 1 acceptance slice requires them.

## Next bounded task

Make the first need **interactable through shared world rules**: introduce the minimum authoritative condition/action that lets the controlled actor help or obstruct the NPC's rest outcome, while preserving offscreen Simulation existence and without putting need/task truth in Godot.
