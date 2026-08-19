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
- authoritative grounded acceleration, braking and reversal-before-opposite-acceleration;
- actor-specific base locomotion capability with semantic `walk` / `run` / `sprint` pace resolution inside `World`;
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
- deterministic replay for the implemented fixtures;
- one actor-keyed `World` locomotion batch per authoritative tick, with atomic validation/mutation and one `SimulationTick` / `WorldRevision` advance regardless of actor count;
- persisted per-actor fixed-step continuation required for deterministic subsequent movement;
- semantic controlled-actor protocol direction + pace intent whose submission does not itself mutate world state, followed by a separate authoritative locomotion tick through the same `World` transition;
- one post-transition authoritative sample batch per successful locomotion tick, with samples canonically ordered by ascending `EntityId` and shared tick/revision metadata;
- a deterministic Core-owned NPC local-waypoint producer that reads authoritative actor spatial state and emits the same actor-generic movement-intent shape used by human control;
- parity evidence that equivalent human and NPC-produced intents enter one actor-generic World batch and produce equivalent spatial/velocity results.

Implemented across the presentation boundary:

- protocol v6 GDExtension methods for semantic controlled movement direction + pace and authoritative sample-batch advancement;
- Godot validation of protocol version, consecutive locomotion tick, newer revision, canonical `EntityId` order, observed identity and valid spatial sample payload before presentation mutation;
- controlled physics-root position/velocity driven by Simulation samples on Godot physics ticks;
- same-epoch smoothing through the project's enabled Godot physics interpolation and interpolation reset on `SpatialEpoch` discontinuity;
- controlled input uses ordinary `run` pace and the existing sprint action selects `sprint`;
- Godot's `LocomotionProfile` is presentation-only and contains no competing speed/acceleration/braking law.

Still deliberately pending:

- local prediction only if measured playtest latency proves it necessary;
- production navigation/content-location geometry and indexing chosen from a real location requirement;
- stamina/endurance, carried load, wounds, progression and concrete magical movement effects until those mechanics exist.

## Authoritative invariants

1. **Simulation decides movement.** Human or NPC code supplies intent; the shared solver produces authoritative spatial state.
2. **No client-authored outcome.** There is no production `SetPosition`, `SetVelocity`, grounded flag, meters-per-second input or Godot collision result input.
3. **Player/NPC parity.** Equivalent actors use the same transition; control source is not a world-law distinction.
4. **Pace is intent, capability is state.** An actor chooses `walk`, `run` or `sprint`; it does not choose the numeric speed limit. `World` resolves the requested pace against authoritative actor capability.
5. **One resolver seam.** Actor state that genuinely changes locomotion capability must affect the `World` resolver rather than add player/NPC-specific multipliers or another solver.
6. **Simulation-owned environment.** Godot geometry can visualize a fixture but is not authoritative collision input.
7. **Determinism.** Same initial state + environment + ordered intent + fixed configuration yields the same result.
8. **Continuous movement preserves `SpatialEpoch`.** Walls, slopes, steps and falls are not teleports.
9. **Selective exact fidelity remains valid.** Only actors needing exact local spatial causality require this state.
10. **Performance is correctness.** The current vector scans are acceptable only for the tiny acceptance fixture, not production world geometry.
11. **Intent submission is not time advancement.** Protocol/controller state may replace desired direction/pace without mutating `World`; the fixed locomotion tick is the authoritative mutation boundary.
12. **A world tick is not an actor call count.** One movement batch may contain multiple actor intents and advances tick/revision once after the entire batch succeeds.
13. **Sample order is authoritative and source-independent.** A successful result contains each moved actor at most once, sorted by ascending `EntityId`.
14. **Presentation cannot resolve movement law.** Godot may validate/order/interpolate samples, but scene colliders, presentation transforms and local profiles cannot choose the authoritative outcome.

## Current native representation

### Environment

`GroundedEnvironment` currently contains:

- `GroundPatch` — bounded X/Z support whose height is either flat or changes linearly along one planar axis;
- `VerticalBarrier` — two-sided axis-aligned vertical blocker at one X or Z coordinate and vertical range.

This is intentionally the smallest representation required by the acceptance arena. Finite arbitrary wall meshes, triangle-mesh collision, terrain acceleration structures and general rigid-body physics are not selected yet.

