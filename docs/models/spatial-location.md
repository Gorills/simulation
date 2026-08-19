# Model: Spatial location

Status: ACCEPTED

## Gameplay purpose

Provide one authoritative spatial truth for identity-resolved 3D movement, reachability and later spatial interactions while allowing Godot to render smoothly and allowing non-relevant world detail to remain semantic/aggregate rather than microsimulated.

The immediate player-facing purpose is narrower: the controlled actor has an authoritative Simulation spawn position that crosses the protocol/GDExtension boundary and initializes the existing third-person presentation.

## Observable patterns / fit-for-purpose criteria

The model is fit for the current stage when:

- a controlled actor has one Simulation-owned exact position independent of its Godot node;
- the Godot presentation initializes from that position rather than inventing its spawn transform;
- bootstrap grid motion cannot change the production spatial position;
- actors can exist without exact 3D state when current causality does not require it;
- a discontinuous relocation can be distinguished from continuous motion;
- protocol samples carry enough ordering metadata for future interpolation/reconciliation;
- no Godot physics object becomes authoritative world state.

The next solver stage will add movement/collision criteria.

## Historical baseline and region

There is no circa-1200 numeric locomotion law encoded at this stage. Human walking/running performance, roads, terrain, loads, horses, vehicles and travel-time assumptions require narrower research when those mechanics become causal.

The historical baseline still constrains semantic geography and future travel systems; this contract only defines exact local 3D representation.

## Causal model

```text
Simulation entity
  -> optional exact SpatialState when exact local geometry is causal
       position (int64 mm)
       velocity (int64 mm/s)
       spatial epoch
       tick/revision context
  -> purpose-built protocol sample
  -> GDExtension mm -> Godot meters
  -> WorldPresentation
  -> visual/predicted Node3D representation
```

The causal arrow does not run backwards from `Node3D.global_position` to authoritative state.

## Entities / state / scales

### Identity

Spatial state belongs to an existing `EntityId`; it never creates identity.

### Exact position

`SpatialPosition` uses signed 64-bit millimeters on X/Y/Z.

Axes match Godot's right-handed 3D convention: X side, Y up/down, Z front/back. This minimizes adapter ambiguity while keeping Godot types out of Simulation.

### Velocity

`SpatialVelocity` uses signed 64-bit millimeters per second.

Velocity is included now because ordered presentation samples and the coming movement solver need a derivative/state-of-motion value. It does not imply Godot may author authoritative velocity.

### Spatial epoch

`SpatialEpoch` is a positive continuity identifier. Ordinary continuous motion remains in one epoch. A teleport/respawn/discontinuous transfer changes the epoch so presentation knows not to interpolate through the impossible path.

### Time and revision

`SimulationTick` records world-time context. `WorldRevision` records authoritative mutation order. Neither is replaced by render frame count or Godot physics frame count.

## Fidelity / representation level

Exact `SpatialState` is **identity-resolved state**, but it is optional.

Use it when current mechanics require exact 3D causality. Do not assign it to every actor merely because every real person is physically somewhere.

A non-spatially-resolved actor can still have authoritative semantic location such as settlement/household/in-transit context once those models exist. Godot materialization and exact Simulation spatial resolution remain separate concerns.

The controlled actor in an active third-person area is exact-spatial by definition.

## Conservation and promotion-demotion invariants

When a future mechanic promotes an entity from semantic/aggregate location to exact spatial state:

- keep the same `EntityId`;
- preserve ownership, inventory, health, relationships, obligations and history;
- choose an exact spawn consistent with its authoritative semantic location and already-observed facts;
- do not invent a convenient position that contradicts prior events;
- create/change `SpatialEpoch` so presentation does not interpolate from an unrelated previous space.

When exact spatial state is no longer required, removing it must not delete the entity or erase causal state outside the spatial model.

No generic promotion/demotion engine is implemented yet.

## Magic sensitivity surface

Spatial assumptions can be changed by future magic involving:

- teleportation or portals;
- flight/levitation;
- phasing through ordinary collision;
- altered gravity;
- transformations that change body size/shape;
- magically created/destroyed obstacles;
- impossible-speed movement;
- spatial distortion or non-Euclidean connections;
- detection/knowledge that changes whether exact location is knowable to an observer.

The contract already supports discontinuous relocation through `SpatialEpoch`. It does **not** pre-implement the other effects.

