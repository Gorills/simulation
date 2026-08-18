# Development Rules

**Status:** ACTIVE

These are the canonical engineering rules for the current project stack. They are constraints for implementation, not proof that the corresponding files/tools already exist.

## 1. Authority and dependency direction

There is one authoritative implementation of world rules: the **C++23 Simulation Core**.

Dependency direction:

```text
presentation -> protocol -> simulation
simulation !-> presentation
```

The Simulation Core must not depend on DOM, Canvas, browser events, keyboard/mouse input, camera state, frame time/FPS, wall-clock time, filesystem/network as gameplay inputs, UI state, locale, or environment variables as sources of world rules.

TypeScript may own input mapping, rendering, animation, camera, UI panels and other presentation state. It must not own authoritative inventory, money, relationships, status, skills, semantic location, ownership, institution membership, spell outcomes or trade outcomes.

## 2. C++

- Language standard: C++23.
- Required CMake settings:

```text
CMAKE_CXX_STANDARD = 23
CMAKE_CXX_STANDARD_REQUIRED = ON
CMAKE_CXX_EXTENSIONS = OFF
```

- Prefer value semantics and RAII.
- Use `std::unique_ptr` only for real heap ownership.
- Use `std::shared_ptr` only for demonstrated shared ownership.
- Raw pointers/references are non-owning views; owning raw pointers and ordinary manual `new/delete` are forbidden.
- Expected gameplay/domain failures are data, normally expressed with typed results such as `std::expected`, not exceptions.
- Use strong domain types where mixing primitive values would cause real bugs.
- No mutable global world state, service locator, mutable singleton, hidden static RNG, or mutable global registry.
- Prefer concrete types. Do not create an interface/abstract base class before a second real implementation/use case unless an adapter boundary inherently requires one.
- Avoid metaprogramming/framework abstractions without current payoff.

### Warnings

Project C++ should use strict target-scoped warnings for GCC/Clang:

```text
-Wall
-Wextra
-Wpedantic
-Wconversion
-Wsign-conversion
-Wshadow
-Wformat=2
-Wundef
-Wnon-virtual-dtor
-Wold-style-cast
```

`-Werror` may be used in a controlled local verification preset for project code, but third-party warnings must not become project errors.

Do not mass-format unrelated files in a bounded task.

## 3. Build system and C++ dependencies

Use:

- CMake;
- Ninja;
- CMake Presets;
- CTest.

Rules:

- project-wide configuration belongs in `CMakePresets.json`;
- machine-specific `CMakeUserPresets.json` is not committed;
- use target-based CMake;
- no project-wide `include_directories()` / `add_definitions()` for project code;
- compile options are target-scoped;
- third-party warnings are isolated from project warning policy.

Initial allowed C++ dependencies:

- GoogleTest **1.17.0**, tests only;
- nlohmann/json **3.12.0**, only at serialization/protocol/persistence/adapter boundaries.

All dependencies must use exact versions/commits plus integrity information in a single dependency lock/bootstrap definition. No floating `main`, `master`, or `latest`. Dependency upgrades are separate bounded tasks.

Domain APIs must not expose `nlohmann::json`; parse/validate JSON at a boundary and convert to typed domain values.

## 4. Determinism

For the same seed, initial state, content version, protocol version, command sequence and simulation step sequence, authoritative results must be reproducible within the declared deterministic contract.

Authoritative logic must not depend on:

- `std::random_device` or hidden system RNG;
- wall clock / `system_clock::now()`;
- frame rate;
- thread scheduling;
- nondeterministic iteration order when order changes results;
- pointer/address ordering;
- locale-dependent parsing;
- unspecified filesystem enumeration order.

Use one explicit seeded PRNG state, or explicitly named deterministic streams derived from the world seed by a fixed algorithm. Do not create convenience RNGs inside systems/NPCs.

If authoritative order matters, use stable ordering or explicitly sort keys.

Use integer/fixed/scaled integer values by default for load-bearing accumulative state such as money, quantities, time, thresholds, obligations, rates and spell costs. A floating-point value that controls an authoritative branch requires an explicit design decision and native/WASM determinism coverage.

Simulation starts single-threaded. Do not add worker/job systems without a measured bottleneck.

## 5. Protocol and WASM boundary

Protocol is a small application contract, not a serialized copy of internal `WorldState`:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

Clients send intent, never desired authoritative state. The simulation computes the outcome.

Protocol has an explicit version. A breaking protocol change updates the version and the affected native/WASM/client verification together. Do not pre-build compatibility for versions that do not exist.

Browser integration uses the same C++ Simulation Core compiled through **Emscripten SDK**.

Early WASM boundary default:

```text
small C-compatible exported facade
+ UTF-8 JSON command/result envelopes
```

Do not expose C++ ABI/object graphs or mutable `WorldState` pointers to JavaScript. TypeScript must not directly mutate C++ memory as the gameplay API. The WASM module must be modularized and avoid global namespace pollution.

