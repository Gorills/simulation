# Verification and playtest contract

This document owns what counts as evidence for a project claim. It does not own AI Layer Task/Work lifecycle.

## Evidence pyramid

```text
many fast deterministic native C++ tests
some protocol/native scenario tests
few bounded Godot playtest scenarios
manual/exploratory playtest when useful
```

Godot tests do not replace Simulation Core tests. Native simulation tests do not prove that the game is usable.

A claim is verified only by a check that actually ran. Missing tooling, an unbuilt target or an unperformed playtest is **not** a pass.

## Current Milestone 0 gates

The first executable verification paths now exist:

```bash
python tools/check_architecture.py
python tools/dev.py check --preset native
python tools/dev.py check --preset dev
python tools/dev.py play --scenario smoke
```

- `native` configures/builds/tests the Godot-free native graph.
- `dev` additionally builds the GDExtension against the configured immutable godot-cpp pin.
- the smoke playtest is accepted only if Godot exits successfully and the supervisor validates the expected native projection plus screenshot artifact.
- the third-person control foundation additionally requires real local keyboard/mouse and gamepad playtesting; the native smoke probe is not evidence of locomotion feel.

The existence of these commands is not evidence that all of them have run in a particular environment.

## Capability proof

A systemic player-facing capability normally needs:

1. authoritative native behavior;
2. explicit protocol input/output;
3. targeted deterministic/regression evidence where relevant;
4. visible player feedback;
5. one bounded playtest proving the real round-trip.

Engine-local presentation/control work does not need a fake native rule merely to satisfy this list, but it must be exercised in the real Godot client and must not create systemic authority in GDScript.

Compile-green alone is not Definition of Done for gameplay.

## Native tests

Tests should be independent, repeatable and deterministic, with no order dependence.

Use behavior-oriented names. Prefer `EXPECT_*` for multiple independent observations and `ASSERT_*` only when continuing would be meaningless.

Native rule tests must not require Godot. Core tests use real value objects/state where practical rather than mock-heavy designs.

The native test graph mirrors production ownership:

- `sim_core_tests` links only `sim_core` + `GTest::gtest_main` and exercises domain state/transitions directly;
- `protocol_tests` links `sim_protocol` + `GTest::gtest_main` and exercises validation plus the command-to-domain-to-projection round-trip.

The first domain tests prove cardinal bootstrap movement advances authoritative probe position/time deterministically. The first protocol tests prove a valid probe move produces the expected projection and malformed diagonal input is rejected without mutating the world.

CTest also registers `architecture_no_godot_in_core`, which runs `tools/check_architecture.py` against `src/sim` and `src/protocol` and enforces repository-level architecture policy.

Recommended CTest labels:

```text
unit
sim
protocol
determinism
scenario
slow
architecture
```

Godot playtests are not registered in the default CTest set.

## Sanitizers

Provide a separate ASan+UBSan preset when memory/UB-sensitive implementation work justifies it. Run it for crashes/corruption and milestone verification rather than every presentation-only edit.

Do not add sanitizer presets merely as decoration before they can run in a supported local toolchain.

## Debug surface

The development client should expose a read-only debug surface sufficient to explain a playtest:

- ready/build/protocol identifiers;
- player presentation position and semantic location;
- world time;
- nearby interactables and visible actor IDs;
- last authoritative events;
- seed and scenario;
- last command result;
- current screen/dialog state.

The debug surface must not expose authoritative mutation methods. Scenario setup belongs to explicit initialization before the world starts, not debugger cheats.

Milestone 0 starts this surface with the current local third-person presentation position plus `SimFacade.debug_projection()`/native smoke projection. The two are deliberately displayed as different concepts until semantic location has a real gameplay contract.

## Third-person control verification

Control feel is empirical. Source review and Godot import success are necessary but insufficient.

Before accepting a material control change, use the pinned Godot runtime and verify the affected device path. For the foundation, check at least:

- keyboard movement is camera-relative and releases cleanly without drift;
- mouse look direction is correct, captured motion is stable across the supported window/stretch setup, and Escape/click pointer lifecycle works;
- left-stick movement has a circular deadzone, reaches the full analog range and does not drift at rest;
- right-stick look has no drift at rest, reaches useful angular speed at full deflection and has the expected vertical direction;
- Shift and L3 both engage sprint without duplicating locomotion code;
- acceleration/deceleration and turn response feel immediate rather than delayed by stacked smoothing;
- `SpringArm3D` pulls the camera inward against nearby geometry and excludes the player collider;
- walking across floor/slope contacts does not introduce obvious jitter or camera stepping.

