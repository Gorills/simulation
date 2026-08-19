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

A claim is verified only by a check that actually ran on the exact revision being claimed. Missing tooling, an unbuilt target, an unperformed playtest, or a check from an earlier revision is **not** a pass.

## Current Milestone 0 gates

The executable verification paths now include:

```bash
python tools/check_architecture.py
python tools/dev.py check --preset native
python tools/dev.py check --preset sanitize
python tools/dev.py check --preset dev
python tools/dev.py play --scenario smoke
```

- `native` configures/builds/tests the Godot-free native graph.
- `sanitize` runs the same Godot-free graph with ASan+UBSan under GCC/Clang.
- `dev` additionally builds the GDExtension against the configured immutable godot-cpp pin.
- the smoke playtest is accepted only if Godot exits successfully and the supervisor validates bootstrap transport, observed identity, authoritative controlled-actor spatial state, presentation initialization and screenshot evidence.
- the third-person control foundation additionally requires real local keyboard/mouse and gamepad playtesting; static authoritative spawn evidence is not proof that continuous locomotion is authoritative yet.

The existence of these commands is not evidence that all of them have run in a particular environment.

## Capability proof

A systemic player-facing capability normally needs:

1. authoritative native behavior;
2. explicit semantic protocol command/result/events/projection;
3. targeted deterministic/regression evidence where relevant;
4. visible Godot feedback based on the authoritative result;
5. one bounded playtest proving the real round-trip;
6. for an actor capability, evidence that human-controlled and NPC actors do not use contradictory world rules.

Engine-local presentation/control work does not need a fake native rule merely to satisfy this list, but it must be exercised in the real Godot client and must not create world authority in GDScript.

Compile-green alone is not Definition of Done for gameplay.

## Native tests

Tests should be independent, repeatable and deterministic, with no order dependence.

Use behavior-oriented names. Prefer `EXPECT_*` for multiple independent observations and `ASSERT_*` only when continuing would be meaningless.

Native rule tests must not require Godot. Core tests use real value objects/state where practical rather than mock-heavy designs.

The native test graph mirrors production ownership:

- `sim_core_tests` links only `sim_core` + `GTest::gtest_main` and exercises domain state/transitions directly;
- `protocol_tests` links `sim_protocol` + `GTest::gtest_main` and exercises validation plus command-to-domain-to-projection behavior.

The current domain tests prove:

- different actor `EntityId`s use the same authoritative bootstrap actor operation;
- actor actions change `WorldRevision` without pretending that `SimulationTick` advanced;
- explicit simulation-time advancement changes tick/revision separately;
- duplicate/unknown/invalid identities fail without mutation;
- exact `SpatialState` can exist for one actor while another authoritative actor has no exact spatial pose;
- invalid `SpatialEpoch` is rejected without mutating the world;
- equal initial state/action sequences remain deterministic.

The current protocol tests prove:

- the minimum `ObservedWorldProjection` contains only the externally controlled actor identity and current tick/revision;
- the controlled actor starts with exact authoritative spatial state at the origin, zero velocity and epoch 1;
- bootstrap grid movement advances world revision but does **not** alter production `SpatialState`;
- malformed bootstrap input is rejected without mutating observed authoritative state.

CTest also registers `architecture_no_godot_in_core`, which runs `tools/check_architecture.py` against `src/sim` and `src/protocol` and enforces the Godot-free dependency boundary.

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

## Authority-boundary regression evidence

As real mechanics arrive, tests must protect the Simulation/Godot ownership boundary rather than only individual formulas.

Representative proofs:

- merchant stock/money transfer happens atomically in Simulation, not UI state;
- the same transaction law works for an NPC buyer and the controlled actor;
- a relationship change is visible through a projection but cannot be set by Godot;
- an offscreen event changes Simulation state without requiring a Godot node;
- materialization/dematerialization does not create/delete authoritative entities;
- stale presentation revisions cannot overwrite newer authoritative samples;
- local prediction, when introduced, reconciles to authoritative movement and cannot create systemic outcomes.

Do not write these tests before the corresponding feature exists. The list defines what evidence the eventual architecture requires.

## Sanitizers

The repository provides the `sanitize` CMake preset for Godot-free native ASan+UBSan verification under GCC/Clang.

Run it for memory/UB-sensitive native changes, crashes/corruption investigations and milestone verification. The minimal native CI gate also runs this preset so a clean independent checkout exercises the sanitizer build/tests for every pull request and push to `main`.

Sanitizer success is correctness evidence, not performance evidence. It does not replace the normal native preset, GDExtension/Godot verification or player-facing playtests.

## Debug surface

The development client should expose a read-only debug surface sufficient to explain a playtest:

- ready/build/protocol identifiers;
- controlled actor `EntityId`;
- presentation position separately from authoritative spatial/semantic location;
- `SimulationTick`, `WorldRevision` and spatial continuity epoch separately;
- nearby/materialized actor IDs and relevant observation state;
- last authoritative events;
- seed and scenario;
- last command result;
- current screen/dialog state.

The debug surface must not expose authoritative mutation methods. Scenario setup belongs to explicit initialization before the world starts, not debugger cheats.

