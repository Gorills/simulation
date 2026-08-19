# Godot 4 client — third-person 3D presentation

Godot is the real presentation/input/UI client. It owns latency-sensitive input, camera, rendering and presentation state; it does **not** own authoritative world state, including actor location.

Canonical contracts:

- runtime boundary: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- authoritative Simulation ↔ Godot boundary: [`simulation-godot-boundary.md`](simulation-godot-boundary.md)
- authoritative-world decision: [`../decisions/0004-authoritative-world-presentation-boundary.md`](../decisions/0004-authoritative-world-presentation-boundary.md)
- third-person control/presentation decision: [`../decisions/0002-third-person-controls.md`](../decisions/0002-third-person-controls.md)
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
├── Player (CharacterBody3D)       # current presentation/prediction shell
│   ├── CollisionShape3D
│   └── VisualRoot
├── CameraRig
│   └── PitchPivot
│       └── SpringArm3D
│           └── Camera3D
├── current-place presentation
└── HUD
```

`Main` is the composition root. It wires the input source, player presentation motor and camera together; reusable components do not discover unrelated siblings through hardcoded `../../` paths.

The current `CharacterBody3D` motor predates the final authoritative spatial bridge. Under ADR 0004 its transform is **presentation/prediction state only** until it is driven/reconciled from Simulation movement samples.

## Ownership: presentation shell vs world authority

Godot owns immediate engine-facing state:

- sampled keyboard/mouse/gamepad input;
- captured-pointer state;
- presentation-side `CharacterBody3D` transform/collision response used by the current migration shell;
- camera orbit/collision;
- animation/audio/VFX/UI presentation state;
- interpolation/prediction caches keyed by Simulation identity when those are introduced.

The C++ Simulation Core owns world truth and causal outcomes, including authoritative entity existence/location, inventory, ownership, economy, relationships, politics, combat consequences and magic effects as those mechanics are implemented.

A Godot transform must not decide authoritative reachability, trade range, attack range, semantic location, ownership or another world rule. The cross-boundary implementation route is [`simulation-godot-boundary.md`](simulation-godot-boundary.md).

The Milestone 0 native `MoveIntent(dx, dy)` is a protocol/GDExtension round-trip proof. It is not the production API for fluid third-person locomotion.

## Control stack

The production control/presentation code is intentionally decomposed:

```text
default_control_profile.tres
  -> PlayerControls
       -> movement axis
       -> gamepad look axis
       -> accumulated mouse look
       -> sprint state
       -> active-device signal

default_locomotion_profile.tres
  -> ThirdPersonPlayer     # current presentation/prediction response

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

Currently owns presentation-motor feel only:

- normal/sprint speed baseline;
- acceleration/deceleration;
- direction-change acceleration;
- turn response;
- sprint analog threshold;
- presentation-side floor/slope behavior used by the migration shell.

Do not infer that these values are authoritative Simulation movement constants. When the authoritative spatial model is introduced, authoritative rules get their own domain configuration/contracts and this Resource remains presentation tuning unless a deliberate bridge says otherwise.

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

These are intentionally separate paths: Godot's controller guidance notes that mouse and controller look should not be treated as identical input devices.

`PlayerControls` also tracks the most recently active device so button-prompt UI can react without duplicating device detection throughout the interface. Left-stick activity uses the radial `move_deadzone`; right-stick activity uses the radial `look_deadzone`.

In the authoritative movement migration, `PlayerControls` remains the semantic input source. Its movement intent will be sent through protocol to the controlled Simulation actor while the presentation shell uses the same intent only for allowed prediction/response.

### Pointer lifecycle

Gameplay starts with the mouse captured. Escape releases it. A **left click** while released recaptures it and consumes that recapture click. Wheel/right/middle-button events do not recapture the pointer.

Do not permanently hide/lock the mouse in a global singleton; menus and dialogue UI need to own pointer visibility at appropriate times.

### Gameplay input gate

`Input` singleton polling reflects global device state; GUI event handling does not suppress `Input.get_vector()` or `Input.is_action_pressed()`. Modal UI/dialogue therefore must not rely on `accept_event()` alone to stop gameplay input.

