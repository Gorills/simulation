# Godot 4 client — third-person 3D

Godot is the real presentation/input/UI client. It owns the latency-sensitive third-person locomotion shell but does not own systemic world laws or consequences.

Canonical contracts:

- runtime boundary: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- third-person ownership decision: [`../decisions/0002-third-person-controls.md`](../decisions/0002-third-person-controls.md)
- product/playable rule: [`../PRODUCT.md`](../PRODUCT.md)
- Godot/GDExtension version policy: [`VERSIONS.md`](VERSIONS.md)
- player-facing evidence: [`../VERIFICATION.md`](../VERIFICATION.md)

Primary Godot references are tracked in [`SOURCES.md`](SOURCES.md).

## Reference client shape

The reference player experience is a desktop third-person action-RPG camera/control scheme.

Current composition:

```text
Main
├── PlayerControls
├── Player (CharacterBody3D)
│   ├── CollisionShape3D
│   └── VisualRoot
├── CameraRig
│   └── PitchPivot
│       └── SpringArm3D
│           └── Camera3D
├── current-place presentation
└── HUD
```

`Main` is the composition root. It wires the input source, player motor and camera together; the reusable components do not discover unrelated siblings through hardcoded `../../` paths.

## Ownership: locomotion shell vs world authority

Godot owns immediate engine-facing state:

- sampled keyboard/mouse/gamepad input;
- captured-pointer state;
- `CharacterBody3D` local transform, collision response and slope handling;
- camera orbit/collision;
- animation/audio/UI presentation state.

The C++ Simulation Core owns systemic world truth and causal outcomes.

A Godot transform alone must not grant money, inventory, ownership, access rights, relationships, damage, trade success, law state or another systemic consequence. When local movement affects semantic location or a gameplay rule, add an explicit application/protocol contract.

The Milestone 0 native `MoveIntent(dx, dy)` is a protocol/GDExtension round-trip proof. It is not the reference API for fluid third-person locomotion.

## Control stack

The production control code is intentionally decomposed:

```text
default_control_profile.tres
  -> PlayerControls
       -> movement axis
       -> gamepad look axis
       -> accumulated mouse look
       -> sprint state
       -> active-device signal

default_locomotion_profile.tres
  -> ThirdPersonPlayer

PlayerControls + Player
  -> ThirdPersonCameraRig
```

### `ControlProfile`

Owns tuneable input/camera feel only:

- movement and look deadzones;
- mouse sensitivity;
- controller angular look speed;
- controller response exponent;
- mouse/controller inversion independently;
- pitch limits;
- camera target height/distance;
- camera collision margin;
- field of view.

Tuning these values should normally be a `.tres` edit, not a new code path.

### `LocomotionProfile`

Owns motor feel only:

- normal/sprint speed;
- acceleration/deceleration;
- direction-change acceleration;
- turn response;
- sprint analog threshold;
- floor snap and slope policy.

Do not put combat, inventory, animation-state-machine or camera settings into this resource.

### `PlayerControls`

Owns device translation only.

Use semantic InputMap actions. Do not put raw `KEY_W`, SDL axis numbers or mouse-button checks into `ThirdPersonPlayer`.

Movement uses:

```gdscript
Input.get_vector(
    &"move_left",
    &"move_right",
    &"move_forward",
    &"move_back",
    profile.move_deadzone,
)
```

This preserves the Godot-provided circular deadzone behavior for a two-axis control.

Mouse look is event-based and uses `InputEventMouseMotion.screen_relative` while the mouse is captured. Controller look is continuously sampled through a second `Input.get_vector()` using the right-stick actions.

These are intentionally separate paths: Godot's own controller guidance notes that mouse and controller look should not be treated as identical input devices.

`PlayerControls` also tracks the most recently active device so future button-prompt UI can react without duplicating device detection throughout the interface. Left-stick activity uses the radial `move_deadzone`; right-stick activity uses the radial `look_deadzone`, so independently tuning the two profiles does not desynchronize prompt detection from the controls.

### Pointer lifecycle