Acceptance fixtures should avoid ambiguous overlapping support patches except deliberate equal-height seams. The step fixture uses adjacent, non-overlapping flat patches with an integer-contiguous boundary so the support transition has one deterministic owner on each side. A real-location collision representation must define lookup/overlap semantics explicitly instead of depending on vector order.

`GroundedLocomotionContext` packages shared environment/body/world-law fixture data. Its `GroundedStepConfig` carries tick rate, resolved per-step planar limits, slope, step and gravity values. Production `World` copies the shared config and overwrites `move_speed`, `acceleration` and `braking` from the current actor capability + requested pace before every grounded step. The temporary flat context therefore carries zero placeholders for those actor-resolved fields.

### Body / timing / actor locomotion capability

The first `UprightCapsule` uses the existing project shell values:

- radius: **380 mm**;
- height: **1800 mm**.

The shared acceptance context currently uses:

- **60 authoritative steps/second**;
- maximum walkable slope: **1192 mm rise per 1000 mm horizontal run**, an integer approximation of the retained **50°** migration baseline;
- maximum ordinary step-up: **300 mm**;
- non-magical downward gravity: **9807 mm/s²**, the nearest integer millimeter representation of standard gravity **9.80665 m/s²**.

`ActorLocomotionCapability` is authoritative actor state. The first project feel baselines are:

- `walk_speed`: **1000 mm/s**;
- `run_speed`: **3000 mm/s**;
- `sprint_speed`: **5800 mm/s**;
- grounded acceleration: **6000 mm/s²**;
- grounded braking: **8000 mm/s²**.

These numbers are **playtest baselines, not biological claims or universal human constants**. The 1.0 m/s walk is intentionally below the previously tried 1.45 m/s value because that looked too fast in the current camera/scene presentation. The former 5.8 m/s migration value is retained only as the current sprint ceiling, not the speed of every actor with full input.

Capability validates `0 <= walk <= run <= sprint` plus non-negative acceleration/braking. It is stored per actor and captured by `WorldSnapshot` because it changes future authoritative movement.

Current `World` resolution intentionally uses only stored base capability + semantic pace. Future wounds, carried load, progression, status effects or concrete magic may alter resolved limits **only when their authoritative mechanics exist**. Do not pre-build a generic modifier stack, stat expression graph or `magic_speed_multiplier`.

Direct low-level geometry fixtures keep a one-tick planar-response default so wall/slope/step tests remain tests of geometry rather than feel tuning. That compatibility value is not used by production `World`, which always overwrites planar response from actor capability.

The 300 mm step threshold and gravity remain reviewable world-law/configuration values. Gravity is a physical non-magical baseline rather than an engine-controller default; explicit future world conditions or magic may alter it through concrete authoritative rules.

### Input and pace

`PlanarMoveIntent` is signed analog X/Z intent at fixed scale 1000 and must remain inside the unit circle. It is direction/magnitude intent only, never displacement or requested velocity.

`ActorGroundedMoveIntent` adds semantic `LocomotionPace`:

- `walk`;
- `run`;
- `sprint`.

The current grounded solver converts the resolved speed limit and analog intent into a target planar velocity, then approaches each component deterministically. Increasing magnitude in the same direction uses acceleration; decreasing magnitude uses braking; a requested reversal first brakes to zero before a later tick accelerates in the opposite direction. The current per-axis response is an acceptance movement law, not a biomechanics model.

When support is lost, the fall slice preserves takeoff planar velocity and deliberately does not invent airborne steering. Later grounded intent does not rewrite airborne planar velocity until landing. A future air-control rule, if wanted, must be explicit.

At the application boundary, `ControlledActorMoveIntent` carries X/Z analog intent plus protocol pace, not speed. Godot converts camera-relative input to X/Z, uses `run` normally and selects `sprint` while the sprint action is held. Numeric capability remains hidden Simulation truth.

### NPC local waypoint steering

`NpcLocalWaypoint` is the smallest current NPC-side locomotion decision input:

```text
actor EntityId
local waypoint X/Z in authoritative millimeters
caller-supplied per-axis arrival tolerance
semantic pace
```

`decide_npc_local_move_toward_waypoint()` reads authoritative `SpatialState` and returns one `ActorGroundedMoveIntent`. It is read-only: invalid waypoint/pace input, unknown actors or actors without exact spatial state return explicit decision errors without mutating world state or advancing time.