Use `PlayerControls.set_gameplay_enabled(false)` while a modal client state owns the controls. This zeros movement/look/sprint intent and clears buffered mouse look while **continuing active-device detection** for UI prompts. Pointer visibility remains separate: the composition root or modal UI should call `release_pointer()`/`capture_pointer()` as appropriate.

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

Exclude the player presentation collider RID from the spring arm.

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

The current camera follows `get_global_transform_interpolated()` from the presentation player while rendering. Once authoritative samples drive the presentation, keep the camera attached to the **rendered/interpolated representation**, not raw unsmoothed Simulation state.

Do not add camera lag, auto-centering, shoulder swaps, lock-on or combat zoom until a real gameplay state needs them. When added, keep them inside the camera component/profile rather than authoritative movement rules.

## Third-person movement migration

`ThirdPersonPlayer` is currently a `CharacterBody3D` presentation/prediction shell.

Its existing response pipeline is:

1. sample semantic intent from `PlayerControls`;
2. transform it into the horizontal camera frame;
3. preserve analog magnitude;
4. derive a presentation target velocity;
5. approach it with separate acceleration/deceleration/direction-change response rates;
6. run `move_and_slide()` for the current presentation shell.

This produces good local feel for the current playable but is **not** the final world-authority path.

The production path must migrate toward:

```text
PlayerControls semantic intent
  -> protocol intent for controlled EntityId
  -> Simulation authoritative spatial transition
  -> ordered/revisioned authoritative sample(s)
  -> Godot presenter
       -> interpolation
       -> optional local prediction + reconciliation if measured latency requires it
       -> CharacterBody3D/Node3D representation
```

Do not copy the current `move_and_slide()` result back into Simulation as authoritative position. The authoritative spatial/collision representation has not been selected yet and must be designed from actual terrain/navigation/determinism/performance constraints.

### Presentation-response rules

The current presentation shell does **not** low-pass-filter the raw movement axis separately. Acceleration, braking and direction changes are separate presentation knobs so tuning one meaning does not silently change another.

The body turns toward desired movement using a frame-rate-independent exponential response. Rotation response is separate from translational response.

Analog stick magnitude is preserved, so slight stick deflection produces slower presentation movement. Keyboard movement naturally produces full action strength.

Sprint changes the current presentation target speed only when movement input exceeds the configured threshold.

When authoritative movement is introduced, presentation parameters may need re-interpretation or reconciliation. Do not let a `.tres` presentation speed become an alternative authoritative speed rule.

### Prediction/reconciliation

Prediction is not required merely because the architecture permits it.

If playtest measurements show local input latency is unacceptable, local prediction may use the same semantic intent sent to Simulation. Keep predicted samples separate from authoritative samples and reconcile on authoritative updates.

Prediction must never grant:

- item transfer;
- trade success;
- hit/damage confirmation;
- ownership/access;
- relationship/political result;
- hidden information;
- another systemic consequence.

Prefer simple interpolation and measured evidence before adding complex prediction.

## InputMap baseline

Current semantic actions:

| Action | Keyboard/mouse | Gamepad |
| --- | --- | --- |
| move | WASD | left stick |
| camera | mouse motion | right stick |
| sprint | Shift | L3 |
| pointer release | Escape | — |

Future actions such as interact, dodge, light/heavy attack, guard/aim, quick item and radial menu belong in InputMap when the corresponding gameplay capability exists. A systemic action then becomes a semantic intent/command through the Simulation boundary; do not implement its outcome in the control script.

Bindings are defaults, not hardcoded gameplay assumptions. Remapping can be added on top of the semantic action layer without rewriting presentation or Simulation rules.

## Frame, physics and Simulation processing

Current Godot presentation processing:

- presentation motor/collision shell: `_physics_process()`;
- right-stick camera integration and rendered camera follow: `_process()`;
- passive device tracking and captured mouse motion: `_input()`;
- pointer release/recapture after GUI handling: `_unhandled_input()`.

Neither Godot `delta` is Simulation time. Authoritative world advancement follows the Simulation Core contract in [`../MODELING.md`](../MODELING.md).

