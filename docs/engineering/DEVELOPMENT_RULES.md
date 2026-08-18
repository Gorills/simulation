# Development Rules

**Status:** ACTIVE

These are the canonical engineering rules for the current project stack. The stack is intentionally chosen so the full build/test/playtest loop runs inside the agent environment without downloading tools or dependencies.

## 1. Authority and dependency direction

There is one authoritative implementation of world rules: the **C++23 Simulation Core**.

```text
presentation -> protocol -> simulation
simulation !-> presentation
```

The Simulation Core must not depend on terminal input, rendering, wall-clock time, filesystem/network as gameplay inputs, UI state, locale, or environment variables as sources of world rules.

Presentation may map input and render projections. It must not own authoritative inventory, money, relationships, status, skills, location, ownership, institutions, spell outcomes, trade outcomes, or any alternate gameplay state.

## 2. Runnable-stack invariant

The required development loop must work with the tools already available in the execution environment and must not require network access.

Current required stack:

- C++23;
- GCC capable of C++23 (Clang is an optional local cross-check);
- CMake;
- Ninja;
- CTest;
- Python 3 standard library;
- native terminal reference client.

The ordinary loop must not download SDKs, package-manager dependencies, browser binaries, test frameworks, or build tools.

A new third-party dependency, browser runtime, WASM toolchain, package manager requirement, or network bootstrap is a separate bounded stack-change task and must prove it runs in the agent environment before becoming required.

Browser/WASM may return later as optional presentation adapters. They must not become a development gate until the environment proves they can build and run locally.

## 3. C++ policy

- Language standard: C++23.
- Required CMake settings:

```text
CMAKE_CXX_STANDARD = 23
CMAKE_CXX_STANDARD_REQUIRED = ON
CMAKE_CXX_EXTENSIONS = OFF
```

- Prefer value semantics and RAII.
- Use heap ownership only when real lifetime requirements demand it.
- No mutable global world state, service locator, mutable singleton, hidden RNG, or mutable global registry.
- Expected gameplay/domain failures are data, not control-flow exceptions.
- Prefer concrete types; do not create interfaces/framework layers for hypothetical use.
- Avoid metaprogramming and abstractions without current payoff.

Project code uses target-scoped warnings:

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

Do not mass-format unrelated files in a bounded task.

## 4. Build and dependency policy

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
- the current required stack has **no third-party C++ dependencies**.

Do not add GoogleTest, nlohmann/json, Emscripten, frontend frameworks, package-manager dependencies, or equivalent libraries merely for convenience. A real need requires a separate bounded decision and local viability proof.

## 5. Determinism

For the same seed, initial state, content version, protocol version, command sequence and simulation step sequence, authoritative results must be reproducible within the declared deterministic contract.

Authoritative logic must not depend on:

- `std::random_device` or hidden system RNG;
- wall clock / `system_clock::now()`;
- rendering/frame rate;
- thread scheduling;
- nondeterministic iteration order when order changes results;
- pointer/address ordering;
- locale-dependent parsing;
- unspecified filesystem enumeration order.

Use explicit seeded PRNG state when randomness is introduced. Do not create convenience RNGs inside systems/NPCs.

Use integer/fixed/scaled integer values by default for load-bearing accumulative state such as money, quantities, time, thresholds, obligations and rates.

Simulation starts single-threaded. Do not add worker/job systems without a measured bottleneck.

## 6. Protocol contract

Protocol is a small application contract, not a copy of internal `WorldState`:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

Clients send intent, never desired authoritative state. The simulation computes the outcome.

Protocol has an explicit version. A breaking protocol change updates the version and affected tests/client verification together. Do not pre-build compatibility for versions that do not exist.

The current native terminal client links to the C++ core through typed protocol structures. A future browser/network adapter must map external data into the same typed protocol rather than bypassing it.

## 7. Reference client

The current required reference client is the native executable `sim_cli`.

Responsibilities:

- read player input;
- map keys to protocol intents;
- render projections and feedback;
- expose machine-readable debug lines for bounded playtests.