The producer deliberately uses coarse deterministic eight-way steering:

- one active axis -> full-strength `±1000` intent on that axis;
- both active axes -> equal `±707` components, the largest equal integer pair inside the scale-1000 unit circle;
- an axis already inside caller-provided arrival tolerance contributes zero.

Full-strength intent does **not** mean a universal 5.8 m/s. It means full requested magnitude at the waypoint's semantic pace; `World` resolves the actor's actual limits.

This is a local steering producer, not a high-level behavior model. It does not choose why the NPC moves, schedule an activity, search a route, query obstacles, compute actor capability or select production navigation geometry.

### Integer integration and continuation

Authoritative positions and velocities remain integer millimeters / millimeters-per-second. Per-axis position remainders carry fractional millimeters between ticks rather than truncating them every step.

Grounded acceleration/braking additionally carry per-axis planar-velocity remainders, and airborne gravity carries a vertical-velocity remainder. These preserve fractional mm/s changes at the fixed authoritative tick rate. Falling remains deterministic semi-implicit fixed-step integration: gravity updates vertical velocity first, then that velocity advances Y.

All remainders affect the next authoritative result, so `GroundedLocomotionContinuation` retains them together with tick-rate provenance. `WorldSnapshot` schema **v4** captures actor capability plus the expanded continuation. Changing configured tick rate while non-pristine continuation exists is rejected rather than silently changing arithmetic semantics.

Slope support height is derived deterministically from patch endpoints and planar coordinate. Walkability uses integer rise/run comparison; authoritative code does not call floating-point trigonometry.

## Implemented behavior

### Flat ground and planar response

Supported movement approaches the target velocity derived from analog intent, semantic pace and actor capability. Zero intent brakes toward stable rest without position jitter.

With the current default capability at 60 Hz:

- first full-strength movement tick from rest changes planar speed by **100 mm/s**;
- walk reaches **1000 mm/s** after 10 ticks;
- run reaches **3000 mm/s** after 30 ticks;
- sprint reaches **5800 mm/s** after 58 ticks;
- braking from **1000 mm/s** reaches zero after 8 ticks in the acceptance fixture.

After 60 continuous full-strength ticks from rest, current acceptance tests resolve X travel of **925 mm walk**, **2275 mm run** and **3045 mm sprint**. These values are deterministic evidence for the chosen integration, not target biomechanics.

### Walls

Head-on movement stops at capsule radius and repeated input does not accumulate penetration. Blocking an axis clears its authoritative planar velocity plus both position and velocity integration remainder on that axis.

For oblique input, the wall-normal component is constrained while the tangential component remains valid.

### Walkable slope

A non-flat `GroundPatch` whose absolute rise/run is within the configured threshold is ordinary ground.

While traversing it:

- X/Z movement remains intent-driven;
- authoritative Y is derived from the support surface;
- vertical velocity reflects per-tick support-height change;
- the actor remains in the same `SpatialEpoch`;
- zero input does not introduce implicit downhill creep.

### Too-steep slope

A patch outside the threshold is not ordinary grounded support for ascent. Entering it blocks the movement component along the gradient axis, clears that axis velocity/remainders, preserves valid tangential movement on ordinary support and never lets the actor become authoritatively grounded on the steep patch through this transition.

Airborne impact/sliding semantics for an unwalkable steep surface remain deliberately unresolved.

### Ordinary step-up and blocking step

The first step slice covers a discrete upward transition between distinct adjacent flat support patches. It does not reinterpret ordinary movement inside one patch as a step.

When a candidate move enters a higher flat patch:

- rise at or below **300 mm** traverses and authoritative Y moves to higher support in the same `SpatialEpoch`;
- rise above **300 mm** blocks only the planar entry axis/axes;
- blocked axes clear velocity and integration remainders;
- valid tangential movement on lower support remains.

The paired acceptance fixture stays intentionally sharp: **300 mm traverses; 301 mm blocks**. A downward discontinuity becomes support loss rather than reverse step-up/floor snap.

### Ledge support loss, falling and landing

A grounded actor remains supported on the same walkable patch or equal/higher support allowed by slope/step rules. Entering no support, or a distinct lower walkable patch, is support loss.

