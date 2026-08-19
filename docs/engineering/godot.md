# Godot 4 client — typed GDScript, 2D

Godot is the real presentation/input/UI client. It does not own authoritative inventory, money, jobs, relationships, ownership, semantic location or spell/trade outcomes.

Canonical contracts:

- runtime boundary: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- product/playable rule: [`../PRODUCT.md`](../PRODUCT.md)
- Godot/GDExtension version policy: [`VERSIONS.md`](VERSIONS.md)
- player-facing evidence: [`../VERIFICATION.md`](../VERIFICATION.md)

Primary Godot references are tracked in [`SOURCES.md`](SOURCES.md).

## Scene composition

Use a composition root following Godot's scene-organization guidance, adapted to an external authoritative simulation:

```text
Main                         # composition root / wiring
├── World (Node2D)           # current-place presentation
│   └── PlaceView/ActorViews # projection-driven visuals
└── GUI (Control/CanvasLayer)# HUD/dialogue/trade/presentation
```

Persistent actors/UI should not be parented to disposable place scenes merely because they are visually located there.

World **data** stays in the native simulation/content side, not in the Godot scene tree.

## Scene independence and wiring

Reusable scenes must not assume a unique parent hierarchy. Parents/composition roots provide dependencies and connect siblings.

Prefer:

- exported/typed node references where appropriate;
- cached `@onready` references for owned children;
- parent-mediated signal connections;
- past-tense semantic signals for child-to-parent notification.

Avoid hardcoded cross-scene `get_node("../../...")` paths.

A useful local direction is “parent calls down, child signals up”; the canonical authority is Godot's scene-organization guidance rather than the slogan itself.

## Typed GDScript

Type parameters, return values and member state when types are known. Do not fall back to untyped `Variant`/`Dictionary` merely to avoid design decisions.

Boundary DTO representation may initially use Godot-friendly values, but UI code should not preserve arbitrary untyped protocol blobs as live world state.

## InputMap

Gameplay input is expressed through semantic InputMap actions so keyboard/gamepad/remapping share the same client intent path.

Do not encode gameplay logic around raw keys such as `KEY_W`.

Input creates semantic protocol requests; it does not mutate world state directly.

## Frame and physics processing

Use `_physics_process` for presentation/physics work that requires the physics tick and `_process` for frame-driven presentation.

Neither `delta` is simulation time. Presentation may interpolate with frame time; authoritative advancement follows the native simulation contract in [`../MODELING.md`](../MODELING.md).

## Resources

Godot Resources are shared data containers. Loading the same resource may return the same in-memory object, so do not mutate shared `.tres` assets as per-NPC authoritative runtime state.

Use immutable/shared configuration where suitable; duplicate resources or use presentation/node state when instance-specific presentation mutation is actually required.

Do not mirror the authoritative C++ world into Resources.

## Node alternatives

Use `Resource`, `RefCounted` or plain values instead of a `Node` when the object needs no transform, processing callback or child ownership.

Do not create one Node per inventory slot/value object merely because the client uses Godot.

## Autoload policy

Autoloads are process-wide services, not a default dependency injection container or a second world authority.

Potentially appropriate uses are small genuinely global presentation/developer services such as settings or a debug overlay.

Forbidden patterns include an Autoload `World`, authoritative `GameState`, global inventory/economy dictionaries or a global EventBus that becomes the ownership model for unrelated scenes.

## Files and project layout

Use Godot's project organization conventions: snake_case files/folders and clear PascalCase scene node names.

Third-party editor addons belong in `addons/`. The Godot project root is `godot/`, keeping native source/docs outside import scanning.

## Spawn order

Set runtime node properties before `add_child()` when those properties do not require tree membership. Handle global-transform/tree-dependent values after insertion when necessary.

## Example boundary usage

```gdscript
func _ready() -> void:
    trade_panel.trade_offered.connect(_on_trade_offered)

func _on_trade_offered(item_id: int, count: int) -> void:
    var result := sim.submit_offer_trade(item_id, count)
    gold_label.text = str(result.projection.player_gold)
```

Bad:

```gdscript
func _ready() -> void:
    get_node("/root/GameState").gold += 10
```

## Anti-patterns

| Anti-pattern | Why |
| --- | --- |
| Autoload world / mutable GameState | creates second authority and global coupling |
| cross-scene hard NodePaths | scenes cannot be reused independently |
| shared `.tres` as per-NPC authoritative stats | shared Resource identity leaks state |
| C# gameplay because editor is Mono | selected project split is GDScript + C++ GDExtension |
| `_process`/`delta` as simulation clock | frame rate becomes a world law |
| AnimationPlayer callback as only place an outcome happens | presentation becomes authority |
| one mega-scene | poor scene independence/ownership |
| ECS addon/nodes as simulation | duplicates the C++ core |
| protocol JSON stored as node metadata for live inventory | client cache becomes domain truth |

## Agent traps

- Treating Mono capability as permission to add a C# gameplay layer.
- Creating `Autoload World` for first-step convenience.
- Replacing a persistent root scene during every location change and accidentally deleting persistent UI/player presentation.
- Scanning groups every frame without a measured need.
- Connecting a reusable child directly to a hardcoded parent path.
- Updating UI optimistically in a way that invents an authoritative outcome rather than waiting for the returned projection/result.