It must not directly mutate simulation state.

The first controls are `W/A/S/D`; the current line-oriented terminal client requires Enter after a command in an interactive terminal. `Q` quits.

A graphical/browser client is optional future presentation work, not a prerequisite for simulation progress.

## 8. Python tooling

Python is developer tooling/playtest orchestration only, not Simulation Core.

Current required tooling uses the Python standard library only.

Rules:

- use type hints where they improve clarity;
- use `pathlib` for paths;
- `subprocess` uses explicit argv, cwd and bounded timeouts;
- no `shell=True` without a specific justified reason;
- cleanup through `try/finally` or bounded process lifecycle code;
- no infinite polling;
- tooling must not perform unexpected network access.

## 9. Native playtest anti-hang contract

The only standard automated gameplay entry point is:

```bash
python tools/play.py --scenario <name>
```

Do not bypass it with ad-hoc gameplay subprocess scripts when a canonical scenario exists.

Standard playtest rules:

- non-blocking lock at `.cache/play/terminal.lock`;
- if busy: print `PLAYTEST BUSY`, return non-zero, and stop;
- exactly one game subprocess for the scenario;
- child starts in its own process group/session;
- hard wall-clock timeout: 10 seconds unless a scenario explicitly justifies less/more;
- on timeout terminate/kill only the owned process group;
- no arbitrary long sleeps or retry loops;
- success retains bounded recent artifacts.

Artifacts:

```text
.cache/play/<run-id>/
  run.json
  stdout.log
  stderr.log
  final.txt
  debug.json
```

`final.txt` is the rendered terminal frame. `debug.json` is machine-readable authoritative evidence exposed by the client projection.

## 10. Test architecture

Use many small deterministic native tests and few bounded gameplay scenarios.

Current tests are dependency-free C++ executables registered in CTest. They must be:

- independent;
- repeatable;
- deterministic;
- behavior-focused;
- small;
- order-independent.

CTest labels:

```text
unit
sim
protocol
determinism
scenario
slow
```

Mocks should be rare. Core domain tests should use real values/state.

## 11. Canonical local commands

```bash
python tools/dev.py doctor
python tools/dev.py configure
python tools/dev.py build
python tools/dev.py test --target sim
python tools/dev.py test --target protocol
python tools/dev.py test --target determinism
python tools/dev.py check
python tools/play.py --scenario smoke
```

`tools/dev.py` is thin orchestration over CMake/CTest. It is not a custom build system.

Do not invent alternate invocation paths without a concrete reason.

## 12. Risk-based verification

Run the smallest sufficient check for the task.

### C++ rule change

```text
build affected target
-> focused native test / CTest label
-> affected playtest if player-visible
```

### Terminal presentation/input change

```text
build sim_cli
-> affected playtest
-> inspect final.txt/debug.json when load-bearing
```

### Persistence change

```text
save/load roundtrip
-> replay determinism
-> one player-facing save/load scenario
```

## 13. No CI

This repository intentionally has no CI. Do not add GitHub Actions, CI configuration, status-gate logic, or CI documentation unless the user explicitly requests CI in a later bounded task.

Development evidence is local build/test/playtest output.

## 14. Bounded agent workflow

Each development pass:

```text
Task N
-> inspect relevant code/tests/docs
-> define IN SCOPE / OUT OF SCOPE internally
-> minimal coherent change
-> self-review diff
-> targeted local tests
-> if gameplay affected: one bounded canonical playtest
-> commit/push only when permitted
-> report VERIFIED / NOT VERIFIED / ASSUMPTIONS / BLOCKERS
-> STOP
```

After `продолжай`, audit the previous task before starting the next. If the previous task has a blocker, fix only that task and stop again.

If the same issue survives two meaningful attempts, stop with observed facts and diagnostics instead of random retries.

## 15. No speculative architecture

Before adding an abstraction ask:

1. what current problem does it solve;
2. is there a second real use case;
3. can it be simpler;
4. does it increase the number of states/layers an agent must remember.

Do not add abstractions, subsystems or frameworks whose current payoff is only hypothetical.