Milestone 0 now exposes the local third-person presentation position, `SimFacade.bootstrap_debug_projection()`, `SimFacade.observed_world_projection()`, `SimFacade.controlled_actor_spatial_projection()`, and the read-only `WorldPresentation` identity/spatial initialization snapshot. The bootstrap grid and production spatial sample remain deliberately separate.

## Third-person control verification

Control feel is empirical. Source review and Godot import success are necessary but insufficient.

Before accepting a material control/presentation change, use the pinned Godot runtime and verify the affected device path. For the foundation, check at least:

- keyboard movement intent is camera-relative and releases cleanly without drift;
- mouse look direction is correct, captured motion is stable across the supported window/stretch setup, and Escape/click pointer lifecycle works;
- left-stick movement has a circular deadzone, reaches the full analog range and does not drift at rest;
- right-stick look has no drift at rest, reaches useful angular speed at full deflection and has the expected vertical direction;
- Shift and L3 both engage sprint intent without duplicating input code;
- presentation acceleration/deceleration/direction-change/turn response feels immediate rather than delayed by stacked smoothing;
- `SpringArm3D` pulls the camera inward against nearby geometry and excludes the player presentation collider;
- walking across floor/slope contacts does not introduce obvious visual jitter or camera stepping.

These checks validate presentation feel. They do **not** prove authoritative continuous world movement. That proof begins only when semantic movement intent drives a Godot-free Simulation movement/collision transition and Godot renders resulting samples.

Tune presentation values through `ControlProfile` / `LocomotionProfile` first. Change presentation algorithms only when playtest evidence shows that tuning cannot solve the problem.

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

For the `smoke` scenario, `debug.json` is a boundary-evidence object with four sections:

```text
bootstrap_projection:
  entity_id=1, x=1, y=0, tick=0, revision=2, seed=1, protocol_version=4

observed_world_projection:
  controlled_actor_id=1, tick=0, revision=2, protocol_version=4
  entities=[{entity_id=1}]

controlled_actor_spatial_projection:
  entity_id=1
  position_m=[0,0,0]
  velocity_mps=[0,0,0]
  spatial_epoch=1
  tick=0, revision=2, protocol_version=4

presentation:
  controlled_entity_id=1
  last_tick=0
  last_revision=2
  protocol_version=4
  observed_entity_ids=[1]
  bound_entity_ids=[1]
  controlled_spatial_initialized=true
  controlled_spatial_epoch=1
  controlled_spatial_tick=0
  controlled_spatial_revision=1
```

The initial actor spawn creates revision 1. `WorldPresentation` binds identity and performs initial placement from the revision-1 spatial projection, then calls `reset_physics_interpolation()`. The bootstrap step creates revision 2 but mutates only the old grid probe; a fresh production spatial projection therefore still reports origin/zero velocity/epoch 1 at revision 2.

This difference is intentional evidence that bootstrap grid coordinates cannot become authoritative third-person position by accident.

Successful artifacts may be retained on a bounded rolling basis; failure artifacts should remain available for diagnosis until explicit cleanup.

## Risk-based verification

### C++ world-rule change

```text
build affected target
-> focused native test
-> determinism/scenario test when relevant
-> affected Godot playtest when player-visible
```

### Simulation ↔ Godot boundary change

```text
native rule/protocol tests
-> identity/tick/revision/epoch/projection assertions
-> adapter build
-> Godot load
-> command/result/projection round-trip
-> presentation reconciliation/materialization evidence when applicable
```

### Godot presentation/control change

```text
Godot import/load
-> affected keyboard/mouse or gamepad playtest
-> collision/camera/input checks relevant to the change
-> screenshot/visible-state evidence when useful
-> confirm no new authoritative state was introduced in Godot
```

### Protocol / GDExtension change

```text
native protocol tests
-> adapter build
-> Godot loads extension
-> one semantic command/projection round-trip playtest
```

### Persistence change

```text
save/load round-trip
-> replay/determinism evidence
-> player-facing save/load scenario
```

### Dependency/version change

Follow [`engineering/VERSIONS.md`](engineering/VERSIONS.md): build native code, build the adapter, load it in the pinned Godot engine and run the smallest playable scenario.

## Independent native CI policy

Local verification remains the primary developer feedback loop. The repository also carries one intentionally small GitHub Actions workflow that runs clean `native` and `sanitize` configure/build/test passes on pull requests and pushes to `main`.

CI is independent evidence, not a substitute for developer-machine verification. It deliberately does **not** claim Godot runtime/load, bounded smoke artifacts, manual keyboard/gamepad feel, screenshots or performance acceptance.

A green check applies only to the exact commit it tested. If code changes after a failure or after the last green run, the affected gate must run again before that new revision may be reported as verified. A local failure must still be fixed locally rather than deferred with “CI will catch it.”

Keep this workflow narrow until a real cross-platform or GDExtension regression demonstrates the need for another automated lane. Do not move empirical performance thresholds or interactive Godot acceptance into CI merely because CI now exists.

## Definition of Done for a gameplay capability

A capability is done only when all applicable items are true:

- authoritative C++ implementation exists for world state/consequences;
- player-controlled and NPC actors use the same relevant world rule rather than privileged player logic;
- Godot has no bypass truth for entity existence/location/inventory/economy/social/politics/combat/magic outcomes;
- protocol expresses the semantic input/result/events/projection when needed;
- projection exposes only the presentation information that should be observable;
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
