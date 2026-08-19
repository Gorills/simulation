# GDExtension adapter

Contract: TZ §10, ADR 0001. This layer is the **only** Godot ↔ simulation seam. It may include godot-cpp. It must not contain world laws.

Canonical sources: [About godot-cpp](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/about_godot_cpp.html), [GDExtension C++ example](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/gdextension_cpp_example.html), [godot-cpp README](https://github.com/godotengine/godot-cpp), [godot-cpp-template](https://github.com/godotengine/godot-cpp-template). Structural analog: [godot-jolt](https://github.com/godot-jolt/godot-jolt) (native library + thin Godot binding, CMake).

## What this layer is

```text
GDScript  --methods/signals-->  GDCLASS facade  -->  src/protocol  -->  sim_core
```

Internal C++ world types are **not** the Godot API. Export a small facade: submit intent, poll/push result, expose projections as typed properties or packed structs. Optional UTF-8 JSON envelopes are for debug/save/replay at the boundary, not for domain objects inside `src/sim`.

## How

**Pin godot-cpp to Godot 4.7.x**, the same series as the verified editor (TZ §2.5). Official compatibility: an extension built for a *newer* minor does not load in an older editor; building for 4.7 is correct for this machine. Do not follow a tutorial that says “use branch `4.x` / latest”.

**Register classes in one `register_types.cpp`**, like the official example: `GDREGISTER_CLASS` at the SCENE init level unless editor-only types need EARLY. Keep `entry_symbol` in the `.gdextension` manifest identical to the C function name.

**One CMake graph for `sim_core` + the extension shared library.** The official tutorial/template use SCons because that is Godot’s engine build. This repository already chose CMake/Ninja (TZ §2.3). Do not add a second SCons tree “because the template has SConstruct”. godot-cpp can be consumed from CMake; pin commit + hash in `cmake/Dependencies.cmake`.

**Manifest lives in the Godot project** (`godot/` + `.gdextension` pointing at the built `.so`). Debug/release and arch names must match what CMake actually emits.

**Lifetime: facade owns a `sim_core` instance, or a handle to one.** GDScript must not receive a raw pointer to `World` as a gameplay API. If a method would be `World* get_world()`, it is already wrong.

**Do not throw across the GDExtension boundary** for ordinary results. Translate `std::expected` into a Godot-friendly result object / error enum the facade returns. Catch unexpected exceptions at the adapter edge, log, return a failure result — never as “the trade succeeded”.

**godot-cpp vs custom C++ module:** official docs: GDExtension does not require compiling the engine; modules do. ADR 0001 already rejected modules. Do not start a `modules/` fork of Godot.

```cpp
// Good: thin bind, protocol types, no laws.
void SimFacade::_bind_methods() {
  ClassDB::bind_method(D_METHOD("submit_move", "dx", "dy"), &SimFacade::submit_move);
}

godot::Dictionary SimFacade::submit_move(int32_t dx, int32_t dy) {
  auto r = app_.handle(MoveIntent{dx, dy});  // src/protocol
  return to_godot(r);                        // projection only
}

// Bad: GDCLASS is the world; GDScript can poke gold.
GDCLASS(WorldNode, Node)
int gold;
void set_gold(int g) { gold = g; }
```

## How not

| Anti-pattern | Why |
| --- | --- |
| Export mutable `World*` / `Array` of internal structs by address | Lifetime coupling; GDScript becomes authority |
| Domain rules in `submit_*` “just this once” | Second implementation; native tests skip it |
| Autoload that steps the sim in `_process` without a command | Hidden ticks; TZ §8 |
| SCons + CMake both compiling the extension | Two sources of truth for flags and godot-cpp version |
| Targeting godot-cpp `master` | TZ: no floating branches |
| Compiling against a different float precision than the editor | Official: extension will not load |
| Copying Summator’s `_process` animation as the sim step | Tutorial is a node demo, not an architecture |

## Agent traps

- Including `<godot_cpp/...>` from `src/sim` because “it’s all C++”.
- Registering every domain class with ClassDB “for the inspector”.
- Using `memnew`/`memdelete` for sim objects (those are Godot heap APIs). Sim objects stay in `sim_core` with RAII.
- Reloading the extension in the editor and assuming static process state survived (debug reload is not a save format).
- Generating `extension_api.json` from a random Godot binary instead of the pinned 4.7.1 toolchain.