Once authoritative transforms arrive on a timing boundary that does not coincide with local Godot physics ticks, do not assume built-in physics interpolation is automatically the correct fit. Store ordered authoritative samples and use a deliberately chosen presentation interpolation strategy. Godot's own interpolation guidance notes externally timed samples as a case where custom interpolation can be more suitable; source is recorded in [`SOURCES.md`](SOURCES.md).

## Presentation replicas

As multiple Simulation entities become visible, Godot should represent them by stable Simulation `EntityId`, conceptually:

```text
EntityId -> presentation node + animation state + interpolation samples + transient effects
```

Presentation lifecycle:

- materialize a representation when an entity enters the allowed local projection;
- update it from authoritative projections/events;
- dematerialize/pool it when it leaves that projection.

A presentation node disappearing is not entity death/deletion. An authoritative entity being created is not accomplished by instantiating a `.tscn`.

Do not create a Godot `World`/`GameState` cache that becomes the real owner of entity existence, inventory, relationships or shops.

## Scene independence and wiring

Prefer:

- typed component scripts;
- exported Resources for **presentation tuning**;
- cached `@onready` references for owned children;
- parent/composition-root dependency wiring;
- semantic signals for child-to-parent notification;
- stable Simulation `EntityId` for materialized world-representation identity.

Avoid:

- global input/controller Autoloads unless a later cross-scene lifecycle proves they are needed;
- hardcoded cross-scene `get_node("../../...")`;
- a mega `player.gd` containing input, camera, motor, combat, inventory and world state;
- component scripts reaching into sibling internals;
- scene-node paths or Godot instance IDs as Simulation identity.

## Typed GDScript

Type parameters, returns and member state when the type is known. Use inference when it remains obvious and static.

Boundary DTO representation may initially use Godot-friendly `Dictionary` values, but UI/presentation code must not preserve arbitrary protocol blobs as a live mutable world model.

Purpose-built Godot-facing wrappers may be introduced when repeated Dictionary handling becomes a demonstrated maintenance problem. Do not create a generic object-mapping framework first.

## Resources

Godot Resources are shared data containers. The committed control/locomotion profiles are shared presentation-tuning resources at runtime.

Do not put per-actor mutable authoritative state into those `.tres` resources.

If a runtime settings menu changes a profile, decide explicitly whether it is editing a user settings copy or a shared project default; do not accidentally mutate a globally shared asset and treat that as save-game state.

## Autoload policy

Autoloads are process-wide services, not a default dependency injection container or a second world authority.

Potentially appropriate future uses are small genuinely global presentation services such as user settings or a debug overlay.

Forbidden patterns include an Autoload authoritative `World`, authoritative `GameState`, global inventory/economy/relationship dictionaries or a global EventBus that becomes the ownership model for unrelated scenes.

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

Future presentation/materialization code should live near the presentation concern it owns. Do not create a generic `managers/` directory as a dumping ground.

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
| smoothing input and velocity simultaneously without need | compounds latency and obscures tuning |
| Godot transform as authoritative location/reachability | creates a second world authority |
| GDScript inventory/shop/relationship as world truth | duplicates Simulation state and rules |
| spawning a `.tscn` to create an authoritative NPC/item | scene lifetime becomes world lifetime |
| shared `.tres` as save-game/player state | resource identity leaks mutable state |
| `_process`/physics delta as Simulation Core clock | frame/physics rate becomes a world law |

## Agent extension rule

When changing the Godot client, identify ownership first:

- binding, device translation or gameplay-input enable/disable -> InputMap / `PlayerControls`;
- sensitivity/deadzone/inversion/FOV/distance -> `ControlProfile`;
- presentation acceleration/deceleration/direction-change/turn feel -> `LocomotionProfile` / presenter;
- presentation interpolation/reconciliation -> presentation/materialization layer;
- camera orbit/collision/recenter/lock-on -> `ThirdPersonCameraRig`;
- authoritative movement/location -> Simulation + protocol, then presentation samples;
- inventory/economy/social/politics/combat/magic/world consequence -> Simulation + protocol, not the control stack.

For any cross-boundary gameplay feature, use [`simulation-godot-boundary.md`](simulation-godot-boundary.md). A change that makes Godot decide world truth is in the wrong place even if it looks visually convenient.
