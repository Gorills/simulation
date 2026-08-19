# Model: Grounded locomotion acceptance

Status: ACCEPTED

## Purpose

Define the minimum observable contract for authoritative third-person grounded locomotion and the neutral native test arena that proves it.

Simulation owns movement truth. Godot may present, interpolate or predict, but `CharacterBody3D` collision results and transforms are never authoritative world state.

Related contracts:

- [`spatial-location.md`](spatial-location.md)
- [`../decisions/0006-authoritative-spatial-contract.md`](../decisions/0006-authoritative-spatial-contract.md)
- [`../PERFORMANCE.md`](../PERFORMANCE.md)
- [`../ARCHITECTURE.md`](../ARCHITECTURE.md)

## Admission / implementation state

Implemented in the Godot-free native transition:

- flat support and exact fixed-step planar integration;
- head-on wall blocking;
- oblique wall motion with preserved tangential component;
- walkable-slope classification and grounded traversal;
- too-steep slope blocking of the gradient component while valid tangential motion remains;
- ordinary flat-support step-up at or below the explicit step threshold;
- blocking-step semantics above the threshold while valid tangential motion remains;
- ledge support loss without downward snapping;
- fixed-step gravity/falling with exact integer remainder carry;
- stable landing on lower walkable support without tunneling;
- zero-input rest on flat and walkable sloped support without downhill creep;
- deterministic replay for the implemented fixtures.

Still deliberately pending:

- `World` / protocol movement wiring;
- ordered authoritative movement samples and Godot reconciliation;
- prediction/navigation and the first content location.

## Authoritative invariants

1. **Simulation decides movement.** Human or NPC code supplies intent; the shared solver produces authoritative spatial state.
2. **No client-authored outcome.** There is no production `SetPosition`, `SetVelocity`, grounded flag or Godot collision result input.
3. **Player/NPC parity.** Equivalent actors use the same transition; control source is not a world-law distinction.
4. **Simulation-owned environment.** Godot geometry can visualize a fixture but is not authoritative collision input.
5. **Determinism.** Same initial state + environment + ordered intent + fixed configuration yields the same result.
6. **Continuous movement preserves `SpatialEpoch`.** Walls, slopes, steps and falls are not teleports.
7. **Selective exact fidelity remains valid.** Only actors needing exact local spatial causality require this state.
8. **Performance is correctness.** The current vector scans are acceptable only for the tiny acceptance fixture. They are not the production spatial index for a large world; production lookup must be bounded by actual local/causal geometry before this transition is used at scale.

## Current native representation

### Environment

`GroundedEnvironment` currently contains:

- `GroundPatch` — bounded X/Z support whose height is either flat or changes linearly along one planar axis;
- `VerticalBarrier` — two-sided axis-aligned vertical blocker at one X or Z coordinate and vertical range.

This is intentionally the smallest representation required by the acceptance arena. Finite arbitrary wall meshes, triangle-mesh collision, terrain acceleration structures and general rigid-body physics are not selected yet.

Acceptance fixtures should avoid ambiguous overlapping support patches except deliberate equal-height seams. The step fixture uses adjacent, non-overlapping flat patches with an integer-contiguous boundary so the support transition has one deterministic owner on each side. A real-location collision representation must define lookup/overlap semantics explicitly instead of depending on vector order.

### Body / timing / movement baseline

The first `UprightCapsule` uses the existing project shell values:

- radius: **380 mm**;
- height: **1800 mm**.

`GroundedStepConfig` currently uses:

- **60 authoritative steps/second**;
- **5800 mm/s** full-strength planar move-speed fixture;
- maximum walkable slope: **1192 mm rise per 1000 mm horizontal run**, an integer approximation of the existing project-owned **50°** Godot locomotion-profile baseline;
- maximum ordinary step-up: **300 mm**;
- non-magical downward gravity: **9807 mm/s²**, the nearest integer millimeter representation of standard gravity **9.80665 m/s²**.

The 300 mm step threshold is a project-owned first acceptance baseline. It is numerically aligned with the existing playable profile's **0.3 m** local floor-contact reach so the migration does not introduce an unrelated scale, but step-up and floor snap remain different semantics. It is not copied from an engine default and remains reviewable through real playtest before authoritative locomotion becomes player-facing.

The gravity baseline is a physical non-magical starting point rather than a Godot/Unreal/Unity controller default. It is configuration, not a universal immutable law: future world conditions or magic may alter it through explicit authoritative rules.

These are repository migration/acceptance values, not immutable final feel tuning.

