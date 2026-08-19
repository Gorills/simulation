# C++23 Simulation Core and protocol

This guide owns C++ implementation practice for the Godot-free Simulation/protocol side.

Canonical contracts:

- runtime ownership/dependencies: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- authoritative Simulation ↔ Godot boundary: [`simulation-godot-boundary.md`](simulation-godot-boundary.md)
- determinism/protocol/modeling: [`../MODELING.md`](../MODELING.md)
- verification: [`../VERIFICATION.md`](../VERIFICATION.md)
- product vertical-slice rule: [`../PRODUCT.md`](../PRODUCT.md)

Primary references are tracked in [`SOURCES.md`](SOURCES.md): C++ Core Guidelines, `std::expected`, GoogleTest and the architecture references behind the Simulation/presentation boundary.

## Current physical shape

The implemented native dependency graph is deliberately small:

```text
src/sim/*          -> sim_core
                         ^
                         |
src/protocol/*     -> sim_protocol
                         ^
                         |
GDExtension ------------+----> godot::cpp
```

Current responsibilities:

| Area | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | authoritative world entities/state, domain value types, invariants, transitions, simulation time/revision | protocol DTOs, Godot, input devices, JSON UI models, render/frame clocks |
| `src/protocol` | external control binding, intents/commands, boundary validation/translation, results/events/projections, protocol version | Godot types, rendering, duplicated world laws |
| `src/adapters/*` | external translation | domain laws or direct world ownership |

Do not create `src/sim/domain`, `application`, `services`, `repositories` or another layer merely because a generic architecture diagram uses those names. Split a real responsibility only when current code needs the boundary.

Headers live beside implementation under `src/` until there is a real public SDK/install requirement. Do not build an `include/` export structure for aesthetics alone.

## World authority and actor identity

The Simulation Core models actors independently of who controls them.

Stable actor/entity identity uses `sim::EntityId`. Human control belongs at the protocol/session boundary; NPC decisions and human input must feed the same relevant world operations.

Do not create a special C++ `Player` domain class merely to expose player-only variants of trade, ownership, relationships, institutions or combat.

The current `World` already demonstrates the rule: it owns actors by `EntityId`, while `protocol::Simulation` binds the bootstrap controller to one actor externally.

## Simulation time vs mutation order

Keep these meanings separate:

```text
SimulationTick  = authoritative world-time progression
WorldRevision   = monotonic authoritative state revision/order
```

An immediate action may advance revision without advancing time. An explicit time step advances Simulation time and may eventually trigger multiple state transitions.

Do not use command count, render frame count or Godot physics frame as Simulation time.

## Ordinary domain failure is data

Expected gameplay outcomes such as insufficient funds, unavailable stock, missing permission or unreachable targets should use a typed result channel such as C++23 `std::expected<T, E>`.

Exceptions may represent exceptional adapter/I/O failures or broken assumptions, but ordinary gameplay results must not be encoded through exceptions and exceptions must not leak across the GDExtension boundary.

```cpp
std::expected<TradeResult, TradeError>
execute_trade(World& world, const OfferTrade& command);
```

Avoid two simultaneous ordinary error channels for the same operation.

## Domain types

Use semantic types where confusing primitives would create real bugs, for example `EntityId`, `SimulationTick`, `WorldRevision`, `WorldSeed`, and later mechanic-specific money/count/location IDs or values when those distinctions become load-bearing.

Do not wrap every primitive merely to create ceremony.

Do not use Godot instance IDs, NodePaths, memory addresses or vector indices as persistent domain identity.

## Domain operation shape

Prefer operations that express the world capability and make invalid states difficult to represent.

Current durable example:

```cpp
world.spawn_actor(EntityId{...});
```

Current intentionally temporary example:

```cpp
world.apply_bootstrap_step(actor_id, CardinalDirection::east);
```

The word `bootstrap` is deliberate: this cardinal-grid operation exists only for the Milestone 0 transport probe and must not grow into production 3D locomotion.

When a real mechanic arrives, introduce its semantic state/operation from the mechanic requirements rather than generalizing the bootstrap probe.

## Global state

No mutable global world, service locator, hidden static RNG, mutable runtime registry or initialization-order-dependent singleton.

Read-only compile-time constants are fine.

## Ownership and lifetime

Follow RAII and make ownership visible in types.

- Value semantics by default.
- `std::unique_ptr` when unique heap ownership is genuinely required.
- `std::shared_ptr` only for proven shared lifetime.
- Raw `T*` / `T&` are non-owning views.
- Avoid owning raw `new`/`delete` in project code.

Do not use Godot allocation APIs for simulation objects.

## Interfaces and abstraction

Prefer concrete types until there is a second real implementation or a boundary is inherently replaceable.