Emscripten must be installed from the official SDK, pinned to a concrete tagged release in `tools/toolchain.lock`, checked with `emcc --check`, and never silently fall back to another version. Do not use `latest` in normal scripts.

## 6. Reference web client

Initial client stack:

- TypeScript;
- HTML;
- Canvas 2D;
- DOM for panels/UI;
- ES modules;
- Web Audio only when needed.

Do not initially add React, Vue, Angular, Phaser, Pixi, Three.js, Babylon.js, Electron, a state-management framework, or a DI container. Add a framework only after a concrete measured/observed limitation of the current client.

TypeScript compiler policy:

```json
{
  "compilerOptions": {
    "strict": true,
    "noUncheckedIndexedAccess": true,
    "exactOptionalPropertyTypes": true,
    "noImplicitOverride": true,
    "useUnknownInCatchVariables": true,
    "noFallthroughCasesInSwitch": true,
    "noImplicitReturns": true
  }
}
```

Pin TypeScript in `package.json` + lockfile before use and invoke the repository-local version, not an arbitrary global `tsc`.

## 7. Python developer tooling

Python is for developer tooling and playtest orchestration only, never Simulation Core rules.

When Python tooling is bootstrapped:

- create a repo-local `.venv`;
- pin Playwright and Ruff in a lock/requirements file before routine use;
- use type hints where they improve tool clarity;
- prefer `pathlib`;
- call `subprocess` with explicit argv, cwd, environment and bounded lifecycle/timeouts;
- do not use `shell=True` without a specific justified reason;
- use `try/finally` or context managers for cleanup;
- never implement unbounded polling;
- centralize browser process lifecycle rather than duplicating it in scenarios.

## 8. Browser playtest anti-hang contract

Gameplay browser runs have one canonical entry point once implemented:

```bash
python tools/play.py --scenario <name>
```

Agents must not bypass it with direct Chromium/Chrome/Playwright launch commands for game playtests.

Standard playtest constraints:

- non-blocking singleton lock at `.cache/play/chromium.lock`;
- if occupied: print `PLAYTEST BUSY`, fail, report blocker, stop; do not wait/retry;
- exactly one Playwright runtime, one browser, one ephemeral BrowserContext and one Page;
- no persistent context, browser pools, multiple tabs/contexts or parallel browser workers;
- bounded timeouts; never `timeout=0`;
- baseline upper bounds: 10s launch, 10s navigation, 5s action/state wait, 45s total scripted run;
- wait on explicit conditions rather than arbitrary long sleeps;
- supervisor owns and may terminate only its own worker process group;
- never `pkill chromium`, `killall chromium`, or clean unrelated browser state;
- close context before browser, then Playwright runtime, local server and worker;
- use Playwright-managed Chromium pinned to the Playwright version for routine playtests.

## 9. Testing and local verification

Test pyramid:

```text
many fast deterministic C++ tests
some protocol/native scenario tests
few browser playtest scenarios
manual/agent exploratory playtest
```

GoogleTests must be independent, repeatable, deterministic, behavior-named, small and order-independent. Prefer real value objects/state over large mock graphs.

Use CTest labels as relevant:

```text
unit
sim
protocol
determinism
scenario
slow
```

Browser playtests do not need to be CTest tests.

Use ASan + UBSan in a dedicated local preset for memory/UB-sensitive changes, milestone checks, or suspicious crashes/corruption—not for every UI tweak.

For load-bearing deterministic scenarios, compare native and WASM results when relevant. Divergence is a blocker for that capability.

Verification is risk-based and minimal: build/test only what proves the current bounded change, plus one browser playtest when gameplay is affected.

## 10. Canonical developer commands

Once implemented, the repository must converge on a single thin front door:

```bash
python tools/dev.py configure
python tools/dev.py build
python tools/dev.py test --target sim
python tools/dev.py test --target protocol
python tools/dev.py test --target determinism
python tools/dev.py wasm
python tools/dev.py check
python tools/play.py --scenario smoke
```

These commands are planned contracts until the files exist. Do not claim they work before verifying them. Do not invent alternate routine invocation paths without a concrete reason.

## 11. No CI

This repository intentionally has **no CI** at the current stage.

- Do not add `.github/workflows/*` or another CI service configuration.
- Do not make local development depend on CI.
- Do not report CI status or wait/poll for CI.
- If CI becomes desirable later, it requires an explicit user request and a separate bounded task.

Local deterministic verification and bounded playtests are the source of development evidence for now.

## 12. Development behavior

Gameplay capability work should normally be vertical:

```text
RULE -> CONTRACT -> EXPERIENCE -> PROOF
```

Do not build a simulation subsystem several fidelity levels ahead of player-facing exposure. Do not create fake client-side simulation while waiting for the core.

Before adding a subsystem or abstraction, ask:

1. What can the player see/do because of it?
2. What observable result is currently wrong without it?
3. Can an F1 causal model solve it with far less complexity?
4. Can it be verified in the playable build now?
5. What are we deliberately not modeling?
6. Does this introduce a second source of truth?
7. Does it require speculative abstractions with no current use case?

If the answers are weak, do not add it.
