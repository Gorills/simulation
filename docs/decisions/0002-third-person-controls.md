# ADR 0002: Third-person Godot control and presentation shell

Status: Superseded in part by ADR 0004  
Date: 2026-08-19

Related contracts: [`../PRODUCT.md`](../PRODUCT.md) · [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../engineering/godot.md`](../engineering/godot.md)

Supersedes ADR 0001 decision 3 where the initial Godot client was described as 2D presentation.

ADR 0004 supersedes the parts of this decision that granted Godot ownership of authoritative local character position/collision state. Input decomposition, control profiles, camera architecture and presentation-side responsiveness remain accepted.

## Context

The Milestone 0 square proved Godot/GDExtension rendering but was not a suitable player-experience reference. The product requires a desktop third-person action-RPG control feel with free camera orbit, keyboard/mouse and gamepad parity, and tunable response.

Godot still must not become the authoritative world merely because it renders the controlled actor.

## Decision

### Godot owns input/camera/presentation responsiveness

Godot owns:

- keyboard/mouse/gamepad sampling;
- pointer lifecycle;
- camera orbit/collision;
- animation/audio/VFX/UI;
- the current `CharacterBody3D` presentation/prediction shell during spatial-authority migration.

Godot does **not** own authoritative actor position or systemic outcomes. A local transform may be interpolated/predicted for responsiveness, but it cannot grant ownership, inventory, access, trade success, damage or another world consequence.

### The native grid round trip is explicitly bootstrap-only

The current transport probe is named accordingly:

```text
BootstrapMoveIntent
  -> protocol::Simulation::bootstrap_move
  -> World::apply_bootstrap_step
  -> BootstrapActorProjection
```

It exists only to prove Godot -> GDExtension -> protocol -> Simulation round-trip behavior. It must not be extended into production third-person spatial movement.

Production locomotion follows ADR 0004: semantic actor intent -> authoritative Simulation spatial result -> ordered presentation samples -> Godot interpolation and, only if needed, local prediction/reconciliation.

### Controls are decomposed by responsibility

```text
InputMap
  -> PlayerControls + ControlProfile
       -> ThirdPersonPlayer + LocomotionProfile   # presentation/prediction response
       -> ThirdPersonCameraRig
            -> SpringArm3D -> Camera3D
```

- `PlayerControls` translates devices into semantic intent; it never changes world state.
- `ControlProfile` owns input/camera feel.
- `LocomotionProfile` owns current presentation response values, not authoritative Simulation movement law.
- `ThirdPersonPlayer` is the current presentation/prediction shell and never reads raw device bindings.
- `ThirdPersonCameraRig` owns camera orientation/collision and never owns world movement.
- `Main` wires the components.

### Input baseline

- WASD / left stick: movement intent;
- mouse / right stick: camera;
- Shift / L3: sprint intent;
- Escape: release captured mouse;
- unhandled left click while released: recapture.

Mouse and gamepad look remain separate sampling paths: event-based captured mouse motion uses `screen_relative`; right-stick look is continuous and delta-scaled.

### Camera baseline

```text
CameraRig (yaw)
  -> PitchPivot
       -> SpringArm3D
            -> Camera3D
```

The direct SpringArm -> Camera relationship and player presentation-collider exclusion remain the default third-person collision solution.

### Presentation-response baseline

The current presentation shell preserves analog input magnitude and exposes separate acceleration, deceleration, direction-change acceleration and turn response. It avoids a second opaque low-pass filter on movement input.

Those values are starting presentation defaults, not claimed Witcher 3 constants and not authoritative Simulation movement constants.

When authoritative movement samples arrive, the presentation shell must reconcile to them rather than copying its local transform back into Simulation.

## Source basis

The implementation uses official Godot 4.7 guidance for InputMap/controller deadzones, captured mouse motion, SpringArm third-person camera behavior and interpolation; official Godot TPS/platformer examples informed decomposition and camera-relative movement. CD PROJEKT RED's Witcher 3 patch 1.07 is only product-design evidence that movement response is a deliberate tuning concern.

Exact sources are recorded in [`../engineering/SOURCES.md`](../engineering/SOURCES.md).

## Consequences

Positive:

- keyboard/mouse and gamepad share semantic intent without duplicating movement implementations;
- camera, input and presentation response can evolve independently;
- presentation feel can remain responsive while Simulation remains authoritative;
- temporary grid transport code is visually impossible to mistake for the production movement contract.

Costs:

- the current `CharacterBody3D` shell must be migrated to authoritative movement samples;
- authoritative spatial/collision design remains separate work;
- final feel still requires real-device playtesting.

## Rejected alternatives

### One large `player.gd`

Rejected because input, camera, presentation response and gameplay authority become entangled.

### Raw device reads in the motor

Rejected because remapping/device parity drift into movement logic.

### Camera with no collision arm

Rejected because geometry clipping is a known third-person problem with an engine-provided solution.

### Heavy smoothing before the motor

Rejected because it hides latency and makes response tuning ambiguous.

### Godot kinematics as authoritative world location

Superseded/rejected by ADR 0004 because it would create a second world authority and make onscreen/offscreen entities obey different location rules.
