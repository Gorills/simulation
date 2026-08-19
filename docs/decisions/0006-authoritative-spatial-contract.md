# ADR 0006: Authoritative spatial contract

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../MODELING.md`](../MODELING.md) · [`../engineering/simulation-godot-boundary.md`](../engineering/simulation-godot-boundary.md) · [`../models/spatial-location.md`](../models/spatial-location.md)

## Context

Simulation is authoritative for entity existence and location, while the current Godot `CharacterBody3D` still moves locally as a presentation/prediction shell. The bootstrap grid probe cannot become the production third-person movement model.

The project also needs to preserve ADR 0005's adaptive causal fidelity: an offscreen or aggregate-resolved entity does not automatically need an exact centimeter-by-centimeter pose merely because it exists.

A durable spatial contract must therefore answer these questions before a collision/navigation solver is chosen:

- what coordinate/unit representation belongs to Simulation;
- which entities need exact spatial state;
- what the protocol sends to Godot;
- how ordered samples relate to `SimulationTick` / `WorldRevision`;
- how discontinuous relocations differ from ordinary motion;
- where presentation interpolation/prediction stops being world authority.

## Decision

### Simulation spatial coordinates use signed 64-bit millimeters

Exact Simulation positions and velocities use integer metric units:

```text
SpatialPosition        = signed int64 millimeters on X/Y/Z
SpatialVelocity        = signed int64 millimeters per second on X/Y/Z
```

Coordinate orientation matches Godot's world convention:

- X = horizontal side axis;
- Y = up/down;
- Z = horizontal front/back axis;
- right-handed 3D coordinate system.

Godot 4.7 uses the metric system with one 3D unit equal to one meter. The GDExtension adapter therefore translates authoritative millimeters to Godot meter-space `Vector3` values at the boundary; Godot units do not leak into `src/sim`.

Millimeter integers are chosen because they provide explicit deterministic units and ample world-space range without binding Simulation to Godot's current single-precision `Vector3` representation.

This does **not** imply every mechanic requires millimeter precision. Semantic places, contained/equipped/in-transit item locations, aggregate populations and other non-spatial forms remain separate domain concepts.

### Exact spatial state is selective

`SpatialState` is optional actor state.

An actor needs exact spatial state when current authoritative causality requires an identity-resolved 3D pose, for example:

- the controlled actor in an active 3D area;
- a materialized NPC participating in movement/combat/interactions where exact reachability matters;
- a moving object whose trajectory has current gameplay consequences.

An entity may continue to exist without exact spatial state when current mechanics only require semantic/aggregate location.

Therefore:

```text
entity exists != entity has exact spatial state != Godot node exists
```

Do not invent exact coordinates for every distant person merely to fill a uniform component table.

### Spatial state currently contains position, velocity and epoch

The first durable state is deliberately small:

```text
SpatialState
  position
  velocity
  spatial_epoch
