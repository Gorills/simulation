# Runtime architecture

This document owns **runtime boundaries, dependency direction, ownership and integration seams**. It does not own product goals, modeling policy, verification procedure or tool versions.

Related canonical owners:

- product and playable invariants: [`PRODUCT.md`](PRODUCT.md)
- modeling/determinism: [`MODELING.md`](MODELING.md)
- verification/playtest evidence: [`VERIFICATION.md`](VERIFICATION.md)
- milestones: [`ROADMAP.md`](ROADMAP.md)
- stack implementation guidance: [`engineering/STACK.md`](engineering/STACK.md)
- Godot/GDExtension versions: [`engineering/VERSIONS.md`](engineering/VERSIONS.md)
- consequential decisions: [`decisions/`](decisions/)

Current source, build configuration, executable tests and lock files are authoritative for actual implemented behavior. A diagram is not proof that a planned path or target already exists.

## Runtime dependency graph

The intended graph is one-way:

```text
Godot 4 client
    |
    v
GDExtension adapter
    |
    v
Application protocol
    |
    v
C++23 Simulation Core

Native tools/tests ---> Application protocol / Simulation Core
Content data -------> Simulation Core
```

Hard direction:

```text
godot/ -> src/adapters/gdextension -> src/protocol -> src/sim

src/sim      !-> Godot / godot-cpp / GDExtension
src/protocol !-> Godot / godot-cpp / GDExtension
godot/       !-> authoritative world state
```

Folder names do not enforce architecture. Real CMake targets/import checks must prove the edges.

## Implemented Milestone 0 target graph

The first executable graph now exists:

```text
sim_tests ----------------> sim_core
                                ^
                                |
world_sim_gdextension ----------+----> godot::cpp
        ^
        |
Godot project / world_sim.gdextension
```

- `sim_core` is the C++23 Godot-free project library.
- `sim_tests` links `sim_core` + `GTest::gtest_main` and does not link Godot.
- `world_sim_gdextension` links `sim_core` + `godot::cpp`; it is the only project target with the Godot binding dependency.
- `architecture_no_godot_in_core` runs `tools/check_architecture.py` through CTest and rejects Godot/godot-cpp include markers under `src/sim` and `src/protocol`.

The first protocol path is `MoveIntent -> MoveOutcome -> PlayerProjection`. `World::move` accepts exactly one cardinal step, mutates authoritative coordinates/tick and returns the resulting projection. Invalid movement is rejected without changing the world.

## Ownership by layer

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | world state, laws, deterministic outcomes, seeded RNG state, domain/application behavior | Godot types, input, frames, UI, wall-clock gameplay truth |
| `src/protocol` | commands/intents, results, events, projections, versioned boundary DTOs | rendering, scene state, duplicated domain rules |
| `src/adapters/gdextension` | translation between Godot-facing values and protocol API; GDExtension registration | world laws or alternate gameplay truth |
| `godot/` | scenes, input, camera, audio, animation, UI, presentation state | authoritative money, inventory, relationships, ownership, spell/trade outcomes |
| native tools/tests | scenarios, diagnostics, verification, developer orchestration | a second simulator or alternate gameplay implementation |

The authoritative world exists once: in the C++ Simulation Core. Presentation prediction/interpolation may exist only when it cannot create authoritative outcomes.

## Protocol boundary

Clients express **intent**, never desired state:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

The protocol is a small application contract, not an exported `WorldState`. Internal simulation types do not automatically become public/client types.

Breaking protocol changes update the explicit protocol version and affected native/client verification together.

