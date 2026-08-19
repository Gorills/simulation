# GDExtension adapter

This layer is the **only Godot ↔ application protocol runtime seam**. It may depend on godot-cpp. It must not contain world laws.

Canonical contracts:

- seam/ownership: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- Godot/API/godot-cpp versions: [`VERSIONS.md`](VERSIONS.md)
- protocol/model semantics: [`../MODELING.md`](../MODELING.md)
- integration verification: [`../VERIFICATION.md`](../VERIFICATION.md)

Primary references are tracked in [`SOURCES.md`](SOURCES.md): official Godot GDExtension docs/examples and current godot-cpp upstream.

## Boundary shape

```text
GDScript -> GDCLASS facade -> src/protocol -> Simulation Core
```

Internal C++ world types are not the Godot API. Export a small semantic facade: submit an intent, receive a result/projection/events, expose bounded diagnostics.

Optional JSON may be useful at debug/save/replay boundaries, but it must not become the live domain model inside `src/sim`.

## Versioning

Do **not** describe godot-cpp as `4.7.x`.

The selected dimensions are separate:

```text
Godot Engine baseline     4.7.1-stable
GDExtension API target    4.7
godot-cpp                 independent v10 line, exact immutable revision
```

The candidate/review/upgrade rules and upstream evidence live in [`VERSIONS.md`](VERSIONS.md). Build files/locks become authoritative for the active revision once bootstrap exists.

Never depend on floating `master`, `main` or `latest`.

## Class registration

Keep GDExtension registration centralized and intentionally small. Use ClassDB registration and the initialization level appropriate to the exposed runtime classes.

The `.gdextension` `entry_symbol` must exactly match the exported entry function and the library paths must match what the build actually emits.

Do not register every domain class with ClassDB for inspector convenience.

## One build graph

This project uses one CMake graph for the native core and the extension adapter.

Official godot-cpp tutorials/templates often demonstrate SCons; that is useful upstream context, not a requirement to introduce a second project build graph.

The GDExtension target links the exact pinned godot-cpp revision. Native simulation/protocol targets do not.

## Facade lifetime

A facade/application composition object may own or reference the application/simulation instance through normal C++ lifetime rules.

Do not expose mutable `World*` to GDScript. If the public API wants `get_world()` so scripts can mutate internals, the boundary is wrong.

## Results and exceptions

Ordinary gameplay failure is a protocol/domain result. Translate it to a Godot-friendly result/error representation.

Unexpected adapter exceptions may be caught at the adapter edge for diagnostics and converted to a safe failure, but do not throw gameplay outcomes across the GDExtension boundary.

```cpp
void SimFacade::_bind_methods() {
  ClassDB::bind_method(
      D_METHOD("submit_move", "dx", "dy"),
      &SimFacade::submit_move);
}

godot::Dictionary SimFacade::submit_move(int32_t dx, int32_t dy) {
  auto result = app_.handle(MoveIntent{dx, dy});
  return to_godot(result);
}
```

Bad:

```cpp
GDCLASS(WorldNode, Node)
int gold;
void set_gold(int value) { gold = value; }
```

## Manifest and compatibility

The `.gdextension` manifest belongs to the Godot project and points to the actual built library variants.

For an extension genuinely targeting the 4.7 API, use the compatible minimum/loader configuration established in [`VERSIONS.md`](VERSIONS.md). Engine and extension floating-point precision must match.

Do not generate API metadata from an arbitrary Godot binary. Custom-engine APIs must match the engine that will load the extension.

## GDExtension vs engine module

The project intentionally uses GDExtension rather than a custom Godot module: the game can use native C++ without maintaining a custom engine build.

Changing to an engine module is a consequential architecture change and requires evidence plus an ADR; it is not an implementation shortcut.

## Anti-patterns

| Anti-pattern | Why |
| --- | --- |
| export mutable `World*` / internal object graph | lifetime coupling and client authority |
| world rules in `submit_*` | second implementation skipped by native tests |
| Autoload that advances the sim implicitly each frame | hidden simulation advancement |
| SCons and CMake both compiling project extension | two build/version sources of truth |
| floating godot-cpp branch | unreproducible binding/API surface |
| wrong precision/API target | loader/ABI incompatibility risk |
| tutorial `GDCLASS` used as the domain world | binding layer becomes simulation |

## Agent traps

- Including `<godot_cpp/...>` from `src/sim` or `src/protocol` because “both are C++”.
- Using Godot `memnew`/`memdelete` for simulation objects.
- Assuming editor hot reload preserves authoritative static process state.
- Copying exact version statements from old tutorials instead of reading [`VERSIONS.md`](VERSIONS.md) and active build pins.
- Adding adapter-specific gameplay behavior because it is easier than extending the protocol.