```

Orientation is **not** added yet. Facing/rotation becomes authoritative only when the first real movement/combat mechanic proves which orientation semantics it needs. Do not pre-encode Euler/quaternion policy without that requirement.

### SpatialEpoch marks discontinuous relocation

`SpatialEpoch` starts positive and changes whenever an authoritative relocation must not be interpolated through, such as:

- teleportation;
- respawn;
- discontinuous world/scene-space transfer;
- another future movement capability explicitly defined as a discontinuity.

Samples with different epochs must not be blended into ordinary movement. Presentation snaps/resets its interpolation state.

This mirrors the general interpolation rule documented by Godot: after initial placement or teleport, set the transform and reset physics interpolation so the renderer does not smooth through an invalid path.

### Tick, revision and epoch have different meanings

Keep all three concepts separate:

```text
SimulationTick  = world-time sample/step context
WorldRevision   = authoritative state ordering
SpatialEpoch    = continuity domain for interpolation
```

An immediate non-spatial command can advance `WorldRevision` without changing `SimulationTick` or spatial position. A teleport can change epoch. Ordinary continuous motion stays within one epoch.

Do not use render frame number, Godot physics frame number or wall-clock timestamps as substitutes.

### Protocol exposes purpose-built spatial samples

The first read model is `ControlledActorSpatialProjection`.

It carries:

- authoritative `EntityId`;
- position in millimeters;
- velocity in millimeters/second;
- `SpatialEpoch`;
- `SimulationTick`;
- `WorldRevision`;
- protocol version.

GDExtension translates the integer metric values into Godot meter-space `Vector3` values. Godot does not receive mutable `SpatialState` or a world-state pointer.

The current PR uses this projection only for initial controlled-actor placement and evidence. Continuous authoritative movement is a later step.

### Movement writes are intent, never client-authored final state

The future movement boundary will accept semantic intent such as desired planar direction/strength and movement mode/gait as required by the real solver.

It must **not** accept general-purpose state setters such as:

```text
SetPosition
SetVelocity
SetTransform
ClientMoveToFinalTransform
```

The authoritative solver decides the resulting position/velocity after movement constraints, collision, terrain and world rules.

The exact command shape is deliberately deferred until the solver exists so this ADR does not invent an API that later mechanics must work around.

### Authoritative collision/navigation stays Godot-free

Any collision, terrain, navigation or reachability fact that changes authoritative outcomes must ultimately be represented in Simulation-owned neutral data and evaluated by Godot-free Simulation code.

The current Godot `CharacterBody3D`, `StaticBody3D`, collision shapes and `move_and_slide()` remain presentation/prototyping behavior until a corresponding authoritative solver/content contract exists.

This ADR does **not** select:

- Godot Physics/Jolt as Simulation authority;
- Recast or another navigation library;
- a custom physics engine;
- navmesh as the universal world representation;
- voxel/grid/heightfield/triangle-mesh collision representation.

Those choices require the first real playable terrain/collision requirement and deterministic native tests.

### Godot presentation may smooth or predict, never decide world truth

Godot is allowed to:

- buffer authoritative spatial samples;
- interpolate between compatible samples;
- snap/reset when `SpatialEpoch` changes;
- locally predict the controlled actor if measured playtest latency justifies it;
- reconcile prediction to authoritative samples.

Godot prediction cannot grant systemic results such as damage, purchase success, item ownership, access, death or relationship changes.

For samples produced on a clock that does not coincide with local Godot physics ticks, use a deliberate sample interpolation strategy instead of assuming built-in physics interpolation is automatically correct. Godot's own interpolation documentation identifies externally timed/server data as a case where custom interpolation can be a better fit.

### Godot world-origin precision is a presentation concern

Simulation's integer millimeter coordinates are not constrained to the precision range of a local Godot `Vector3`.

Godot 4.7 documents decreasing single-precision physics/rendering accuracy far from the origin and recommends enabling large-world coordinates only when actually needed. For a third-person game, the documented single-precision range is often adequate for ordinary open-world scales.

The project therefore does **not** enable large-world coordinates or origin rebasing now. If real playable geography exceeds acceptable local precision, Godot can introduce presentation-space rebasing/region transforms without changing Simulation's authoritative global coordinates.

## Consequences

### Positive

- authoritative world position gains explicit units and identity/order semantics;
- the client no longer needs to define the Simulation coordinate system;
- offscreen/fidelity policy remains compatible with exact 3D gameplay;
- future teleportation/magic has an explicit discontinuity contract;
- large-world presentation precision can change independently later;
- the bootstrap grid cannot silently become production location.

### Costs

- a real movement/collision solver is still required before third-person motion becomes authoritative;
- presentation needs explicit sample buffering/interpolation rather than blindly copying `CharacterBody3D` state back into Simulation;
- future mechanics that require orientation must add a reviewed orientation contract;
- any presentation-space rebasing must preserve the Simulation↔Godot conversion boundary.

## Deliberately not introduced

This decision does not introduce:

- continuous movement commands;
- authoritative collision/navigation solver;
- authoritative rotation/facing;
- jump/fall/swim/fly modes;
- prediction/reconciliation implementation;
- NPC materialization factory;
- dynamic spatial LOD;
- Godot large-world-coordinate builds;
- origin shifting;
- networking.

The next bounded spatial implementation should add one deterministic movement/collision vertical slice against neutral Simulation-owned environment data, then drive the existing player presentation from resulting ordered samples.
