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
ControlledActorMoveIntent
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

The controlled locomotion path is now executable end-to-end:

```text
PlayerControls camera-relative analog intent
  -> SimFacade.controlled_actor_submit_move_intent(x, z)
  -> protocol/session intent state
  -> SimFacade.advance_locomotion_tick()
  -> Simulation::advance_locomotion_tick()
  -> World::advance_grounded_locomotion_tick(actor-keyed batch)
  -> authoritative movement/collision transition
  -> AuthoritativeMovementSampleBatch
  -> GDExtension unit/DTO translation
  -> WorldPresentation
```

Submitting movement intent alone is not a world mutation. The fixed locomotion tick is the authoritative time/state transition.

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

## Transition results: ordered movement samples

Continuous authoritative movement is not modeled as repeated polling of `ControlledActorSpatialProjection`.

A successful locomotion tick returns one `AuthoritativeMovementSampleBatch`:

```text
tick
revision
protocol_version
samples[] sorted ascending by EntityId
  entity_id
  position mm X/Y/Z
  velocity mm/s X/Y/Z
  SpatialEpoch
```

The batch is the post-transition result of one atomic World locomotion tick. Every sample shares the batch tick/revision. Sample order is canonical and does not inherit the order in which player/NPC intent producers were collected.

Across locomotion batches, `SimulationTick` provides movement-time order. `WorldRevision` locates that batch relative to other authoritative world mutations. Do not add a second sequence counter until a concrete transport/presentation requirement proves that tick + revision are insufficient.

Protocol **v5** makes this transition-result contract Godot-facing through `SimFacade.advance_locomotion_tick()`. `ControlledActorSpatialProjection` remains useful for initial/debug reads; it is not the continuous frame stream.

## Identity and presentation replicas

Godot presentation objects are keyed by Simulation `EntityId` through one owner:

```text
ObservedWorldProjection
  -> WorldPresentation
       -> EntityId -> EntityBinding -> presentation root
```

Do not create another `EntityId -> Node` registry in combat, HUD, inventory or NPC scripts.

The first Milestone 1 slice binds the pre-existing controlled `Player` and materializes observed non-controlled actor IDs through `npc_presentation.tscn`. The NPC shell starts hidden and receives no authoritative transform until its first movement sample. Additional item/NPC presentation kinds wait for real capabilities rather than a universal scene factory.

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

The controlled actor and the first living-need NPC are exact-spatial because their current shared locomotion causality needs it. ADR 0005 governs when other entities require identity-resolved exact spatial detail.

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

- `SimulationTick` — authoritative world-time context and locomotion-batch time order;
- `WorldRevision` — monotonic authoritative mutation order across movement and non-movement changes;
- `SpatialEpoch` — per-actor continuity identity for movement interpolation.

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

Godot's interpolation contract requires resetting physics interpolation after initial placement or a discontinuous relocation.

## Continuous movement — implemented bridge

The active movement path is:

```text
PlayerControls semantic movement intent + Core NPC need decision
        ↓
protocol v5 collects actor-keyed movement intents
        ↓
Godot-free Simulation movement/collision solver
        ↓
ordered AuthoritativeMovementSampleBatch
        ↓
GDExtension mm/mm-s -> meter/m-s translation
        ↓
WorldPresentation batch validation
        ↓
physics-root authoritative transform/velocity update
        ↓
Godot physics interpolation for same-epoch rendering
```

`WorldPresentation` validates the complete sample shape before presentation application. The controlled stream requires:

- matching protocol version;
- exactly the next locomotion tick;
- a strictly newer world revision;
- samples strictly ascending by positive `EntityId`;
- every sample to belong to the observed set;
- valid position/velocity vectors and positive `SpatialEpoch`;
- one controlled-actor sample.

A same-epoch controlled sample updates the physics-root position/velocity during `_physics_process()`. A changed `SpatialEpoch` is a discontinuity: the new authoritative position is applied and interpolation is reset rather than blending across the relocation.

The controlled `ThirdPersonPlayer` no longer calls `move_and_slide()` or computes local gravity, acceleration/deceleration or sprint displacement. Its physics-root position therefore cannot create a competing location outcome. The visual child may rotate toward authoritative velocity because facing is still presentation-only.

The implemented solver uses neutral Simulation-owned acceptance geometry. Production content-location geometry/query representation is still deliberately unresolved; choose it from a concrete terrain/reachability requirement. Do not make Godot Physics/Jolt, `StaticBody3D`, a navmesh or `CharacterBody3D` authoritative by convenience.

## Smooth presentation

Simulation locomotion and the current local Godot client both run at the project-owned **60 Hz** fixed baseline. The in-process client advances one authoritative locomotion tick from each Godot physics tick, so the current bridge deliberately uses Godot's enabled physics interpolation for same-epoch transform smoothing instead of inventing another wall-clock/jitter buffer.

If a future transport decouples authoritative sample arrival from local physics ticks, re-admit that timing model and introduce explicit custom interpolation/jitter buffering only then. Do not assume the current in-process timing contract is a networking architecture.

If `SpatialEpoch` changes, do not interpolate from the previous epoch. Snap/reset the representation and clear incompatible interpolation history.

Prediction is optional. If later playtests justify it:

- predict from the same semantic intent sent to Simulation;
- keep predicted state separate from authoritative samples;
- reconcile to authoritative state;
- never let prediction grant damage, ownership, purchases, access or another systemic success.

## Revision ordering

`WorldPresentation` rejects an observed-world projection older than its last applied revision.

For controlled continuous movement it additionally requires the exact next locomotion tick and a revision newer than both the previous controlled spatial sample and the latest applied world revision. Duplicate/stale movement batches are rejected before they can move the presentation.

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
- projections/events/ordered sample batches ↔ Dictionaries/Arrays/typed Godot values;
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
controlled_actor_submit_move_intent(x, z)
advance_locomotion_tick()
bootstrap_submit_move(...)      # smoke only
bootstrap_debug_projection()    # smoke/debug only
```

Protocol v5 was introduced when the movement intent/batch surface became client-facing. A future breaking DTO/semantic change must update the shared protocol version and affected native/Godot evidence together.

## Extension checklist

For a feature crossing the boundary, answer:

1. Which authoritative entities participate?
2. What semantic intent is attempted?
3. Which Simulation state/rules decide it?
4. What result/error is returned?
5. Which events explain the consequence?
6. Which purpose-built projection or transition-result stream is needed?
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
- universal multi-kind Godot entity-scene factory;
- production content-location collision/index representation;
- authoritative facing/orientation;
- authoritative sprint/acceleration/deceleration semantics;
- automatic origin rebasing;
- local prediction/rollback framework.