Gameplay starts with the mouse captured. Escape releases it. A **left click** while released recaptures it and consumes that recapture click. Wheel/right/middle-button events do not recapture the pointer.

Do not permanently hide/lock the mouse in a global singleton; menus and dialogue UI will need to own pointer visibility at appropriate times.

### Gameplay input gate

`Input` singleton polling reflects global device state; GUI event handling does not suppress `Input.get_vector()` or `Input.is_action_pressed()`. Modal UI/dialogue therefore must not rely on `accept_event()` alone to stop locomotion.

Use `PlayerControls.set_gameplay_enabled(false)` while a modal client state owns the controls. This zeros movement/look/sprint intent and clears buffered mouse look while **continuing active-device detection** for UI prompts. Pointer visibility remains a separate responsibility: the composition root or modal UI should call `release_pointer()`/`capture_pointer()` as appropriate.

Passive device tracking and captured mouse motion run in `_input()` so GUI consumption does not hide device changes. Pointer release and recapture run in `_unhandled_input()` so Controls get the first opportunity to consume those gameplay lifecycle events.

Do not scatter `if menu_open` checks through the motor and camera.

## Third-person camera

Use:

```text
yaw root
  -> pitch pivot
       -> SpringArm3D
            -> Camera3D
```

Keep `Camera3D` a direct child of the spring arm and leave the arm shape empty unless there is a measured reason to change it. Godot then uses the camera near-plane pyramid for the collision sweep instead of falling back to an inaccurate ray.

Exclude the player collision RID from the spring arm.

Mouse rotation:

- uses `screen_relative`;
- is not multiplied by frame `delta`, because the event already describes a displacement;
- has independent X/Y inversion.

Gamepad rotation:

- uses the right-stick semantic actions;
- is angular velocity and therefore multiplied by frame `delta`;
- has a circular deadzone;
- has an optional radial response exponent for future tuning;
- has independent X/Y inversion.

Pitch is clamped. Yaw is unbounded.

The camera follows `get_global_transform_interpolated()` from the player while rendering. This keeps camera follow aligned with Godot's physics interpolation instead of manually adding another positional smoothing layer.

Do not add camera lag, auto-centering, shoulder swaps, lock-on or combat zoom until a real gameplay state needs them. When they are added, keep them inside the camera component/profile rather than the motor.

## Third-person locomotion

`ThirdPersonPlayer` is a `CharacterBody3D`.

Movement is:

1. sampled from `PlayerControls`;
2. transformed into the horizontal camera frame;
3. scaled by analog magnitude;
4. converted to a target horizontal velocity;
5. approached using separate acceleration, deceleration and direction-change response rates;
6. applied with `move_and_slide()` in `_physics_process()`.

The controller does **not** smooth the raw movement axis separately. An opaque input filter adds latency and makes response harder to reason about; translational response belongs in explicit motor response rates. Acceleration, braking and direction changes are separate knobs so tuning one meaning does not silently change another.

The player body turns toward the desired travel direction with a frame-rate-independent exponential response. Rotation response is a separate tuning value from linear acceleration.

Analog stick magnitude is preserved, so slight stick deflection produces slower travel. Keyboard movement naturally produces full action strength.

Sprint changes target speed only when movement input exceeds the configured threshold; this avoids accidental full sprint from tiny stick deflection.

Physics interpolation is enabled. Teleports/respawns must call `reset_physics_interpolation()` after setting the new transform.

## InputMap baseline

Current semantic actions:

| Action | Keyboard/mouse | Gamepad |
| --- | --- | --- |
| move | WASD | left stick |
| camera | mouse motion | right stick |
| sprint | Shift | L3 |
| pointer release | Escape | — |

Future actions such as interact, dodge, light/heavy attack, guard/aim, quick item and radial menu belong in InputMap when the corresponding gameplay capability exists. Do not prebuild combat state machines merely to fill the controller.

Bindings are defaults, not hardcoded gameplay assumptions. Remapping can be added on top of the semantic action layer without rewriting the motor/camera.