Acceleration/deceleration, sprint, facing/turn response, grounding snap, jump semantics and airborne steering are not yet authoritative.

### Input

`PlanarMoveIntent` is signed analog X/Z intent at fixed scale 1000 and must remain inside the unit circle. It is intent only, never displacement.

While grounded, current intent directly defines the existing constant-speed planar fixture. When support is lost, the fall slice preserves takeoff planar velocity and deliberately does not invent airborne steering. A later air-control rule, if wanted, must be explicit rather than silently reusing grounded intent semantics.

### Integer integration

Authoritative positions and velocities remain integer millimeters / millimeters-per-second. Per-axis position remainders carry fractional millimeters between ticks rather than truncating them every step.

Airborne gravity additionally carries a separate vertical-velocity remainder so dividing acceleration by the 60 Hz tick rate does not lose fractional millimeters-per-second each tick. Falling uses deterministic semi-implicit fixed-step integration: gravity updates vertical velocity first, then that velocity advances Y for the tick.

Slope support height is derived deterministically from the patch endpoints and planar coordinate. Walkability uses integer rise/run comparison; authoritative code does not call floating-point trigonometry.

## Implemented behavior

### Flat ground

Supported movement advances according to intent and fixed configuration. Zero input produces stable rest with no positional jitter.

At the current fixture, 5800 mm/s for 60 full-strength ticks resolves to exactly 5800 mm travel.

### Walls

Head-on movement stops at capsule radius and repeated input does not accumulate penetration.

For oblique input, the wall-normal component is constrained while the tangential component remains valid.

### Walkable slope

A non-flat `GroundPatch` whose absolute rise/run is within the configured threshold is ordinary ground.

While traversing it:

- X/Z movement remains intent-driven;
- authoritative Y is derived from the support surface;
- vertical velocity reflects the per-tick support-height change;
- the actor remains in the same `SpatialEpoch`;
- zero input does not introduce implicit downhill creep.

### Too-steep slope

A patch outside the threshold is not ordinary grounded support for ascent.

When a candidate grounded step enters a too-steep patch:

- the movement component along that patch's gradient axis is blocked;
- its velocity/remainder on that axis are cleared;
- a valid tangential component is preserved when supported by ordinary ground;
- the actor is not allowed to stand authoritatively on the too-steep patch through this grounded transition.

Airborne impact/sliding semantics for an unwalkable steep surface remain deliberately unresolved. The current fall acceptance lands only on walkable support; it does not fake a stable grounded state on a too-steep ramp or invent a generic sliding model before real geometry requires one.

### Ordinary step-up and blocking step

The first step slice deliberately covers a discrete **upward transition between distinct adjacent flat support patches**. It does not reinterpret ordinary movement inside one patch as a step and does not use a flat patch's `gradient_axis` as a hidden step normal.

When a candidate move enters a higher flat patch:

- the solver compares the positive support-height discontinuity with `max_step_up`;
- a rise at or below **300 mm** is ordinary grounded movement and authoritative Y moves to the higher support in the same `SpatialEpoch`;
- a rise above **300 mm** blocks only the planar axis or axes by which the candidate entered the higher patch;
- blocked axes clear velocity and integration remainder;
- valid tangential movement on the lower support remains;
- the entry axis is derived from actual patch-boundary membership, not from presentation geometry or client collision results.

The paired acceptance fixture is intentionally sharp: **300 mm traverses; 301 mm blocks**. A downward discontinuity between distinct supports is not a reverse step-up or a floor snap: it becomes support loss and uses the fall/landing transition. Compound ramp-to-step seams, arbitrary staircases and finite tread depth should be added only when real geometry requires them rather than by inventing a generic character-physics framework now.

### Ledge support loss, falling and landing

A grounded actor remains supported when it stays on the same walkable patch or enters equal/higher walkable support allowed by the existing slope/step rules. Entering no support, or a distinct lower walkable patch, is support loss.

On support loss:

- authoritative Y is **not** snapped to a lower patch;
- grounded support-derived `velocity.y` is not treated as launch inertia; falling starts with zero vertical inertial velocity because no jump mechanic exists yet;
- gravity is applied during the same fixed tick that loses support;
- planar takeoff velocity and its integration remainder continue through the airborne phase;
- later planar intents do not rewrite that velocity while airborne in this slice;
- ordinary falling preserves the existing `SpatialEpoch`.

While airborne, gravity integrates vertical velocity using the configured positive downward magnitude and its own remainder. A lower walkable support is a landing candidate only when the falling Y path crosses that support from above. The landing clamps Y exactly to support, zeros vertical velocity plus both vertical remainders, and cannot tunnel through the plane even when one fixed-tick displacement would otherwise pass below it.