See [`MODELING.md`](MODELING.md#protocol-semantics) for semantic rules.

## GDExtension seam

Godot crosses into native gameplay through exactly one runtime seam: the GDExtension adapter.

The adapter should be deliberately boring:

1. receive a semantic client request;
2. validate/translate Godot-facing values into protocol values;
3. invoke the application/protocol surface;
4. translate results/projections/events back to Godot-facing values;
5. expose diagnostics without embedding world rules.

The Milestone 0 `SimFacade` follows that contract: it owns the native `sim::World`, exposes `submit_move` and read-only `debug_projection`, and converts only returned protocol projections into Godot dictionaries.

If a gameplay rule is implemented inside a `GDCLASS`, GDScript node, UI script or serialization helper, the boundary is probably being violated.

The adapter may depend on godot-cpp. `src/sim` and `src/protocol` may not. Version rules live only in [`engineering/VERSIONS.md`](engineering/VERSIONS.md).

## Godot client architecture

Godot is the real reference client, not merely a debug visualizer, but it remains presentation/application input rather than world authority.

Prefer a composition root shaped around persistent application/UI ownership and replaceable world views, following Godot scene-organization guidance. Scene independence, parent-mediated wiring, typed GDScript, InputMap and restrained Autoload usage are detailed in [`engineering/godot.md`](engineering/godot.md).

The first client creates `SimFacade`, maps InputMap actions to semantic moves and renders only the returned native projection. Its automated smoke scenario exercises that same path and captures read-only projection/screenshot evidence.

Do not let a convenient Autoload, Resource or UI model become an authoritative parallel inventory/economy/social state.

## Vertical capability rule

Normal gameplay work follows the product contract:

```text
minimal world rule
  -> protocol command/result/projection
  -> GDExtension translation
  -> Godot affordance/feedback
  -> focused deterministic/regression proof
  -> bounded real playtest
```

A coherent capability may touch several layers. It must not broaden into unrelated subsystem work.

## External AI Layer boundary

`Gorills/ai-layer` is the development control plane, not a runtime/build dependency of the game.

AI Layer may own durable Work/Task/Epic state, Project Map, Knowledge, project Decisions storage and project skills outside this repository. This repository owns source, tests, build configuration, product/modeling/runtime contracts, committed ADRs and short host bootstrap files.

Therefore:

- do not add repository-local `.ai-layer/` state;
- do not commit AI Layer databases, registry data or copied Work/Task/Epic state;
- do not reimplement AI Layer continuation or managed workflow in repository docs;
- do not link/import `ai-layer` into game/runtime/build targets;
- project skills are materialized by AI Layer into host-native catalogs outside the repository rather than copied here;
- AI Layer Knowledge/Decision records provide durable context but do not silently override current source or committed ADRs.

See [`AGENT_CONTEXT.md`](AGENT_CONTEXT.md) for how agent instructions are packaged without conflicting with this boundary.

## Repository shape

The Milestone 0 paths below now exist; `content/`, mechanic models/research and additional adapters remain future-on-demand areas:

```text
src/
  sim/
  protocol/
  adapters/
    gdextension/

godot/
tests/
tools/
cmake/

docs/
  INDEX.md
  PRODUCT.md
  ARCHITECTURE.md
  MODELING.md
  VERIFICATION.md
  ROADMAP.md
  AGENT_CONTEXT.md
  engineering/
  decisions/
  models/       # when serious mechanic contracts exist
  research/     # when load-bearing research artifacts exist
```

Do not create future directories solely to satisfy this picture. Establish physical boundaries when real code or evidence needs them.

## Mechanical architecture verification

Current executable checks establish the first load-bearing boundaries:

- CMake target dependencies keep native tests Godot-free and isolate godot-cpp to `world_sim_gdextension`;
- `tools/check_architecture.py`/CTest rejects direct Godot include markers in `src/sim` and `src/protocol`;
- native tests prove the first command-to-core behavior independently from Godot;
- the smoke playtest is designed to prove Godot load plus a real protocol/projection round-trip once run in the pinned local environment.

As the graph grows, add narrow checks for new real dependency edges. Do not add speculative architecture tooling before there is code to check, but do not leave a mechanically checkable invariant as prose once the relevant targets exist.