Tune values through `ControlProfile` / `LocomotionProfile` first. Change algorithms only when playtest evidence shows that tuning cannot solve the problem.

## Godot playtest supervisor

`tools/play.py` is the single ordinary automated playtest entry point:

```bash
python tools/play.py --scenario smoke
```

Do not create an alternative long-lived runner merely for convenience.

### Preflight

The supervisor fails before launch unless the expected debug GDExtension library exists and the resolved Godot binary reports the exact 4.7.1 baseline. `GODOT_BIN` is the explicit override when Godot is not on `PATH`.

### Locking

The supervisor takes the non-blocking `.cache/play/godot.lock`.

If the lock is held, exit non-zero with a clear `PLAYTEST BUSY` result. Do not wait forever, poll indefinitely or start a competing Godot process.

### Process ownership

One ordinary scenario run owns exactly the Godot process group it spawned for the repository project.

Never use `pkill godot` or `killall godot`. Never terminate an editor or process that the supervisor did not create.

### Deadlines

Every run has a bounded timeout; the current smoke default is 30 seconds. No wait is infinite.

Wait on conditions such as ready state, projection state, an explicit event or screenshot checkpoint. Arbitrary sleeps are only for intentional presentation observation.

### Cleanup

The supervisor owns graceful termination, a hard deadline, forced termination of **its own** process group if necessary, diagnostics and lock release.

Use `try/finally` or an equivalent lifecycle guard.

### Artifacts

Each run writes under:

```text
.cache/play/<run-id>/
  run.json
  stdout.log
  stderr.log
  final.png
  debug.json
```

For the `smoke` scenario, success additionally requires `debug.json` to contain the native projection:

```text
x=1, y=0, tick=1, seed=1, protocol_version=1
```

and `final.png` to exist. The Godot scene obtains that state by calling `SimFacade.submit_move(1, 0)`, writes the returned projection to the debug artifact/surface, and captures the real 3D client. It does not move the `CharacterBody3D` from this bootstrap grid-step result.

Successful artifacts may be retained on a bounded rolling basis; failure artifacts should remain available for diagnosis until explicit cleanup.

## Risk-based verification

### C++ world-rule change

```text
build affected target
-> focused native test
-> determinism/scenario test when relevant
-> affected Godot playtest when player-visible
```

### Godot presentation/control change

```text
Godot import/load
-> affected keyboard/mouse or gamepad playtest
-> collision/camera/input checks relevant to the change
-> screenshot/visible-state evidence when useful
```

### Protocol / GDExtension change

```text
native protocol tests
-> adapter build
-> Godot loads extension
-> one command/projection round-trip playtest
```

### Persistence change

```text
save/load round-trip
-> replay/determinism evidence
-> player-facing save/load scenario
```

### Dependency/version change

Follow [`engineering/VERSIONS.md`](engineering/VERSIONS.md): build native code, build the adapter, load it in the pinned Godot engine and run the smallest playable scenario.

## Local-only verification policy

This project intentionally has **no CI**. Verification and milestone acceptance are performed on the developer machine with the repository-owned bootstrap, CMake/CTest presets, bounded Godot playtests and direct local control playtesting.

Do not add GitHub Actions, GitLab CI, CircleCI, Jenkins, Buildkite, Azure Pipelines, Travis, AppVeyor, or another project CI service/configuration. Do not create committed workflow files as a substitute for running the local gates.

A local failure must be fixed locally; it cannot be deferred with “CI will catch it.” Empty GitHub status/check contexts are expected and are not missing project evidence. The applicable local test/build/playtest artifacts are the evidence.

## Definition of Done for a gameplay capability

A capability is done only when all applicable items are true:

- authoritative C++ implementation exists when the capability has systemic world consequences;
- Godot has no bypass truth for systemic outcomes;
- protocol expresses the systemic input/output when one is needed;
- deterministic behavior is tested where required;
- a targeted regression test protects important causality;
- player-facing feedback exists;
- the capability is actually reachable in the game;
- the bounded/manual local playtest demonstrates the expected outcome;
- relevant docs/model changed if and only if the real contract changed;
- unrelated refactoring is absent.

## Evidence reporting

Reports should distinguish facts, not perform a second workflow ceremony:

- **VERIFIED** — checks that actually ran and their results;
- **NOT VERIFIED** — checks not run or unavailable;
- **BLOCKERS** — concrete conditions preventing completion.

For gameplay evidence record the scenario, relevant input/action and observable screenshot/debug outcome.

Never say “fully verified” when only a subset of the required evidence exists.