This slice deliberately does not add jump impulse, coyote time, air acceleration/steering, floor snap, fall damage or steep-surface sliding.

### Deterministic replay

Native tests require exact equality for repeated flat/wall, slope, step and ledge/fall/landing intent streams from identical state/configuration/environment.

## Neutral acceptance arena

```text
flat lane
wall lane:
  head-on wall
  oblique approach
slope lane:
  below-threshold ramp
  above-threshold ramp
step lane:
  300 mm traversable step
  301 mm blocking step
ledge lane:
  platform
  drop
  lower landing plane
```

Rules:

- dimensions use Simulation integer millimeters;
- native data is authoritative; Godot scenes are visualization only;
- paired fixtures sit deliberately on opposite sides of the real configured threshold;
- tests assert outcomes rather than duplicating solver classification logic;
- do not generalize the tiny fixture representation into arbitrary-world collision before a real location requires it.

## Parameters deliberately unresolved

Still require reviewed choices backed by the next real mechanic/playtest:

- contact/skin tolerance beyond the exact current boundaries;
- grounding/snap tolerance;
- acceleration/deceleration/gait semantics;
- jump and airborne steering semantics;
- airborne interaction/sliding on unwalkable steep surfaces;
- bounded collision-resolution iteration or equivalent rule;
- production local-geometry indexing/query representation.

Defaults from Godot, Unreal or Unity remain references, not project values.

## Godot presentation contract

Once authoritative continuous samples are exposed, Godot must:

- consume ordered `EntityId`-keyed samples;
- smooth compatible samples without changing Simulation truth;
- snap/reset only across real `SpatialEpoch` discontinuities;
- never decide authoritative wall/slope/step/landing results;
- remove duplicate local world-law movement as authoritative migration completes.

The current local `ThirdPersonPlayer` remains a temporary presentation/feel shell.

## Performance contract

The acceptance fixture is intentionally tiny, so the current `std::vector` environment scan is bounded test code, not evidence that scanning all world geometry per actor per 60 Hz tick is acceptable.

Before this solver is applied to a real populated location, admission must identify the actual local-geometry query bound and measure it against [`../PERFORMANCE.md`](../PERFORMANCE.md). Do not add an octree/BVH/ECS merely in anticipation; do not ship an unbounded full-location scan either.

## Magic sensitivity surface

Magic must modify real authoritative constraints when implemented:

- flight removes ordinary support requirements;
- levitation/altered gravity changes support/gravity rules;
- phasing changes applicable collision constraints;
- body transformation changes collision dimensions/clearance;
- supernatural speed changes movement limits while preserving authoritative collision;
- teleportation changes `SpatialEpoch` rather than becoming extreme continuous velocity.

Do not reserve a generic `MagicMovementMode` hierarchy now.

## Source basis

The behavior categories are adapted from established controller contracts, not their implementations/default values:

- Godot 4.7 `CharacterBody3D`: <https://docs.godotengine.org/en/4.7/classes/class_characterbody3d.html>
- Unreal Engine `UCharacterMovementComponent`: <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent>
- Unity 6 Character Controller: <https://docs.unity3d.com/6000.0/Documentation/Manual/class-CharacterController.html>

The non-magical gravity acceptance baseline uses NIST's standard acceleration of free fall, **9.80665 m/s²**, rounded to the nearest integer project spatial unit as **9807 mm/s²**: <https://www.nist.gov/pml/special-publication-811/nist-guide-si-appendix-b-conversion-factors/nist-guide-si-appendix-b9>.

The 50° slope migration baseline and the numerical 0.3 m reference used to select the initial 300 mm step acceptance baseline come from this repository's existing `godot/config/default_locomotion_profile.tres`, not from an engine default. Their semantics remain separate.

## Next bounded task

Expose this stable shared movement transition through `World` and a semantic controlled-actor movement protocol command/result. Do not expose a final-position setter. Ordered authoritative movement samples and Godot reconciliation remain the following slice after the World/protocol transition is proven.

Do not build the first visual content location before the required native collision semantics are proven.

## Falsifiers

Revise this model if a real third-person requirement shows that:

- the flat/wall/slope/step/fall categories are insufficient;
- the body/contact representation cannot provide stable behavior without different observable semantics;
- a required magical capability invalidates support/gravity as the non-magical baseline;
- deterministic native evaluation is impossible with the chosen environment representation;
- player/NPC parity requires materially different world-rule inputs rather than different intent sources.
