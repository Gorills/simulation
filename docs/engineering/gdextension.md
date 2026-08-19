# GDExtension adapter

This layer is the **only Godot ↔ application protocol runtime seam**. It may depend on godot-cpp. It must not contain world laws.

Canonical contracts:

- seam/ownership: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- authoritative Simulation ↔ Godot contract: [`simulation-godot-boundary.md`](simulation-godot-boundary.md)
- Godot/API/godot-cpp versions: [`VERSIONS.md`](VERSIONS.md)
- protocol/model semantics: [`../MODELING.md`](../MODELING.md)
- integration verification: [`../VERIFICATION.md`](../VERIFICATION.md)

Primary references are tracked in [`SOURCES.md`](SOURCES.md).

## Boundary shape

```text
GDScript -> GDCLASS facade -> src/protocol -> Simulation Core
```

Internal C++ world types are not the Godot API. Export a small semantic facade: submit an intent, receive a result/projection/events, expose bounded diagnostics.

The adapter is a translation boundary: Godot values do not leak into the domain, and domain storage objects do not leak into Godot. It is not a place for world orchestration.

## Versioning

Keep these dimensions separate:

```text
Godot Engine baseline     4.7.1-stable
GDExtension API target    4.7
godot-cpp                 independent v10 line, exact immutable revision
```

Active pins and upgrade evidence live in [`VERSIONS.md`](VERSIONS.md) and machine-readable build/tool files. Never depend on floating `master`, `main` or `latest`.

The application protocol version is owned by `src/protocol/version.hpp`; bootstrap DTOs do not own the shared version number.

## Class registration

Keep registration centralized and intentionally small. Do not register every domain class with ClassDB for inspector convenience.

The `.gdextension` `entry_symbol` must match the exported entry function and the manifest library paths must match actual CMake output.

## One build graph

This project uses one CMake graph. `world_sim_gdextension` links `sim_protocol` + the exact pinned godot-cpp revision. `sim_core`/`sim_protocol` remain Godot-free.

Do not introduce a second project build graph because an upstream tutorial uses SCons.

## Facade lifetime

A facade may own the application/protocol composition object through normal C++ lifetime rules.

Do not expose mutable `World*` to GDScript. If the public API wants `get_world()` so scripts can mutate internals, the boundary is wrong.

The current Milestone 0 `SimFacade` owns `protocol::Simulation`, not `sim::World`. Its hardcoded controlled actor is bootstrap/session setup, not a domain `Player` identity.

## Results, projections and errors

Ordinary gameplay refusal is a typed protocol/domain result. Translate it without recomputing the rule.

Purpose-built projections are translated field-for-field into Godot-facing values. Do not add hidden information or reconstruct domain meaning in the adapter.

The current Godot-facing API deliberately separates durable read shape from temporary smoke code:

```text
observed_world_projection()     -> production-shaped identity/presence read model
bootstrap_submit_move(dx, dy)   -> Milestone 0 transport probe only
bootstrap_debug_projection()    -> Milestone 0 transport/debug probe only
```

`ObservedWorldProjection` currently carries only `controlled_actor_id`, `SimulationTick`, `WorldRevision`, protocol version, and the minimum observed `EntityId` set. It deliberately does **not** export bootstrap grid coordinates.

`BootstrapMoveIntent` / `BootstrapActorProjection` remain transport evidence, not templates for production spatial movement.

When real capabilities arrive, prefer semantic protocol operations such as `buy_item`, `attack`, or a real controlled-actor movement intent over extending the bootstrap DTO.

Unexpected adapter exceptions may be caught at the adapter edge for diagnostics and converted to a safe failure, but ordinary gameplay outcomes must not cross the boundary as exceptions.

## Entity identity

Carry stable Simulation `EntityId` values across the seam for materialized entities.

Do not translate Simulation identity into Godot instance IDs, NodePaths, scene names or unstable array positions. Godot scene-node lifetime is presentation lifetime, not world lifetime.

Godot applies observed identity through `WorldPresentation`; feature scripts must not create their own authoritative-ID registries beside it.

## Time and revision

Preserve `SimulationTick` and `WorldRevision` as different values when a projection exposes them.

The adapter must not substitute frame counters, physics frames or local timestamps. Godot uses authoritative revision/tick metadata for ordering/presentation; it does not define those values.

## Manifest and compatibility

The current manifest targets the 4.7 API floor, uses `world_sim_library_init` as the entry symbol and maps debug/release platform libraries to filenames emitted by CMake.

Engine and extension floating-point precision must match. Custom-engine APIs must match the engine that loads the extension.

## Anti-patterns

| Anti-pattern | Why |
| --- | --- |
| export mutable `World*` / internal object graph | client acquires authority/lifetime coupling |
| serialize the entire `WorldState` for convenience | leaks hidden/internal state and encourages a second mutable world |
| world rules in `submit_*` | second implementation skipped by native tests |
| adapter computes price/relationship/combat outcome | translation seam becomes gameplay authority |
| Godot scene spawn treated as authoritative entity creation | scene lifetime becomes world lifetime |
| frame callback implicitly advances Simulation | hidden world clock |
| Godot instance ID used as `EntityId` | engine lifetime becomes domain identity |
| generic Dictionary stored as live Godot world model | projection cache becomes second authority |

## Agent extension rule

When adding a cross-boundary feature:

1. define/verify the authoritative Simulation operation/state;
2. expose a semantic protocol intent/result/events/projection;
3. make the adapter translate only those values;
4. make Godot render/react to the result through the relevant presentation owner;
5. prove the native transition and the real Godot round trip.

If step 3 starts deciding what happened, move that decision behind the protocol.
