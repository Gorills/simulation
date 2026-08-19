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
- zero-input rest on flat and walkable sloped support without downhill creep;
- deterministic replay for the implemented fixtures.

Still deliberately pending:

- ordinary step-up / blocking-step semantics;
- ledge support loss, gravity, falling and landing;
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

Acceptance fixtures should avoid ambiguous overlapping support patches except deliberate equal-height seams. A real-location collision representation must define lookup/overlap semantics explicitly instead of depending on vector order.

### Body / timing / movement baseline

The first `UprightCapsule` uses the existing project shell values:

- radius: **380 mm**;
- height: **1800 mm**.

`GroundedStepConfig` currently uses:

- **60 authoritative steps/second**;
- **5800 mm/s** full-strength planar move-speed fixture;
- maximum walkable slope: **1192 mm rise per 1000 mm horizontal run**, an integer approximation of the existing project-owned **50°** Godot locomotion-profile baseline.

These are repository migration values, not engine defaults and not immutable final feel tuning.

Acceleration/deceleration, sprint, facing/turn response, step height, grounding snap and gravity are not yet authoritative.

### Input

`PlanarMoveIntent` is signed analog X/Z intent at fixed scale 1000 and must remain inside the unit circle. It is intent only, never displacement.

### Integer integration

Authoritative positions and velocities remain integer millimeters / millimeters-per-second. Per-axis integration remainders carry fractional millimeters between ticks rather than truncating them every step.

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

When a candidate step enters a too-steep patch:

- the movement component along that patch's gradient axis is blocked;
- its velocity/remainder on that axis are cleared;
- a valid tangential component is preserved when supported by ordinary ground;
- the actor is not allowed to stand authoritatively on the too-steep patch through this grounded transition.

Sliding/falling on steep surfaces belongs to the future gravity/fall slice; this stage does not invent it.

### Deterministic replay

Native tests require exact equality for repeated flat/wall and slope intent streams from identical state/configuration/environment.

## Neutral acceptance arena

```text
flat lane
wall lane:
  head-on wall
  oblique approach
slope lane:
  below-threshold ramp
  above-threshold ramp
step lane (pending):
  below-threshold step
  above-threshold step
ledge lane (pending):
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
- ordinary step-up threshold;
- grounding/snap tolerance;
- gravity and falling integration;
- acceleration/deceleration/gait semantics;
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

The 50° migration baseline comes from this repository's existing `godot/config/default_locomotion_profile.tres`, not from an engine default.

## Next bounded task

Implement ordinary **step-up versus blocking-step** semantics using the accepted neutral fixture. Then implement ledge/fall/landing. Only after the native grounded set is stable should the shared transition enter `World`/protocol and drive ordered Godot presentation samples.

Do not build the first visual content location before the required native collision semantics are proven.

## Falsifiers

Revise this model if a real third-person requirement shows that:

- the flat/wall/slope/step/fall categories are insufficient;
- the body/contact representation cannot provide stable behavior without different observable semantics;
- a required magical capability invalidates support/gravity as the non-magical baseline;
- deterministic native evaluation is impossible with the chosen environment representation;
- player/NPC parity requires materially different world-rule inputs rather than different intent sources.
