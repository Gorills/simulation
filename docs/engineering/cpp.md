# C++23 Simulation Core and protocol

This guide owns C++ implementation practice for the Godot-free simulation/protocol side.

Canonical contracts:

- runtime ownership/dependencies: [`../ARCHITECTURE.md`](../ARCHITECTURE.md)
- determinism/protocol/modeling: [`../MODELING.md`](../MODELING.md)
- verification: [`../VERIFICATION.md`](../VERIFICATION.md)
- product vertical-slice rule: [`../PRODUCT.md`](../PRODUCT.md)

Primary references are tracked in [`SOURCES.md`](SOURCES.md): C++ Core Guidelines, `std::expected`, GoogleTest and the Godot-free-core/thin-binding structural precedent.

## Initial physical shape

The target responsibility layout is:

| Area | Owns | Must not own |
| --- | --- | --- |
| `src/sim/domain` | domain value types, invariants, state transitions | filesystem, Godot, JSON, clocks, frame delta |
| `src/sim/application` | command handling and explicit simulation advancement | rendering/input |
| `src/sim/content` | validated typed content | live `nlohmann::json` domain state |
| `src/sim/persistence` | snapshot/save-load mapping | gameplay laws |
| `src/sim/random` | explicit seeded PRNG state/helpers | hidden/system RNG |
| `src/protocol` | intents/commands, results, events, projections, protocol version | Godot types or rendering concepts |
| `src/adapters/*` | external translation | domain laws |

These are responsibility names, not a mandate to create one target/package per folder. Start with the smallest target graph that mechanically preserves the dependency boundary.

Headers may live beside implementation under `src/` until there is a real public SDK/install requirement. Do not build an `include/` export structure for aesthetics alone.

## Ownership and lifetime

Follow RAII and make ownership visible in types.

- Value semantics by default.
- `std::unique_ptr` when unique heap ownership is genuinely required.
- `std::shared_ptr` only for proven shared lifetime.
- Raw `T*` / `T&` are non-owning views.
- Avoid owning raw `new`/`delete` in project code.

Do not use Godot allocation APIs for simulation objects.

## Ordinary domain failure is data

Expected gameplay outcomes such as insufficient funds, unavailable stock, missing permission or unreachable targets should use a typed result channel such as C++23 `std::expected<T, E>`.

Exceptions may represent exceptional adapter/I/O failures or broken assumptions, but ordinary gameplay results must not be encoded through exceptions and exceptions must not leak across the GDExtension boundary.

```cpp
std::expected<TradeResult, TradeError>
execute_trade(World& world, const OfferTrade& command);
```

Avoid two simultaneous ordinary error channels for the same operation.

## Domain types

Use strong types where confusing primitives would create real bugs, for example `Money`, `SimulationTick`, `PersonId`, `HouseholdId`, `PlaceId`, `ItemCount`.

Do not wrap every primitive merely to create ceremony.

## Global state

No mutable global world, service locator, hidden static RNG, mutable runtime registry or initialization-order-dependent singleton.

Read-only compile-time constants are fine.

## Interfaces and abstraction

Prefer concrete types until there is a second real implementation or a boundary is inherently replaceable (for example an external adapter/storage boundary).

Do not create `IWorld`, generic repositories, strategy hierarchies or metaprogramming frameworks in anticipation of unknown future needs.

## Headers

Headers are self-contained, include what they use, and avoid `using namespace` at global scope. Use forward declarations only when they provide a real compile firewall.

## Deterministic coding

The detailed contract is in [`../MODELING.md`](../MODELING.md).

In authoritative code:

- one explicit seeded RNG state;
- no `std::random_device` or wall-clock gameplay input;
- no authoritative dependence on `unordered_map` iteration order;
- no address-based ordering/identity;
- integer/scaled domain quantities for load-bearing money/time/counts by default;
- single-threaded initial core; parallelism only after measured need and proof of result stability.

## Protocol coding

The client sends semantic intent; it never sets desired world truth.

Good:

```cpp
std::expected<TradeResult, TradeError>
execute_trade(World&, const OfferTrade&);
```

Bad:

```cpp
void set_money(World&, int new_money);
```

Internal `World` object graphs are not protocol DTOs. Keep projections deliberately bounded and purpose-specific.

## Native tests

World rules are proved without Godot. Link GoogleTest's provided test main unless a real custom process-level setup is needed.

Tests are independent and start from explicit state/fixtures. Prefer `EXPECT_*` when later assertions remain meaningful; use `ASSERT_*` to stop before invalid continuation.

Detailed evidence rules live in [`../VERIFICATION.md`](../VERIFICATION.md).

## Warnings and formatting

Project targets should enable a strict warning set appropriate to GCC/Clang, including common correctness/conversion/shadow/format warnings. Keep warnings target-scoped so third-party code does not inherit project `-Werror` policy.

Once available, use one project `clang-format` configuration and targeted `clang-tidy` checks. Do not mass-format unrelated files inside a gameplay change.

Exact tool availability/versions must be proven by the current environment and lock/build files, not this prose.

## Anti-patterns

| Anti-pattern | Why it fails here |
| --- | --- |
| god-object `World` + service locator | hidden coupling/state and poor testability |
| ECS/job system on day one | speculative scale architecture |
| Godot headers/`Node`/frame `delta` in `src/sim` | breaks headless authority boundary |
| `nlohmann::json` as live inventory/money/domain model | serialization representation becomes untyped domain truth |
| `shared_ptr<World>` everywhere | ownership/step authority becomes unclear |
| exceptions for normal gameplay refusal | two incompatible control/error channels |
| address/pointer identity as domain ID/order | non-deterministic/lifetime-coupled semantics |
| template framework before second use | complexity without current payoff |

## Minimal CMake shape

```cmake
add_library(sim_core STATIC)
target_sources(sim_core PRIVATE
  src/sim/application/step.cpp
  src/protocol/command.cpp
)
target_include_directories(sim_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(sim_core PUBLIC cxx_std_23)
```

List sources explicitly. Build/tool policy lives in [`cmake-python.md`](cmake-python.md).