Do not create `IWorld`, generic repositories, entity-component frameworks, strategy hierarchies or metaprogramming frameworks in anticipation of unknown future needs.

The protocol/GDExtension boundary is already the replaceable presentation seam; the domain does not need an interface around itself merely to look abstract.

## Headers

Headers are self-contained, include what they use, and avoid `using namespace` at global scope. Use forward declarations only when they provide a real compile firewall.

Do not expose a broad `World::all_entities()`/mutable collection simply because a client might want data later. Add purpose-specific domain/query access when a real protocol projection needs it; this discourages serializing the whole world into Godot.

## Deterministic coding

The detailed contract is in [`../MODELING.md`](../MODELING.md).

In authoritative code:

- one explicit seeded RNG state;
- stable explicit IDs;
- no `std::random_device` or wall-clock gameplay input;
- no authoritative dependence on `unordered_map` iteration order;
- no address-based ordering/identity;
- integer/scaled domain quantities for load-bearing money/time/counts by default;
- single-threaded initial core; parallelism only after measured need and proof of result stability.

## Protocol coding

Protocol has two jobs that must not be conflated:

```text
write side: semantic intent -> validation/translation -> authoritative operation -> result/events
read side:  authoritative state -> purpose-built read-only projection
```

We adapt this command/read-model separation because it fits the game boundary. We do **not** import a distributed CQRS framework, message broker or second data store.

Good command shape:

```cpp
std::expected<TradeResult, TradeError>
execute_trade(World&, const BuyItem&);
```

Bad:

```cpp
void set_money(World&, int new_money);
void set_relationship(World&, int value);
```

Internal `World` object graphs are not protocol DTOs. Keep projections deliberately bounded and purpose-specific.

Protocol/session code may bind an external human controller to an `EntityId`. That binding is not an `is_player` world rule.

## Domain events

Domain events describe facts that happened after successful authoritative transitions. They can support feedback, causal explanation, tests and projection updates.

Do not make every setter emit a generic string event. Prefer typed events when a real mechanic needs them.

Do not adopt full event sourcing merely because events exist; current authoritative state/snapshots remain the persistence model until a separate need is proven.

## Native tests

World rules are proved without Godot. Link GoogleTest's provided test main unless a real custom process-level setup is needed.

Tests are independent and start from explicit state/fixtures. Prefer `EXPECT_*` when later assertions remain meaningful; use `ASSERT_*` to stop before invalid continuation.

Important boundary regressions should prove concepts, not implementation trivia. Current examples include:

- two different actor IDs use the same domain operation;
- failed actor lookup does not mutate revision;
- actor action changes revision but not world time;
- explicit time advancement changes `SimulationTick`;
- protocol validation failure leaves the authoritative projection unchanged.

Detailed evidence rules live in [`../VERIFICATION.md`](../VERIFICATION.md).

## Warnings and formatting

Project targets enable a strict warning set appropriate to GCC/Clang/MSVC. Keep warnings target-scoped so third-party code does not inherit project warning policy.

Once available, use one project `clang-format` configuration and targeted `clang-tidy` checks. Do not mass-format unrelated files inside a gameplay change.

Exact tool availability/versions must be proven by the current environment and lock/build files, not this prose.

## Anti-patterns

| Anti-pattern | Why it fails here |
| --- | --- |
| special `Player` world class/branch for ordinary actor laws | breaks player/NPC parity |
| god-object `World` exposed wholesale to protocol/client | leaks ownership and encourages a universal world dump |
| ECS/job system on day one | speculative scale architecture |
| Godot headers/`Node`/frame `delta` in `src/sim` | breaks headless authority boundary |
| `nlohmann::json` as live inventory/money/domain model | serialization representation becomes untyped domain truth |
| `shared_ptr<World>` everywhere | ownership/step authority becomes unclear |
| exceptions for normal gameplay refusal | incompatible control/error channels |
| address/pointer/scene identity as domain ID/order | non-deterministic/lifetime-coupled semantics |
| command count used as Simulation time | action frequency becomes a world clock |
| generic framework before second use | complexity without current payoff |

## Actual CMake shape

The repository intentionally keeps domain and protocol as separate targets:

```cmake
add_library(sim_core STATIC)
target_sources(sim_core PRIVATE src/sim/world.cpp ...)

add_library(sim_protocol STATIC)
target_sources(sim_protocol PRIVATE src/protocol/simulation.cpp ...)
target_link_libraries(sim_protocol PRIVATE sim_core)

add_library(world_sim_gdextension SHARED ...)
target_link_libraries(world_sim_gdextension PRIVATE sim_protocol godot::cpp)
```

Do not put `src/protocol/*.cpp` into `sim_core`: that reverses the architecture the targets are meant to encode.

List sources explicitly. Build/tool policy lives in [`cmake-python.md`](cmake-python.md).
