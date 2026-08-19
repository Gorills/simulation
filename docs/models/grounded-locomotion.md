# Model: Grounded locomotion acceptance

Status: ACCEPTED

## Purpose

Define the **minimum observable contract** that the first authoritative third-person locomotion solver and its neutral test arena must satisfy.

This document deliberately specifies outcomes before implementation. It prevents the first demo scene, Godot `CharacterBody3D`, or a convenient physics API from silently defining Simulation movement semantics.

It is not the solver design and it does not choose body shape, collision library, tick rate, numeric slope/step thresholds, acceleration values, or navigation technology.

Related contracts:

- [`spatial-location.md`](spatial-location.md)
- [`../decisions/0006-authoritative-spatial-contract.md`](../decisions/0006-authoritative-spatial-contract.md)
- [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- [`../MODELING.md`](../MODELING.md)

## Admission result

The locomotion/collision acceptance contract is `READY`.

Implementing the full solver before this contract would be premature because the accepted spatial model intentionally leaves body representation, grounding, slope/step semantics, movement cadence and collision representation unresolved.

The next solver change may choose those details only insofar as they are required to satisfy this accepted observable contract.

## Authoritative invariants

1. **Simulation decides movement.** Human or NPC code supplies intent; the solver produces authoritative position/velocity/state.
2. **Godot does not submit final transforms.** No `SetPosition`, `SetVelocity` or copy-back from `CharacterBody3D` is a production movement path.
3. **Player/NPC parity holds.** The same movement transition must accept intent for any equivalent exact-spatial actor; human control is only one intent source.
4. **The environment is Simulation-owned data.** A Godot scene may visualize the fixture, but Godot collision objects are not authoritative inputs to native tests.
5. **Fixed input + initial state + environment = repeatable result.** Deterministic native replay is required before presentation feel is considered accepted.
6. **Continuous movement stays in one `SpatialEpoch`.** A normal collision, slope transition, step or fall must not masquerade as teleportation.
7. **Exact spatial fidelity remains selective.** This contract applies to actors whose local 3D position is currently causally relevant; it does not require microscopic poses for every offscreen actor.

## First movement scope

The first authoritative solver models ordinary **grounded humanoid locomotion** over static local geometry.

The first slice needs only enough state to distinguish:

- supported/grounded motion;
- unsupported/falling motion;
- blocking versus traversable surfaces;
- continuous position/velocity updates.

Do not create a universal movement-mode framework merely to reserve future features.

### Explicitly out of scope

- jumping;
- swimming;
- climbing/mantling;
- ladders;
- mounts/vehicles;
- flight;
- moving platforms;
- pushing dynamic rigid bodies;
- ragdolls;
- root-motion authority;
- NPC pathfinding/navigation;
- local prediction/reconciliation;
- networking;
- magic locomotion implementations.

Those may extend or supersede this baseline when a real mechanic requires them.

## Input semantics

The first production movement command must express **intent**, not outcome.

At minimum the future solver needs a planar desired movement direction/strength for one authoritative step. A gait/sprint semantic may be included only if that same slice makes it authoritative.

The command must not contain a client-authored final position, transform, collision result or grounded flag.

The exact protocol DTO is deliberately deferred to the solver PR so the API is derived from the real transition rather than invented here.

## Required observable behaviors

### 1. Flat-ground movement

Given supported initial state on a flat walkable surface and non-zero planar intent:

- the actor advances continuously in the intended horizontal direction;
- authoritative velocity reflects the motion;
- the actor remains supported;
- no vertical drift is introduced by resting on a flat plane.

With zero intent after movement, the actor converges to the solver's defined resting state without positional jitter or perpetual micro-motion.

This contract does not yet prescribe acceleration/deceleration numbers.

### 2. Head-on blocking wall

Given intent directly into a blocking near-vertical surface:

- the actor must not pass through the surface;
- the final authoritative pose must remain non-penetrating according to the chosen body/contact model;
- repeated identical input must not accumulate penetration or instability.

### 3. Oblique wall motion

Given movement with both into-wall and tangential components:

- the blocking component is constrained;
- valid tangential motion remains possible;
- the actor does not stick merely because desired motion touches a wall obliquely.

The precise slide algorithm is implementation detail; the observable requirement is preserved valid tangential progress without penetration.

### 4. Walkable versus unwalkable slope

The solver must expose an explicit authoritative distinction between a slope considered ground and one too steep for ordinary grounded ascent.

The acceptance arena contains a **paired slope fixture**:

- one ramp strictly inside the configured walkable threshold;
- one ramp strictly outside it.

Expected outcomes:

- the inside-threshold ramp can be traversed while remaining grounded;
- the outside-threshold ramp cannot be ascended as ordinary ground;
- standing still on supported ordinary ground does not create unintended downhill creep unless a future reviewed movement rule deliberately chooses that behavior.

No numeric angle is chosen by this document.

### 5. Traversable versus blocking step

The solver must expose an explicit maximum ordinary step-up capability rather than treating every vertical discontinuity as either a wall or a teleport.

The acceptance arena contains a **paired step fixture**:

- one obstacle strictly below the configured ordinary step-up threshold;
- one obstacle strictly above it.

Expected outcomes:

- the lower fixture can be crossed without jump/teleport semantics;
- the higher fixture blocks ordinary grounded movement;
- step handling remains continuous in the current `SpatialEpoch`.

No numeric step height is chosen here.

### 6. Ledge, fall and landing

The arena contains a walkable platform ending in a drop to a lower walkable surface.

When intent carries the actor beyond support:

- grounded support is lost;
- authoritative vertical motion becomes falling motion under the solver's explicit gravity rule;
- the actor cannot remain suspended because the presentation still thinks it is on a floor;
- contact with the lower walkable surface produces a stable grounded state;
- landing remains continuous and does not change `SpatialEpoch`.

The exact gravity magnitude and fall-damage rules are outside this stage.

### 7. Deterministic replay

For every fixture above, the native test suite must be able to run:

```text
same initial world state
+ same neutral collision fixture
+ same ordered intent sequence
+ same fixed simulation step configuration
=
exact same authoritative samples/results
```

Determinism evidence belongs in Godot-free tests. A visually similar Godot run is complementary playtest evidence, not the source of truth.

## Neutral acceptance-arena geometry

The first arena is a **test fixture**, not a content/location design.

It contains only the minimum static geometry needed to falsify the movement rules:

```text
start pad / flat lane
wall lane:
  head-on wall
  oblique wall approach
slope lane:
  below-threshold ramp
  above-threshold ramp
step lane:
  below-threshold step
  above-threshold step
ledge lane:
  supported platform
  drop
  lower landing plane
```

### Geometry rules

- fixture dimensions are expressed in the Simulation spatial unit contract (integer millimeters);
- geometry is created from neutral native data, not read from Godot nodes at runtime;
- the paired threshold fixtures must sit on opposite sides of the actual solver configuration, with a deliberate non-zero margin;
- fixture generation must not duplicate the solver's classification code — tests provide geometry and assert outcomes;
- presentation geometry may be derived from the same authored fixture values later, but presentation is not authoritative collision input.

The next implementation must choose the smallest collision representation that can express these fixtures cleanly and deterministically. Do not generalize to arbitrary world meshes until a real location requires them.

## Parameters deliberately not chosen yet

The solver PR must make the following concrete only when implementation/tests require them:

- movement simulation cadence;
- character/body collision representation and dimensions;
- contact tolerance/skin semantics;
- walkable-slope threshold;
- ordinary step-up threshold;
- grounding/snap tolerance;
- gravity magnitude;
- grounded speed/acceleration/deceleration values needed by authoritative motion;
- maximum solver iterations or equivalent bounded collision-resolution rule.

Defaults from Godot, Unreal or Unity are **references, not project values**. Any selected numeric value becomes repository-owned and should be justified by gameplay scale, stability tests and playtest rather than copied because another engine ships it.

## Godot presentation contract

During this stage Godot remains a presentation shell.

Once continuous authoritative samples exist, Godot must:

- consume ordered `EntityId`-keyed spatial samples;
- visually smooth compatible samples without changing Simulation truth;
- reset/snap only across real `SpatialEpoch` discontinuities;
- not decide whether a wall, slope, step or landing was authoritative;
- keep camera/animation feel separate from native movement correctness.

The existing local `ThirdPersonPlayer` motor may remain temporarily useful for feel during migration, but it must not become a second world-law implementation.

## Magic sensitivity surface

This is the non-magical baseline, but the model must remain extensible when magic changes real constraints.

Examples:

- flight removes the ordinary support requirement rather than toggling a visual animation;
- levitation/altered gravity changes the authoritative gravity/up/support rules;
- phasing changes which collision constraints apply;
- body transformation can change collision dimensions/clearance;
- supernatural speed changes movement limits while still resolving authoritative collision;
- teleportation is a discontinuity and therefore changes `SpatialEpoch` instead of pretending to be extreme continuous velocity.

Do not implement these effects now and do not reserve a generic `MagicMovementMode` hierarchy. A concrete magic capability must modify the real movement constraints when it becomes causal.

## Source basis

The acceptance categories are based on established character-controller contracts, not copied implementations:

- **Godot 4.7 `CharacterBody3D`** — <https://docs.godotengine.org/en/4.7/classes/class_characterbody3d.html>: script-driven character body; grounded mode; floor/wall classification; `floor_max_angle`; floor snapping; wall sliding; explicit floor state and slide collision behavior.
- **Epic Games Unreal Engine `UCharacterMovementComponent`** — <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent> and <https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine>: separate walking/falling behavior, explicit walkable-floor semantics, maximum step height, bounded simulation iterations, authoritative movement evaluation and presentation smoothing as production precedents.
- **Unity 6 Character Controller** — <https://docs.unity3d.com/6000.0/Documentation/Manual/class-CharacterController.html>: independent `Slope Limit`, `Step Offset` and contact/skin concepts as corroborating evidence that slope traversal, step traversal and collision tolerance are distinct tuning/solver concerns.

These sources justify the **questions and observable categories**. Their default values and engine-specific algorithms are not imported into Simulation.

## Acceptance for this contract

This modeling stage is complete when a future agent can implement the first solver without inventing what the test arena is supposed to prove.

The **next bounded task** after this document is accepted is:

> choose the smallest neutral static collision representation and the minimum grounded-body/solver parameters needed to implement the fixtures above, then implement flat/wall movement as the first deterministic native vertical slice.

Do not build the visual first location before that native slice proves the collision representation.

## Falsifiers

Revise this contract if a real third-person gameplay requirement shows that:

- grounded/wall/slope/step/fall categories are insufficient for the first playable movement slice;
- the chosen body/contact model cannot express stable wall or ground behavior without different observable semantics;
- a required magical capability invalidates the assumption that ordinary support/gravity is the correct non-magical baseline;
- deterministic native evaluation is impossible with the chosen environment representation;
- player/NPC parity requires materially different world-rule inputs rather than different intent sources.