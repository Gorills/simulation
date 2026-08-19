# Godot 4 client — third-person 3D presentation

Godot is the interactive presentation/input/UI client. It owns latency-sensitive input, camera, rendering and presentation state; it does **not** own authoritative world state, including entity identity or location.

Canonical contracts:

- runtime boundary: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- Simulation ↔ Godot implementation route: [`simulation-godot-boundary.md`](simulation-godot-boundary.md)
- authoritative-world decision: [`../decisions/0004-authoritative-world-presentation-boundary.md`](../decisions/0004-authoritative-world-presentation-boundary.md)
- third-person control decision: [`../decisions/0002-third-person-controls.md`](../decisions/0002-third-person-controls.md)
- product/playable rule: [`../PRODUCT.md`](../PRODUCT.md)
- Godot/GDExtension versions: [`VERSIONS.md`](VERSIONS.md)
- verification: [`../VERIFICATION.md`](../VERIFICATION.md)

Primary Godot references are tracked in [`SOURCES.md`](SOURCES.md).

## Current composition

```text
Main
├── PlayerControls
├── WorldPresentation
│   └── Player (CharacterBody3D)       # presentation/prediction shell
│       ├── EntityBinding              # authoritative EntityId only
│       ├── CollisionShape3D
│       └── VisualRoot
├── CameraRig
│   └── PitchPivot
│       └── SpringArm3D
│           └── Camera3D
├── environment/demo collision
└── HUD
```

`Main` is the composition root. It obtains the native `ObservedWorldProjection`, asks `WorldPresentation` to bind the controlled presentation to the authoritative `EntityId`, then wires controls/player/camera.

Reusable components do not discover unrelated siblings through hardcoded `../../` paths.

## Authority boundary

Godot owns:

- keyboard/mouse/gamepad sampling;
- pointer capture;
- local presentation/prediction transform and collision response;
- camera orbit/collision;
- animation/audio/VFX/UI;
- presentation interpolation/prediction caches.

Simulation owns:

- entity identity/existence;
- authoritative location/movement results once the spatial capability exists;
- inventory/ownership/economy;
- relationships/social state;
- politics/institutions;
- combat consequences;
- magic consequences;
- simulation time/history.

A Godot transform cannot grant trade range, attack range, ownership, inventory, access, relationship changes or another systemic outcome.

The Milestone 0 `BootstrapMoveIntent`/grid path is transport evidence only. Do not extend it into production locomotion.

## Presentation identity owner

`WorldPresentation` is the only ordinary Godot owner of authoritative presentation identity/presence.

Current read flow:

```text
SimFacade.observed_world_projection()
  -> WorldPresentation.apply_observed_world_projection(...)
       -> validates EntityIds / revision ordering
       -> records observed identity set
       -> assigns controlled EntityId to EntityBinding
```

`EntityBinding` is a component under a presentation root. It stores the authoritative `EntityId`; it does not generate IDs or own world state.

Do not create additional `EntityId -> Node` dictionaries in combat, inventory, HUD, NPC or feature scripts. When real NPC/item materialization exists, extend the single presentation-lifecycle owner instead.

The current stage deliberately binds only the pre-existing controlled actor. Do not create a generic presentation factory until a second real materialized scene kind requires it.

## Control stack

```text
default_control_profile.tres
  -> PlayerControls
       -> move axis
       -> gamepad look axis
       -> accumulated mouse look
       -> sprint intent
       -> active-device signal

default_locomotion_profile.tres
  -> ThirdPersonPlayer

PlayerControls + Player
  -> ThirdPersonCameraRig
```

### ControlProfile

Owns input/camera feel only:

- movement/look deadzones;
- mouse sensitivity;
- controller look speed/response exponent;
- inversion;
- pitch limits;
- camera target height/distance;
- collision margin;
- FOV.

Tuning these values should normally be a `.tres` edit.

### LocomotionProfile

Owns presentation motor feel only:

- normal/sprint speed;
- acceleration/deceleration;
- direction-change acceleration;
- turn response;
- sprint analog threshold;
- floor snap/slope settings.

It is not authoritative world movement configuration. When the Simulation spatial model exists, separate authoritative movement constants/rules from presentation responsiveness.

### PlayerControls

Owns device translation only.

Use semantic InputMap actions. Do not put raw `KEY_W`, joystick-axis indices or direct mouse-button movement logic into `ThirdPersonPlayer`.

Movement uses Godot `Input.get_vector()` so the stick path has circular deadzone behavior. Mouse look uses captured `InputEventMouseMotion.screen_relative`; gamepad look is continuously sampled and delta-scaled.

Passive active-device tracking continues while modal UI is open so prompts can switch device. Gameplay intent is disabled through `PlayerControls.set_gameplay_enabled(false)` rather than scattered `if menu_open` checks.

Escape releases the pointer. Only an unhandled left click recaptures it; wheel/right/middle clicks do not.

## Third-person camera

Hierarchy:

