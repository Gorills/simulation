# Godot 4 client (typed GDScript, 2D)

Contract: TZ §10–11. Engine docs used here are **Godot 4.7** (this repo’s verified editor). Godot owns presentation. It does not own inventory, money, jobs, relationships, or spell/trade outcomes.

Canonical sources: [Best practices index](https://docs.godotengine.org/en/stable/tutorials/best_practices/index.html), [Scene organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/scene_organization.html), [Autoloads vs regular nodes](https://docs.godotengine.org/en/stable/tutorials/best_practices/autoloads_versus_regular_nodes.html), [Node alternatives](https://docs.godotengine.org/en/stable/tutorials/best_practices/node_alternatives.html), [Project organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/project_organization.html), [Resources](https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html), [Idle vs physics](https://docs.godotengine.org/en/stable/tutorials/scripting/idle_and_physics_processing.html), [Input events / InputMap](https://docs.godotengine.org/en/stable/tutorials/inputs/inputevent.html), [Static typing](https://docs.godotengine.org/en/stable/tutorials/scripting/gdscript/static_typing.html).

## Tree that matches the official “Main / World / GUI” split

Godot’s own scene-organization article starts from a `Main` controller, a `World` child, and a `GUI` sibling — then **swap World children** on location change instead of `change_scene_to_file()` deleting the player. Do that here, adapted to a sim client:

```text
Main (main.gd)                    # composition root: wires adapter + scenes
├── World (Node2D)                # presentation of the current place
│   └── PlaceView / ActorViews    # visuals driven by projections
├── GUI (Control / CanvasLayer)   # HUD, dialogue, trade — display only
└── (optional Autoload)           # settings, debug overlay — not the world
```

Parent-child means **lifetime**, not map geometry. Official warning: if rooms are deleted on travel, **do not parent the player to the room** or you invent an undocumented special case. Keep the avatar under World (or a persistent Actors branch) and change place visuals beside it.

World **data** (village stocks, occupations) lives in C++ / `content/`, not under `godot/`.

## How

**Scenes are reusable and environment-agnostic.** Official rule: design scenes with no hard dependencies. Parents inject `@export` node refs, Callables, or connect signals. Children do not `get_node("../../HUD/Gold")`.

**Call down, signal up.** Official: siblings only know their own hierarchy; an **ancestor mediates**. Parents call child methods to start work. Children emit **past-tense** signals (`trade_offered`, `actor_selected`) to respond. Connect in the parent’s `_ready()`, with null-checks on `get_node` in C++ bindings; in GDScript cache `@onready` / `@export` refs.

**Typed GDScript.** Annotate parameters, returns, and member types. Prefer `PersonId` coming from the adapter over untyped `Dictionary` blobs in UI code. Do not use `Variant` to silence the analyzer.

**InputMap actions, not scancodes.** Official InputMap exists so keyboard/gamepad/remap share one code path. Gameplay scripts query `Input.is_action_*("move_east")`. Hardcoded `KEY_W` is a defect.

**`_physics_process` vs `_process`.** Official: physics-timestep work (moving a colliding body) in `_physics_process`; frame/visual work in `_process`. **Neither delta is simulation time.** Presentation may interpolate with delta; the core advances only on explicit ticks/commands.

**Resources are shared data containers.** Official: loading the same `.tres` returns the same object. Duplicate (or keep mutable runtime state on the node / in C++) if each actor needs its own numbers. Config stays immutable.

**Prefer a Resource or RefCounted over a Node** when there is no transform, process, or child to own (official node-alternatives). Do not spawn a Node per inventory slot.

**Autoloads are process-wide services**, not managers of everyone else’s data. Official audio-manager example: global `Sound.play()` hides the bug source and over-allocates. Prefer scene-local `AudioStreamPlayer`. Allowed autoloads here: debug overlay, settings. Forbidden: a Godot `World` that simulates.

**Filesystem names:** `snake_case` files/folders; `PascalCase` node names (official project organization). Keep third-party editor addons in `addons/`. Put a `.gdignore` on non-Godot trees that must not be imported (`docs/`, C++ `src/` if the Godot project root were ever the repo root — this repo’s Godot root is `godot/`, which is the correct isolation).

**Set node properties before `add_child` when spawning at runtime** (official logic preferences), except properties that require being in the tree (global transform).

```gdscript
# Good: parent owns wiring; child announces intent; gold comes from a projection.
func _ready() -> void:
    trade_panel.trade_offered.connect(_on_trade_offered)

func _on_trade_offered(item_id: int, count: int) -> void:
    var result := sim.submit_offer_trade(item_id, count)
    gold_label.text = str(result.projection.player_gold)

# Bad: scene assumes a unique tree; GDScript is money truth.
func _ready() -> void:
    get_node("/root/World").gold += 10
```

## How not

| Anti-pattern | Why |
| --- | --- |
| Autoload EventBus / `GameState` dictionary | Official: global access makes every script a suspect; TZ: second authority |
| `get_node("../../..")` across scenes | Official “development hell”: scenes cannot be instanced elsewhere |
| Mutating a shared `.tres` as per-NPC stats | Official: one in-memory Resource; all instances change |
| C# gameplay because the editor is Mono | TZ §2.5 / ADR 0001: GDScript + C++ only |
| `_process` as the simulation clock | Frame rate becomes world law |
| AnimationPlayer callback as the only place a trade resolves | Presentation miss = missing outcome |
| One mega-scene | Official: split; Main/World/GUI |
| ECS addon / Godot nodes as the sim | TZ forbids replacing the C++ core |
| Putting `content/village/` inside `godot/` | Godot import and C++ tests diverge |

## Agent traps

- Treating the Mono editor as permission to add `.cs`.
- Creating `Autoload World` “for convenience” on the first scene.
- Using `change_scene_to_file` for every room and losing GUI/player.
- `preload()` of the whole game in one script (official: unexpected load-time spikes).
- Groups scanned every frame (`get_nodes_in_group` in `_process`).
- Storing protocol JSON in node metadata as the live inventory.
- Connecting signals in the child to a hardcoded parent path.