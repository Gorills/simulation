# ADR 0002: Third-person Godot locomotion and control shell

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../PRODUCT.md`](../PRODUCT.md) · [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../engineering/godot.md`](../engineering/godot.md)

Supersedes ADR 0001 decision 3 only where it described the initial Godot client as 2D presentation.

## Context

The 2D square used by the Milestone 0 bootstrap proved that Godot could load the GDExtension and render a returned native projection. It was never a suitable reference implementation for the intended player experience.

The product direction is a desktop 3D third-person systemic RPG with an action-RPG control feel: free camera orbit, camera-relative locomotion, keyboard/mouse and gamepad parity, and controls that can be tuned without rewriting gameplay code.

At the same time, the existing architectural rule remains important: systemic world laws and outcomes must not migrate into GDScript merely because the player is controlled in Godot.

## Decision

### Godot owns the immediate locomotion shell

Godot owns the parts that are intrinsically engine-facing and latency-sensitive:

- keyboard/mouse and gamepad sampling;
- pointer capture;
- local `CharacterBody3D` kinematic motion and scene collision response;
- player presentation transform and facing;
- third-person camera orbit and camera collision;
- animation/audio/UI driven from those presentation values.

This is **not** permission for Godot to own systemic world outcomes. A local transform does not grant ownership, inventory, access rights, trade success, damage results, institutional state or other authoritative consequences.

The Simulation Core continues to own semantic world state and causal gameplay rules. When precise player locomotion needs to affect semantic location, interaction reachability or other world rules, that boundary must be expressed explicitly through the application protocol rather than by letting arbitrary scene state become world truth.

### The old native grid move is bootstrap evidence, not the 3D locomotion API

The existing `MoveIntent(dx, dy)` path remains useful as the smallest GDExtension/protocol round-trip proof during Milestone 0.

It must not be copied as the movement model for the third-person player. Fluid character locomotion is represented by the Godot locomotion shell until a real semantic-location capability requires a native contract.

### Controls are decomposed by responsibility

The reference control stack is:

```text
InputMap
  -> PlayerControls
       -> ControlProfile
       -> semantic move/look/sprint state
  -> ThirdPersonPlayer
       -> LocomotionProfile
       -> CharacterBody3D motion/facing
  -> ThirdPersonCameraRig
       -> SpringArm3D
       -> Camera3D
```

Responsibilities are deliberately narrow:

- `ControlProfile` contains camera/input tuning values only.
- `LocomotionProfile` contains motor tuning values only.
- `PlayerControls` translates device input into semantic control state; it never moves the player.
- `ThirdPersonPlayer` consumes movement intent and owns local kinematic motion; it never reads raw keys or joystick indices.
- `ThirdPersonCameraRig` consumes look intent and owns camera orientation/collision; it never moves the player.
- `Main` is the composition root that wires these components together.

A future model should normally be able to tune a feel value by editing a `.tres` profile, change camera behavior inside the camera rig, or change movement behavior inside the motor without touching the other components.

### Input baseline

The default desktop scheme starts with:

- WASD / left stick: movement;
- mouse / right stick: camera;
- Shift / L3: sprint;
- Escape: release captured mouse;
- left mouse click while released: recapture.

InputMap actions are semantic and controller mappings use Godot/SDL-standard axes. Future combat/interaction actions extend the same InputMap rather than adding raw-key branches to player scripts.

Mouse and gamepad look intentionally use different sampling paths. Mouse motion is event-based and uses `InputEventMouseMotion.screen_relative`; right-stick look is continuous and delta-scaled.

### Camera baseline

The camera uses the Godot-recommended third-person hierarchy:

```text
CameraRig (yaw)
  -> PitchPivot
       -> SpringArm3D
            -> Camera3D
```

`Camera3D` remains a direct child of `SpringArm3D` with no custom shape so the spring arm can use the camera near-plane pyramid for collision sweeping. The player body is excluded from the arm collision query.

Pitch is clamped. Mouse sensitivity, controller angular speed, deadzone, optional controller response exponent, inversion, field of view, target height, arm distance and collision margin live in `ControlProfile`.

### Movement baseline

Ground movement is camera-relative and preserves analog stick magnitude.

The motor does **not** low-pass-filter raw movement input before gameplay sees it. Responsiveness comes from direct input plus explicit motor response rates, where the physical feel can be tuned and reasoned about. Acceleration, deceleration and direction-change acceleration are independent values; changing braking must not silently change steering/reversal response.

`CharacterBody3D` movement runs in `_physics_process()` using `move_and_slide()`. Physics interpolation is enabled so rendering can remain smooth at a fixed physics tick. The camera follows the interpolated player transform in render processing.

The body turns toward desired movement using a frame-rate-independent exponential response. Rotation tuning is separate from translational response.

### Tuning values are defaults, not laws

The committed profile values are a reviewed starting point, not a claim that a particular commercial game uses the same numeric constants.

Change values from playtest evidence. Do not fork the input or motor implementation merely to get a different sensitivity, deadzone, speed or turn rate.

## Source basis

The implementation is based on:

- Godot 4.7 controller guidance: semantic InputMap, `Input.get_vector()` and circular deadzones;
- Godot 4.7 mouse guidance: `screen_relative` for captured mouselook;
- Godot 4.7 third-person camera guidance: `SpringArm3D` with a direct `Camera3D` child;
- Godot 4.7 physics interpolation guidance;
- Godot's official TPS demo separation of player input/camera from player motion, camera-relative movement and controller/mouse camera paths;
- Godot's official 4.7 3D platformer use of `CharacterBody3D`, camera-relative `Input.get_vector()`, explicit acceleration/deceleration and physics interpolation;
- CD PROJEKT RED's Witcher 3 patch 1.07 record that movement response required a dedicated alternative mode, reinforcing that response tuning must be an explicit player-experience concern.

The primary URLs are recorded in [`../engineering/SOURCES.md`](../engineering/SOURCES.md).

## Consequences

Positive:

- the committed code itself demonstrates the intended composition for future models;
- input devices, camera feel and locomotion feel can evolve independently;
- keyboard/mouse and gamepad share semantic actions without pretending their camera input is sampled identically;
- the camera cannot trivially clip through scene geometry;
- analog movement remains analog;
- exact control tuning can change without creating new gameplay architecture.

Costs:

- the project now has a deliberate distinction between local kinematic/presentation state in Godot and systemic world authority in C++;
- future semantic-location and interaction work must define the bridge explicitly;
- final feel still requires real controller/mouse playtesting on the pinned Godot runtime.

## Rejected alternatives

### One large `player.gd`

A monolithic player script makes every tuning task a cross-cutting edit and encourages weak models to mix input, camera, motor, animation and gameplay authority.

### Raw device keys in the motor

This makes remapping and controller parity expensive and teaches future code to duplicate device-specific logic.

### Direct camera child with no collision arm

This clips through geometry and ignores Godot's dedicated third-person camera solution.

### Heavy input smoothing before the motor

This hides latency inside an opaque filter. The baseline keeps raw intent immediate and puts translational response in explicit motor rates where it can be tuned deliberately.

### Reimplementing engine character collision in the Simulation Core now

That would create a bespoke 3D movement/physics problem before systemic gameplay requires it. The Godot locomotion shell uses the engine's purpose-built `CharacterBody3D`; semantic world consequences remain native.
