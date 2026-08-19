# Model: Grounded locomotion acceptance

Status: ACCEPTED

## Purpose

Define the **minimum observable contract** that authoritative third-person grounded locomotion and its neutral test arena must satisfy.

This document specifies outcomes before implementation so a Godot `CharacterBody3D`, demo scene, or convenient physics API cannot silently define Simulation movement semantics.

Related contracts:

- [`spatial-location.md`](spatial-location.md)
- [`../decisions/0006-authoritative-spatial-contract.md`](../decisions/0006-authoritative-spatial-contract.md)
- [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- [`../MODELING.md`](../MODELING.md)

## Admission result

The acceptance contract was the prerequisite for solver work. The first flat/wall implementation slice is now admitted and implemented; slope/step/fall and live protocol/presentation wiring remain separate bounded stages.

## Authoritative invariants

1. **Simulation decides movement.** Human or NPC code supplies intent; the solver produces authoritative position/velocity/state.
2. **Godot does not submit final transforms.** No `SetPosition`, `SetVelocity` or copy-back from `CharacterBody3D` is a production movement path.
3. **Player/NPC parity holds.** The same movement transition accepts intent for any equivalent exact-spatial actor; human control is only one intent source.
4. **The environment is Simulation-owned data.** A Godot scene may visualize the fixture, but Godot collision objects are not authoritative inputs to native tests.
5. **Fixed input + initial state + environment = repeatable result.** Deterministic native replay is required before presentation feel is considered accepted.
6. **Continuous movement stays in one `SpatialEpoch`.** A normal collision, slope transition, step or fall must not masquerade as teleportation.
7. **Exact spatial fidelity remains selective.** This contract applies to actors whose local 3D position is causally relevant; it does not require microscopic poses for every offscreen actor.

## First movement scope

The initial authoritative solver models ordinary **grounded humanoid locomotion** over static local geometry.

The full accepted baseline needs enough state to distinguish:

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

Production movement input expresses **intent**, not outcome.

`PlanarMoveIntent` is the first native form: signed analog X/Z components use a fixed scale of 1000 and must remain inside the unit circle. The transition derives displacement from authoritative configuration and fixed-step integration. Client-authored final position, transform, collision result or grounded flag are not inputs.

The protocol DTO is still deferred until the shared world transition is ready to enter the controlled-actor runtime path.

## Implemented flat/wall slice

The first native slice deliberately chooses the smallest representation needed to prove flat ground and walls.

### Neutral environment data

`GroundedEnvironment` currently contains:

- `GroundPatch` — bounded X/Z support with endpoint heights; equal endpoint heights are flat support, while unequal heights reserve a neutral ramp representation for the next slope slice;
- `VerticalBarrier` — a two-sided axis-aligned vertical blocker at one X or Z coordinate and a vertical range.

The current barrier intentionally spans its whole test lane. Finite wall endpoints/corners and arbitrary triangle meshes are not generalized before a real location requires them.

### Body and timing baseline

The first `UprightCapsule` uses:

- radius: **380 mm**;
- height: **1800 mm**.

The first `GroundedStepConfig` uses:

- **60 authoritative steps/second**;
- **5800 mm/s** full-strength planar move speed for the flat/wall fixture.

These values are repository migration baselines taken from the existing playable Godot shell (`CapsuleShape3D`, project physics rate, and locomotion profile). They are **not** Godot/Unreal/Unity defaults and are not treated as final feel tuning.

Acceleration/deceleration, sprint, slope response and turning/facing are not promoted into authoritative movement by this slice.

### Deterministic integer integration

Authoritative spatial state remains integer millimeters/mm-per-second. Fixed-step integration stores a small per-axis remainder so fractional millimeters are carried into later ticks instead of being truncated away every step.

With the current fixture, 5800 mm/s for 60 identical full-strength ticks therefore resolves to exactly 5800 mm of authoritative travel.

### Implemented outcomes

The native transition currently proves:

- stable flat support at the actor's current support height;
- exact fixed-step planar integration;
- zero-input rest without positional jitter;
- head-on wall blocking at capsule radius;
- repeated wall input without accumulating penetration;
- oblique wall intent preserving tangential motion while the normal component is blocked;
- deterministic replay from identical state/environment/intent;
- explicit rejection of invalid analog intent;
- explicit rejection of sloped support by the **current** transition rather than pretending slope behavior is implemented.

The transition is Godot-free and actor-neutral. It is not yet called from `World`/protocol, so the live Godot player still uses its local presentation motor while migration continues.

## Required observable behaviors

### 1. Flat-ground movement — first slice implemented

Given supported initial state on a flat walkable surface and non-zero planar intent:

- the actor advances continuously in the intended horizontal direction;
- authoritative velocity reflects the motion;
- the actor remains supported;
- no vertical drift is introduced by resting on a flat plane.

With zero intent, the actor reaches the current slice's defined resting state without positional jitter.

### 2. Head-on blocking wall — first slice implemented

Given intent directly into a blocking near-vertical surface:

- the actor does not pass through it;
- the final pose remains non-penetrating under the chosen capsule/barrier model;
- repeated identical input does not accumulate penetration.

### 3. Oblique wall motion — first slice implemented

Given movement with into-wall and tangential components:

- the blocking component is constrained;
- valid tangential motion remains possible;
- the actor does not stick merely because desired motion touches a wall obliquely.

### 4. Walkable versus unwalkable slope — pending

The solver must expose an explicit authoritative distinction between a slope considered ground and one too steep for ordinary grounded ascent.

The acceptance arena contains a paired slope fixture:

- one ramp strictly inside the configured walkable threshold;
- one ramp strictly outside it.

Expected outcomes:

- the inside-threshold ramp can be traversed while remaining grounded;
- the outside-threshold ramp cannot be ascended as ordinary ground;
- standing still on supported ordinary ground does not create unintended downhill creep unless a future reviewed rule deliberately chooses that behavior.

No numeric angle is accepted yet.

### 5. Traversable versus blocking step — pending

The solver must expose an explicit maximum ordinary step-up capability.

The acceptance arena contains:

- one obstacle strictly below the configured step-up threshold;
- one obstacle strictly above it.

The lower fixture is traversable without jump/teleport semantics; the higher fixture blocks ordinary grounded movement. No numeric step height is accepted yet.

### 6. Ledge, fall and landing — pending

When intent carries the actor beyond support:

- grounded support is lost;
- authoritative vertical motion becomes falling motion under an explicit gravity rule;
- presentation cannot keep the actor suspended;
- contact with a lower walkable surface produces stable grounded state;
- landing remains continuous and does not change `SpatialEpoch`.

Gravity magnitude and fall-damage rules remain outside the current slice.

### 7. Deterministic replay — first slice implemented for flat/wall

For each implemented fixture, native tests require:

```text
same initial world state
+ same neutral collision fixture
+ same ordered intent sequence
+ same fixed simulation step configuration
=
exact same authoritative result
```

As slope/step/fall become implemented, the same requirement extends to those fixtures.

## Neutral acceptance-arena geometry

The arena is a **test fixture**, not content/location design:

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

Geometry rules:

- dimensions use Simulation integer millimeters;
- authoritative geometry comes from neutral native data, never Godot nodes;
- paired threshold fixtures sit on opposite sides of actual solver configuration with deliberate margin;
- tests provide geometry and assert outcomes instead of duplicating classification code;
- Godot may later visualize the same authored fixture values, but presentation geometry is not authoritative collision input.

The current flat/wall representation must be extended only as the pending fixtures require. Do not generalize to arbitrary world meshes before a real location requires them.

## Parameters still deliberately unresolved

The next solver slices still need reviewed choices for:

- contact tolerance/skin semantics beyond the exact flat/wall capsule boundary;
- walkable-slope threshold;
- ordinary step-up threshold;
- grounding/snap tolerance;
- gravity magnitude;
- authoritative acceleration/deceleration/gait semantics;
- maximum solver iterations or equivalent bounded collision-resolution rule.

Defaults from Godot, Unreal or Unity remain **references, not project values**.

## Godot presentation contract

Once continuous authoritative samples exist, Godot must:

- consume ordered `EntityId`-keyed spatial samples;
- visually smooth compatible samples without changing Simulation truth;
- reset/snap only across real `SpatialEpoch` discontinuities;
- not decide whether a wall, slope, step or landing was authoritative;
- keep camera/animation feel separate from native movement correctness.

The existing local `ThirdPersonPlayer` motor may remain temporarily useful during migration, but it must not become a second world-law implementation.

## Magic sensitivity surface

This is the non-magical baseline, but magic must later alter the **real constraints**:

- flight removes ordinary support requirement;
- levitation/altered gravity changes authoritative gravity/up/support rules;
- phasing changes applicable collision constraints;
- body transformation changes collision dimensions/clearance;
- supernatural speed changes movement limits while still resolving authoritative collision;
- teleportation changes `SpatialEpoch` instead of pretending to be extreme continuous velocity.

Do not reserve a generic `MagicMovementMode` hierarchy now.

## Source basis

The acceptance categories are based on established character-controller contracts, not copied implementations:

- **Godot 4.7 `CharacterBody3D`** — <https://docs.godotengine.org/en/4.7/classes/class_characterbody3d.html>: grounded mode, floor/wall classification, `floor_max_angle`, floor snapping and wall sliding.
- **Epic Games Unreal Engine `UCharacterMovementComponent`** — <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent> and <https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine>: walking/falling, walkable-floor semantics, step height, bounded simulation and authoritative movement/smoothing precedents.
- **Unity 6 Character Controller** — <https://docs.unity3d.com/6000.0/Documentation/Manual/class-CharacterController.html>: independent slope, step and contact/skin concerns.

These sources justify the **questions and observable categories**. Their default values and engine-specific algorithms are not imported into Simulation.

## Next bounded task

The next locomotion implementation should extend the accepted neutral model to **slope classification/traversal**, then ordinary step handling and fall/landing. Only after the native grounded set is stable should the shared transition be wired through `World`/protocol and drive ordered Godot presentation samples.

Do not build the visual first content location before the neutral fixture/solver stages prove the required collision semantics.

## Falsifiers

Revise this contract if a real third-person gameplay requirement shows that:

- grounded/wall/slope/step/fall categories are insufficient for the first playable movement slice;
- the body/contact model cannot express stable behavior without different observable semantics;
- a required magical capability invalidates ordinary support/gravity as the non-magical baseline;
- deterministic native evaluation is impossible with the chosen environment representation;
- player/NPC parity requires materially different world-rule inputs rather than different intent sources.