## Frame and physics processing

- player motion and collision: `_physics_process()`;
- right-stick camera integration and interpolated camera follow: `_process()`;
- passive device tracking and captured mouse motion: `_input()`;
- pointer release/recapture after GUI handling: `_unhandled_input()`.

Neither `delta` is simulation time. Godot physics time drives the local locomotion shell only; authoritative world advancement follows the Simulation Core contract in [`../MODELING.md`](../MODELING.md).

## Scene independence and wiring

Prefer:

- typed component scripts;
- exported Resources for tuning;
- cached `@onready` references for owned children;
- parent/composition-root dependency wiring;
- semantic signals for child-to-parent notification.

Avoid:

- global input/controller Autoloads unless a later cross-scene lifecycle proves they are needed;
- hardcoded cross-scene `get_node("../../...")`;
- a mega `player.gd` containing input, camera, motor, combat, inventory and world state;
- component scripts reaching into sibling internals.

## Typed GDScript

Type parameters, returns and member state when the type is known. Use inference when it remains obvious and static.

Boundary DTO representation may initially use Godot-friendly `Dictionary` values, but UI code must not preserve arbitrary protocol blobs as live world state.

## Resources

Godot Resources are shared data containers. The committed control/locomotion profiles are intentionally shared immutable-style tuning resources at runtime.

Do not put per-player mutable authoritative state into those `.tres` resources.

If a runtime settings menu changes a profile, decide explicitly whether it is editing a user settings copy or a shared project default; do not accidentally mutate a globally shared asset and treat that as save-game state.

## Autoload policy

Autoloads are process-wide services, not a default dependency injection container or a second world authority.

Potentially appropriate future uses are small genuinely global presentation services such as user settings or a debug overlay.

Forbidden patterns include an Autoload `World`, authoritative `GameState`, global inventory/economy dictionaries or a global EventBus that becomes the ownership model for unrelated scenes.

## Files and project layout

Use snake_case files/folders and clear PascalCase scene node names.

The current control foundation lives under:

```text
godot/
  config/
    default_control_profile.tres
    default_locomotion_profile.tres
  scripts/
    controls/
      control_profile.gd
      locomotion_profile.gd
      player_controls.gd
      third_person_camera_rig.gd
    player/
      third_person_player.gd
```

Keep new control features near the component they modify. Do not create a generic `managers/` directory as a dumping ground.

## Anti-patterns

| Anti-pattern | Why |
| --- | --- |
| one monolithic `player.gd` | weak ownership; every tweak risks unrelated behavior |
| raw key/axis reads in player motor | breaks remapping and device parity |
| separate keyboard and gamepad movement implementations | behavior drifts by device |
| square manual stick deadzone | worse diagonal/low-magnitude feel than `Input.get_vector()` |
| mouse look from scaled `relative` | sensitivity changes with content scaling |
| mouse displacement multiplied by `delta` | makes mouse feel frame-rate dependent |
| gamepad angular rate not multiplied by `delta` | makes stick camera frame-rate dependent |
| camera without collision arm | clips through geometry |
| smoothing input and velocity simultaneously | compounds latency and obscures tuning |
| Godot transform granting systemic outcome | turns local kinematic state into a second world authority |
| shared `.tres` as save-game/player state | resource identity leaks mutable state |
| `_process`/physics delta as Simulation Core clock | frame/physics rate becomes a world law |

## Agent extension rule

When changing controls, identify the ownership first:

- binding, device translation or gameplay-input enable/disable -> InputMap / `PlayerControls`;
- sensitivity/deadzone/inversion/FOV/distance -> `ControlProfile`;
- acceleration/deceleration/direction-change/speed/turn/slope feel -> `LocomotionProfile`;
- movement algorithm/collision response -> `ThirdPersonPlayer`;
- camera orbit/collision/recenter/lock-on -> `ThirdPersonCameraRig`;
- combat/world consequence -> **not** the control stack; use the gameplay/protocol layer.

A change that touches three of these areas for one tuning request is probably in the wrong place.