On support loss:

- authoritative Y is not snapped down;
- support-derived `velocity.y` is not treated as launch inertia;
- gravity applies on the same tick;
- current planar velocity and its remainders continue through airborne movement;
- later planar intents do not rewrite that velocity while airborne;
- ordinary falling preserves `SpatialEpoch`.

Landing on lower walkable support uses swept vertical crossing, clamps Y exactly to support, zeros vertical velocity/remainders and prevents tunneling. Jump impulse, coyote time, air steering, floor snap, fall damage and steep-surface sliding remain outside this slice.

### World batch, capability resolution, samples and protocol

`World::advance_grounded_locomotion_tick()` accepts actor-keyed `ActorGroundedMoveIntent` values plus one Simulation-owned locomotion context.

Before invoking the solver for each actor, `World` validates actor capability and pace and calls the single internal resolver that copies shared world-law config and injects that actor's speed/acceleration/braking limits. Equivalent actor state + equivalent pace + equivalent directional intent therefore uses the same transition regardless of human/NPC source.

Before mutation, the full batch validates entity identity, exact spatial state, duplicate actors, continuation compatibility, capability, pace and intent, and computes every candidate transition. Any failure rejects the batch without partial mutation/time advance. Samples are allocated and canonically sorted before commit; only then are spatial/continuation states committed and `SimulationTick` / `WorldRevision` advanced once.

The successful Core result is one `GroundedLocomotionTickResult` containing shared post-transition tick/revision and one `GroundedLocomotionSample` per moved actor, sorted by ascending `EntityId` independent of collection order.

At the protocol boundary, `submit_controlled_actor_move_intent()` validates/stores desired X/Z + pace only. `advance_locomotion_tick()` binds it to the controlled actor, collects the RestNeed NPC's walk intent, invokes one shared World batch and maps the result to `AuthoritativeMovementSampleBatch`. Protocol **v6** exposes pace because the client-facing semantic command shape changed. It still exposes no position/velocity setter or numeric speed request.

### Godot sample application

The local Godot client currently runs at the same project-owned 60 Hz fixed baseline. Each Godot physics tick submits camera-relative X/Z + semantic pace, advances one Simulation locomotion tick and receives one post-transition authoritative batch.

Before moving any bound presentation node, `WorldPresentation` validates the full batch and all applicable bindings/roots: exact next locomotion tick, newer revision, matching protocol version, ascending observed IDs, valid vectors/epochs and a controlled sample. Duplicate/stale/non-consecutive batches are rejected before presentation mutation.

For a valid controlled sample:

- the `CharacterBody3D` physics-root position and velocity are set from the authoritative sample;
- same-epoch updates use Godot physics interpolation for rendered smoothing;
- epoch change applies the discontinuity and resets interpolation;
- scene collision results are never fed back into Simulation;
- visual facing may follow authoritative velocity because facing remains presentation-only.

`LocomotionProfile` now retains only presentation `turn_response`; old local move/sprint speeds, acceleration/deceleration, floor/slope movement fields were removed so there is no second source of movement truth.

## Deterministic replay

Native tests require exact equality for repeated flat/wall, slope, step and ledge/fall/landing streams. Additional locomotion dynamics tests prove deterministic acceleration/braking/reversal; capability tests prove different paces on the same capability, the same pace on different capabilities, and snapshot preservation/rejection; World/protocol tests prove atomic multi-actor batches, deterministic continuation, canonical sample order, strict tick/revision ordering, protocol pace validation and RestNeed walk arrival/braking.

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
- paired fixtures sit deliberately on opposite sides of real configured thresholds;
- tests assert outcomes rather than duplicating solver classification logic;
- do not generalize the tiny fixture representation into arbitrary-world collision before a real location requires it.

The temporary flat context remains an integration fixture only. It demonstrates command-to-World routing and drives the current Godot sample bridge; it is not evidence that visible Godot floor/colliders are Simulation content.

## Parameters deliberately unresolved

Still require a concrete mechanic/playtest before selection:

- contact/skin tolerance beyond exact current boundaries;
- grounding/snap tolerance;
- stamina/endurance and sustainable pace selection;
- effects of carried load, wounds, body variation and progression on capability;
- concrete magical movement effects and their costs/countermeasures;
- jump and airborne steering semantics;
- airborne interaction/sliding on unwalkable steep surfaces;
- bounded collision-resolution iteration or equivalent rule;
- production local-geometry indexing/query representation.

