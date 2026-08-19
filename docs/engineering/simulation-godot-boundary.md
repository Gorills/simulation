# Simulation ↔ Godot boundary

This guide is the implementation route for ADR 0004 and the authoritative spatial decision in [`../decisions/0006-authoritative-spatial-contract.md`](../decisions/0006-authoritative-spatial-contract.md).

Use it when adding a mechanic that crosses from C++ Simulation into Godot presentation or from Godot input/UI into an authoritative world action.

## The shortest correct mental model

```text
Godot sends intent.
Simulation decides what happened.
Godot renders the result.
```

Godot may make presentation smoother. It may not invent authoritative state.

## Actor parity

A human-controlled actor and an NPC are simulated actors with stable `EntityId`s.

```text
PlayerControls -> protocol control binding --+
                                           |
NPC decision -------------------------------+-> same domain action path
```

Do not add `is_player` branches to movement, economy, relationships, institutions, inventory, ownership, damage or other world laws.

## Write path: intent into world change

A Godot feature that wants to change the world submits semantic intent through `SimFacade`/protocol.

Good:

```text
BuyItem
OfferTrade
Attack
Travel
GiveGift
future: DesiredMovementIntent
```

Bad:

```text
SetMoney
SetInventory
SetRelationship
SetPosition
SetVelocity
SetTransform
```

The client may describe what an actor attempts. Simulation computes the resulting world state.

## Read path: purpose-built projections

Godot reads purpose-built projections, not domain objects or mutable `WorldState`.

Implemented durable reads:

### ObservedWorldProjection

```text
controlled_actor_id
SimulationTick
WorldRevision
protocol_version
entities: [{EntityId}, ...]
```

This projection answers identity/presence only. It deliberately contains no bootstrap grid coordinates and no exact 3D pose.

### ControlledActorSpatialProjection

```text
EntityId
position: signed int64 millimeters X/Y/Z
velocity: signed int64 millimeters/second X/Y/Z
SpatialEpoch
SimulationTick
WorldRevision
protocol_version
```

The protocol retains explicit integer metric units. GDExtension translates position/velocity to Godot meter-space `Vector3` values.

Do not add hidden state or reconstruct domain meaning in the adapter.

Future feature projections such as shops, inventory, relationships or institutions should appear only when real mechanics need them. Do not create a universal `WorldProjection`.

## Identity and presentation replicas

Godot presentation objects are keyed by Simulation `EntityId` through one owner:

```text
ObservedWorldProjection
  -> WorldPresentation
       -> EntityId -> EntityBinding -> presentation root
```

Do not create another `EntityId -> Node` registry in combat, HUD, inventory or NPC scripts.

The current stage binds only the pre-existing controlled `Player`. Generic NPC/item scene factories wait for real second/third presentation kinds.

## Exact spatial state and materialization are different

Keep these states independent:

```text
authoritative entity existence
semantic location
optional exact SpatialState
knowledge/observation
Godot materialization
visual frustum/occlusion
```

An entity can exist and remain causally important without exact 3D state. A materialized entity normally needs enough projected state to render, but Godot materialization itself does not create world existence.

The current controlled actor is exact-spatial because active third-person movement needs it. ADR 0005 governs when other entities require identity-resolved exact spatial detail.

## Units and coordinates

Simulation exact spatial units:

```text
position = signed int64 millimeters
velocity = signed int64 millimeters / second
X/Z horizontal, Y up
right-handed orientation aligned with Godot
```

Godot 4.7 uses one 3D unit = one meter. Convert only in GDExtension.

Never include Godot `Vector3`, `Transform3D`, `CharacterBody3D`, physics RIDs or scene paths in `src/sim`/`src/protocol`.

Simulation's integer coordinates also remain independent of Godot's floating-point large-world precision. Do not enable Godot large-world coordinates or origin shifting until real playable scale requires it.

## Tick, revision and spatial continuity

These are different contracts:

- `SimulationTick` — authoritative world-time context;
- `WorldRevision` — monotonic authoritative mutation order;
- `SpatialEpoch` — continuity identity for movement interpolation.

A non-spatial command can increase revision while position stays unchanged. A teleport/respawn/discontinuous transfer changes epoch. Ordinary continuous movement stays within an epoch.

Godot must not replace any of these values with render frames, physics frames or wall-clock time.

## Initial placement

Current executable flow:

