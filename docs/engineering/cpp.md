# C++23 Simulation Core and protocol

Contract: TZ §3–9. Folder sketch: TZ §4. This page is implementation architecture, not a second product spec.

Canonical sources: [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines), [`std::expected`](https://en.cppreference.com/w/cpp/utility/expected), [GoogleTest primer](https://google.github.io/googletest/primer.html). Structural analog: [godot-jolt](https://github.com/godot-jolt/godot-jolt) keeps a Godot-free physics library and a separate binding — same split as `src/sim` vs `src/adapters/gdextension`.

## Layout that actually enforces direction

TZ names `domain / application / content / persistence / random / diagnostics` under `src/sim`. Those are **responsibility names**, not CMake packages.

| Folder | Owns | Must not |
| --- | --- | --- |
| `src/sim/domain` | types, invariants, state transitions | files, Godot, JSON, clocks, frame delta |
| `src/sim/application` | command handling, stepping the world | rendering, InputMap |
| `src/sim/content` | parsed, typed content after load | `nlohmann::json` as the live model |
| `src/sim/persistence` | save/load mapping | gameplay rules |
| `src/sim/random` | seeded PRNG **as world state** | `std::random_device`, hidden static RNG |
| `src/protocol` | intents, results, events, projections, version | Godot types, sprites, cameras |
| `src/adapters/*` | CLI / GDExtension translation | domain laws |

One CMake library `sim_core` (sim + protocol). Tests and adapters **link** it. Do not `add_subdirectory` a fake “package” per folder until a target actually forbids a dependency.

Headers live next to sources under `src/` (application layout). Do not invent a Pitchfork `include/` install tree until this is a published SDK.

## How

**Ownership (Core Guidelines R.1, I.11).** Every resource is a type. Value types by default. `std::unique_ptr` for unique heap ownership. `std::shared_ptr` only with proven shared lifetime. Raw `T*` / `T&` are non-owning views. No owning raw `new`/`delete` in project code.

**Ordinary domain failure is data.** C++23 `std::expected<T, E>` for “trade refused”, “not reachable”, “insufficient funds”. Exceptions are for broken invariants / adapter I/O, and **must not** cross the GDExtension C ABI as a gameplay outcome (TZ §6.2).

**Headers are self-contained (SF.11).** A header compiles alone. No `using namespace` at global scope in a header (SF.7). Include what you use. Forward-declare only when it is a real compile firewall, not cargo-cult Pimpl on every type.

**Prefer concrete types (TZ §6.5, Core Guidelines I.25).** No abstract base until a second real implementation exists, except the adapter/serialization surface that *is* the variation point.

**Determinism is a type-system problem (TZ §7).** Integer/`SimulationTick`/money/counts. `std::unordered_map` is lookup-only; never iterate it to decide an authoritative outcome. One seeded RNG in world state; systems borrow it, they do not construct their own.

**Protocol is a small application contract, not WorldState dumped to the client (TZ §9).** Client sends intents (`OfferTrade`). Core returns `CommandResult` + events + projections. Breaking protocol changes bump the version and native tests together.

**Tests prove rules without Godot.** GoogleTest isolates each test on a fresh fixture. Name suites/tests as valid identifiers **without underscores** in the `TEST()` arguments. Prefer `EXPECT_*` so one test can report several failures; use `ASSERT_*` only when continuing is meaningless (null deref). Link `GTest::gtest_main`. Do not write a custom `main` unless the test binary needs custom init that fixtures cannot express.

```cpp
// Good: intent in, expected out, no Godot, no json in the domain.
std::expected<TradeResult, TradeError>
execute_trade(World& world, const OfferTrade& cmd);

// Bad: desired state, JSON as model, exception as “not enough gold”.
void SetMoney(nlohmann::json& world, int gold);
```

## How not

| Anti-pattern | Why it fails here |
| --- | --- |
| `class World` god-object + service locator | Hidden mutable globals; TZ §6.4 forbids them; every system becomes untestable |
| ECS / job system / multithread sim on day one | TZ §34: no speculative architecture; first sim is single-threaded |
| `godot.hpp` / `Node` / `delta` in `src/sim` | Second authority; headless tests lie |
| `nlohmann::json` fields as inventory/money | Schema becomes the domain; upgrades become untyped |
| `shared_ptr<World>` everywhere | Ownership disappears; cycles; “who steps the world?” |
| `std::expected` *and* throwing `TradeError` | Two error channels; agents will mix them |
| Address-ordered sets / pointer identity as IDs | Non-deterministic across allocators |
| Header-only “framework” of templates | TZ §6.6; compile times and unreadability without a second use case |
| Pitchfork `include/` + installed export on Milestone 0 | We are not shipping a public C++ SDK yet |

## Agent traps

- Copying the official godot-cpp **Summator** into `src/sim` so a `GDCLASS` *is* the world.
- Using `std::chrono::system_clock::now()` or `delta` as simulation time (TZ §8: integer ticks).
- Iterating `unordered_map` to pick “the first NPC”.
- Adding a second RNG “just for this NPC”.
- Creating `IRepository` / `IWorld` with one implementation.
- Putting save-format code inside `execute_trade`.
- Testing Godot scenes to prove a C++ rule.

## CMake shape for the library

```cmake
add_library(sim_core STATIC)
target_sources(sim_core PRIVATE
  src/sim/application/step.cpp
  src/protocol/command.cpp
)
target_include_directories(sim_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(sim_core PUBLIC cxx_std_23)
# warnings: TZ §6.7, target-scoped, not inherited by godot-cpp / gtest
```

List sources explicitly. See [`cmake-python.md`](cmake-python.md).