Defaults from Godot, Unreal or Unity remain references, not project values.

## Godot presentation contract

Godot consumes ordered authoritative movement batches through the v6 GDExtension boundary. It must continue to:

- submit direction/magnitude + semantic pace, never numeric requested velocity;
- consume ordered `EntityId`-keyed sample batches rather than poll a read projection as a stream;
- reject duplicate, stale, malformed or non-consecutive controlled batches before moving presentation roots;
- smooth compatible same-epoch movement without changing Simulation truth;
- snap/reset only across real `SpatialEpoch` discontinuities;
- never decide authoritative wall/slope/step/landing results;
- keep local facing/camera state presentation-only;
- avoid reintroducing local `move_and_slide()`, Godot gravity, sprint speed or acceleration as a competing physics-root law.

Prediction is not currently present. Add it only if measured latency shows a real need, and keep predicted state disposable/reconcilable to authoritative samples.

## Performance contract

The acceptance fixture is intentionally tiny, so the current `std::vector` environment scan is bounded test code, not evidence that scanning all world geometry per actor per 60 Hz tick is acceptable.

Before this solver is applied to a real populated location, admission must identify the actual local-geometry query bound and measure it against [`../PERFORMANCE.md`](../PERFORMANCE.md). Do not add an octree/BVH/ECS merely in anticipation; do not ship an unbounded full-location scan either.

## Magic sensitivity surface

Magic must modify concrete authoritative constraints/capability when implemented:

- flight removes ordinary support requirements;
- levitation/altered gravity changes support/gravity rules;
- phasing changes applicable collision constraints;
- body transformation changes collision dimensions/clearance and may change capability;
- a concrete supernatural-speed effect changes resolved actor movement capability while preserving authoritative collision;
- teleportation changes `SpatialEpoch` rather than becoming extreme continuous velocity.

Do not reserve a generic `MagicMovementMode` hierarchy, global effect bus or `magic_speed_multiplier` now.

## Source basis

The behavior categories are adapted from established controller contracts, not their implementations/default values:

- Godot 4.7 `CharacterBody3D`: <https://docs.godotengine.org/en/4.7/classes/class_characterbody3d.html>
- Unreal Engine `UCharacterMovementComponent`: <https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent>
- Unity 6 Character Controller: <https://docs.unity3d.com/6000.0/Documentation/Manual/class-CharacterController.html>

The non-magical gravity baseline uses NIST's standard acceleration of free fall, **9.80665 m/s²**, rounded to **9807 mm/s²**: <https://www.nist.gov/pml/special-publication-811/nist-guide-si-appendix-b-conversion-factors/nist-guide-si-appendix-b9>.

The 50° slope migration baseline and numerical 0.3 m reference used to select the first 300 mm step threshold came from the repository's pre-authoritative Godot locomotion profile. The current Godot profile is presentation-only; those retained values now live in Simulation/docs rather than client movement configuration.

The `1.0 / 3.0 / 5.8 m/s` pace values and `6.0 / 8.0 m/s²` planar response values are project feel baselines selected for this playable acceptance model. They are not sourced physiological constants and must be reviewed by playtest rather than defended as real-world measurements.

## Next bounded task

Continue **Milestone 1 — Living Need** from the now-corrected locomotion foundation. The next slice should add a real authoritative way for the player/world to affect the NPC need outcome or prove offscreen continuation, not another locomotion abstraction.

Do not add a generic behavior tree/GOAP/schedule framework, stamina system, modifier stack, prediction or production navigation architecture until a concrete gameplay requirement demonstrates the need.

## Falsifiers

Revise this model if a real third-person requirement shows that:

- the flat/wall/slope/step/fall categories are insufficient;
- current per-axis acceleration/braking produces materially wrong observable movement and needs a vector-response rule;
- actor capability cannot express a concrete gameplay factor without leaking movement law into input/presentation;
- a required magical capability invalidates support/gravity or ordinary capability resolution as the baseline;
- deterministic native evaluation is impossible with the chosen representation;
- player/NPC parity requires materially different world-rule inputs rather than different intent sources.