```text
CameraRig (yaw)
  -> PitchPivot (pitch)
       -> SpringArm3D
            -> Camera3D
```

Keep `Camera3D` a direct spring-arm child unless a measured reason requires another setup. Exclude the player presentation collider from spring-arm collision.

Mouse displacement is not multiplied by frame delta. Gamepad angular velocity is multiplied by frame delta. Pitch is clamped; yaw is unbounded.

The current camera follows the player's interpolated Godot presentation transform. When Simulation-driven authoritative samples arrive, evaluate custom sample interpolation as described in [`simulation-godot-boundary.md`](simulation-godot-boundary.md) rather than assuming Godot physics interpolation is automatically the right clock.

## Current presentation locomotion

`ThirdPersonPlayer` is a `CharacterBody3D` presentation/prediction shell.

Current feel path:

1. read semantic movement intent from `PlayerControls`;
2. transform it into camera-relative horizontal direction;
3. preserve analog magnitude;
4. choose normal/sprint target speed;
5. apply separate acceleration, deceleration and direction-change response;
6. use `move_and_slide()` for the current local presentation shell;
7. turn the visual/body root toward travel direction.

This remains useful for control feel and collision/camera prototyping, but its transform is **not** authoritative world location.

Do not use `ThirdPersonPlayer.global_position` to decide trade success, interaction reachability, combat range, item pickup, trespass or any other Simulation rule.

## Future spatial migration

Production movement must become:

```text
PlayerControls semantic intent
  -> protocol actor movement intent
  -> Simulation authoritative spatial rule
  -> revisioned spatial samples/projection
  -> WorldPresentation / player presenter
  -> interpolation
  -> optional local prediction + reconciliation if playtest needs it
```

The authoritative collision/navigation representation is intentionally not selected yet. Choose it from real terrain/navigation/determinism/performance constraints.

Prediction must remain separate from authoritative samples and cannot predict systemic success such as damage, item transfer or access rights.

## Processing ownership

Current presentation processing:

- `ThirdPersonPlayer` local shell: `_physics_process()`;
- right-stick camera integration and camera follow: `_process()`;
- passive device tracking/captured mouse motion: `_input()`;
- pointer release/recapture after GUI handling: `_unhandled_input()`.

Neither Godot frame delta nor physics delta is Simulation time.

`WorldPresentation` applies explicit protocol projections. It must never advance Simulation because a frame happened.

## UI interaction

The project-wide design system remains the visual source of truth. Modal UI should disable gameplay intent through `PlayerControls`, manage pointer visibility deliberately, and submit semantic world commands rather than mutating presentation caches as gameplay truth.

UI projection caches are disposable read models. They are not the authoritative inventory/economy/social world.

## Typed GDScript and resources

Use typed parameters/returns/member state when the type is known. Boundary Dictionaries are acceptable at the GDExtension edge, but parse them in the relevant boundary/presentation owner instead of spreading string-key interpretation across feature scripts.

Shared `.tres` control profiles are project tuning assets, not save-game state.

Do not introduce a global mutable `GameState`/inventory/economy Resource or Autoload.

## Autoload policy

Autoloads are not the default dependency-injection mechanism and must never become a second world authority.

Potential future uses are small genuinely global presentation services such as user settings. Forbidden uses include authoritative `World`, inventory/economy dictionaries or a global event bus that becomes the ownership model for unrelated systems.

## Anti-patterns

| Anti-pattern | Why |
| --- | --- |
| one mega `player.gd` | mixes input/camera/presentation/gameplay responsibilities |
| raw device reads in motor | breaks remapping/device parity |
| separate keyboard/gamepad movement implementations | behavior drifts |
| Godot transform as world position | creates a second authority |
| Godot instance ID as Simulation EntityId | engine lifetime becomes domain identity |
| multiple feature-local EntityId registries | presentation identity drifts and dematerialization becomes unsafe |
| bootstrap grid projection driving production 3D position | temporary probe becomes architecture |
| frame/physics delta as Simulation clock | render rate becomes world law |
| UI Dictionary cache as inventory/economy truth | projection becomes mutable second world |
| generic materializer factory before real entity kinds | speculative framework with no current contract |

## Agent extension rule

Before editing Godot code, pick the smallest owner:

- binding/device translation/gameplay-input enable -> `PlayerControls` / InputMap;
- sensitivity/deadzone/inversion/FOV/camera distance -> `ControlProfile`;
- presentation acceleration/speed/turn/slope feel -> `LocomotionProfile`;
- current presentation motor/collision behavior -> `ThirdPersonPlayer`;
- camera behavior -> `ThirdPersonCameraRig`;
- authoritative presentation identity/presence/materialization -> `WorldPresentation` + protocol projection;
- visual entity identity storage -> `EntityBinding` (assigned only by `WorldPresentation`);
- inventory/economy/social/combat/politics/magic/location outcome -> **Simulation/protocol**, not the Godot control stack.

If a change needs a new local authoritative dictionary or another `EntityId -> Node` map, it is probably in the wrong place.