A concrete magical capability must alter the real authoritative movement/collision/location mechanism rather than setting a presentation transform or applying a generic magic multiplier.

## Implemented magic deviations

None.

## Inputs

Current stage:

- Simulation/content initialization supplies the controlled actor's initial exact spatial state.

Future continuous movement input must be a semantic intent (desired movement direction/strength/mode as the real solver requires), not a final transform/position setter.

## Transitions / scheduling

Current production spatial state is static after spawn; the Milestone 0 grid step deliberately mutates only bootstrap state.

The next stage will define deterministic continuous movement/collision transitions and their relation to `SimulationTick`.

No authoritative spatial rule depends on Godot `_process`, `_physics_process`, rendered FPS or wall-clock time.

## Outputs / consequences

Current output:

`ControlledActorSpatialProjection` exposes:

- `EntityId`;
- position in mm;
- velocity in mm/s;
- `SpatialEpoch`;
- `SimulationTick`;
- `WorldRevision`;
- protocol version.

GDExtension converts position/velocity to Godot meter-space `Vector3` values for presentation.

Future consequences such as reachability, collision, interaction range, fall state, entry into a place or combat positioning must be computed from Simulation-owned spatial/environment rules before being exposed.

## Player-facing exposure

`WorldPresentation` applies the first authoritative sample as initial placement of the controlled presentation and calls `reset_physics_interpolation()` after positioning.

This function is intentionally **initialization-only**. Continuous samples require a buffer/interpolator so future weak models do not implement movement by teleporting the node every time a projection arrives.

The current local `ThirdPersonPlayer` motor can still move the representation for control feel during migration, but its resulting transform remains non-authoritative until the movement solver/sample loop replaces it.

## Uncertainty

Not yet decided:

- authoritative movement tick frequency;
- body/capsule representation;
- static/dynamic collision representation;
- slope/step/grounding semantics;
- navigation representation;
- authoritative facing/orientation;
- jump/fall/swim/fly models;
- local prediction policy;
- sample interpolation delay/buffer size;
- world-origin rebasing policy.

These are deliberately deferred to measured/vertical requirements rather than guessed now.

## Simplifications

- only the controlled actor receives a production spatial projection today;
- initial position is the world origin with zero velocity;
- no authoritative motion is performed yet;
- Godot demo collision remains presentation/prototype collision only;
- orientation is omitted until a mechanic needs authoritative facing.

## Deliberately not simulated

- detailed gait/biomechanics;
- foot placement;
- animation/root motion as world authority;
- every offscreen actor's precise 3D pose;
- long-distance travel routes/times;
- mount/vehicle physics;
- full rigid-body physics;
- magic spatial effects beyond reserving continuity semantics.

## Sources

Engineering facts:

- Godot 4.7, [Introduction to 3D](https://docs.godotengine.org/en/4.7/tutorials/3d/introduction_to_3d.html): metric 3D scale, 1 unit = 1 meter, Y-up right-handed coordinates.
- Godot 4.7, [Large world coordinates](https://docs.godotengine.org/en/4.7/tutorials/physics/large_world_coordinates.html): single-precision world-space limits, large-world-coordinate trade-offs, and origin-shifting alternative.
- Godot 4.7, [Physics interpolation quick start](https://docs.godotengine.org/en/4.7/tutorials/physics/interpolation/physics_interpolation_quick_start_guide.html): set initial/teleport transform then reset physics interpolation.
- Godot, [Physics interpolation introduction](https://docs.godotengine.org/en/stable/tutorials/physics/interpolation/physics_interpolation_introduction.html): fixed-timestep interpolation and custom interpolation for externally timed/server-style samples.
- Epic Games, [Networked Movement in Character Movement Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine): production precedent separating movement input, authoritative evaluation, corrections and presentation smoothing. This is conceptual evidence only; Unreal is not a dependency.

Historical human locomotion/travel sources will be added when their numeric assumptions become gameplay rules.

## Falsifiers

Revise this contract if:

- millimeter integer coordinates cannot represent a required authoritative mechanic efficiently or correctly;
- a real movement/collision mechanic proves velocity or epoch semantics insufficient;
- world geometry requires a coordinate topology that cannot be expressed as one global Cartesian position;
- presentation cannot preserve required precision without a revised adapter/rebasing contract;
- a magic capability requires a continuity model richer than epoch-based discontinuity;
- authoritative orientation becomes necessary and the chosen orientation model changes the state/sample contract.