```text
SimFacade.observed_world_projection()
  -> WorldPresentation binds EntityId

SimFacade.controlled_actor_spatial_projection()
  -> WorldPresentation.initialize_controlled_spatial_presentation()
       -> validates matching EntityId/tick/revision/protocol
       -> sets meter-space position
       -> reset_physics_interpolation()
```

This API is **initial placement only**. Reusing it every authoritative update would turn continuous movement into repeated teleports and is forbidden by its contract.

Godot's official interpolation guidance requires resetting physics interpolation after initial placement/teleport to avoid interpolating from an invalid previous transform.

## Continuous movement — next stage

The target path is:

```text
PlayerControls semantic movement intent
        ↓
protocol movement intent
        ↓
Godot-free Simulation movement/collision solver
        ↓
ordered authoritative spatial samples
        ↓
Godot sample buffer
        ↓
interpolation
        ↓
optional local prediction + reconciliation if measured need exists
```

The current `ThirdPersonPlayer.move_and_slide()` path remains presentation/prototype movement. Do **not** copy its resulting `global_position`/velocity back into Simulation.

The first real solver must choose neutral Simulation-owned environment/collision data from a concrete terrain/reachability requirement. Do not make Godot Physics/Jolt, `StaticBody3D`, a navmesh or `CharacterBody3D` authoritative by convenience.

## Smooth presentation

Simulation tick rate and render rate are different clocks.

Presentation may interpolate compatible authoritative samples. If samples arrive on timing boundaries that do not match local Godot physics ticks, evaluate deliberate custom sample interpolation rather than forcing them through an unrelated clock. Godot's own interpolation guidance calls out externally timed/server-style samples as a case where custom interpolation may fit better.

If `SpatialEpoch` changes, do not interpolate from the previous epoch. Snap/reset the representation and clear incompatible sample history.

Prediction is optional. If later playtests justify it:

- predict from the same semantic intent sent to Simulation;
- keep predicted state separate from authoritative samples;
- reconcile to authoritative state;
- never let prediction grant damage, ownership, purchases, access or another systemic success.

## Revision ordering

`WorldPresentation` rejects an observed-world projection older than its last applied revision.

Future continuous spatial buffers must also order samples explicitly by authoritative tick/revision and reject impossible regressions/duplicates according to their own contract.

This is an ordering guard, not networking architecture or rollback.

## Trading example ownership

| Concern | Owner |
| --- | --- |
| merchant exists/location | Simulation |
| stock/price/money | Simulation |
| whether trade is permitted | Simulation |
| transaction result | Simulation |
| shop layout/focus | Godot UI/design system |
| transfer animation/sound | Godot presentation |

## Bandit attack example ownership

| Concern | Owner |
| --- | --- |
| attackers/targets and authoritative position | Simulation |
| attack intent/hostility/legality | Simulation |
| collision/reachability once implemented | Simulation |
| wounds/death/loot/consequences | Simulation |
| actor scene nodes | Godot presentation |
| animation/VFX/audio/camera | Godot presentation |

Offscreen attacks do not need Godot nodes to resolve.

## GDExtension adapter rule

Allowed:

- Godot primitives ↔ protocol DTOs;
- explicit unit conversion such as millimeters ↔ meters;
- enums/errors ↔ Godot-friendly values;
- projections/events ↔ Dictionaries/Arrays/typed Godot values;
- diagnostics.

Not allowed:

- collision/movement resolution;
- prices/trade decisions;
- relationship changes;
- authoritative spawning;
- ownership/damage decisions;
- using scene transforms as world truth.

Current surface:

```text
observed_world_projection()
controlled_actor_spatial_projection()
bootstrap_submit_move(...)      # smoke only
bootstrap_debug_projection()    # smoke/debug only
```

## Extension checklist

For a feature crossing the boundary, answer:

1. Which authoritative entities participate?
2. What semantic intent is attempted?
3. Which Simulation state/rules decide it?
4. What result/error is returned?
5. Which events explain the consequence?
6. Which purpose-built projection is needed?
7. Which fields are actually observable?
8. Which parts are presentation-only?
9. Can an NPC use the same world capability?
10. What deterministic native test proves the transition?
11. What bounded Godot playtest proves presentation of the result?

If the answer begins with “Godot sets the world state”, stop and move the decision behind protocol.

## Deliberately not introduced yet

- generic ECS;
- networking/server infrastructure;
- full event sourcing;
- regional runtime LOD/sharding;
- multithread world jobs;
- universal projection/event bus;
- generic Godot entity-scene factory;
- authoritative continuous movement/collision solver;
- authoritative facing/orientation;
- automatic origin rebasing;
- prediction/reconciliation framework.
