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

## Capability proof

A player-facing capability normally needs:

1. authoritative native behavior;
2. explicit protocol input/output;
3. targeted deterministic/regression evidence where relevant;
4. visible player feedback;
5. one bounded playtest proving the real round-trip.

Compile-green alone is not Definition of Done for gameplay.

## Native tests

Tests should be independent, repeatable and deterministic, with no order dependence.

Use behavior-oriented names. Prefer `EXPECT_*` for multiple independent observations and `ASSERT_*` only when continuing would be meaningless.

Native rule tests must not require Godot. Core tests use real value objects/state where practical rather than mock-heavy designs.

Recommended CTest labels once targets exist:

```text
unit
sim
protocol
determinism
scenario
slow
```

Godot playtests do not have to be registered in the default `ctest` set.

## Sanitizers

Provide a separate ASan+UBSan preset once the build exists. Run it for memory/UB-sensitive changes, crashes/corruption and milestone verification rather than every presentation-only edit.

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

## Godot playtest supervisor

When `tools/play.py` exists, it is the single ordinary playtest entry point:

```bash
python tools/play.py --scenario <name>
```

Until it exists, do not create an alternative long-lived runner just to satisfy documentation.

### Locking

The supervisor takes a non-blocking lock such as `.cache/play/godot.lock`.

If the lock is held, exit non-zero with a clear `PLAYTEST BUSY` result. Do not wait forever, poll indefinitely or start a competing Godot process.

### Process ownership

One ordinary scenario run owns exactly the Godot process group it spawned for the repository project.

Never use `pkill godot` or `killall godot`. Never terminate an editor or process that the supervisor did not create.

### Deadlines

Every start/action/state-wait/run has a bounded timeout. Exact values belong in executable scenario/tool configuration once implemented; no wait is infinite.

Wait on conditions such as ready state, projection state, an explicit event or screenshot checkpoint. Arbitrary sleeps are only for intentional presentation observation.

### Cleanup

The supervisor owns graceful termination, a hard outer deadline, forced termination of **its own** process group if necessary, diagnostics and lock release.

Use `try/finally` or an equivalent lifecycle guard.

### Artifacts

A useful run artifact layout is:

```text
.cache/play/<run-id>/
  run.json
  stdout.log
  stderr.log
  final.png
  debug.json
```

Successful artifacts may be retained on a bounded rolling basis; failure artifacts should remain available for diagnosis until an explicit cleanup.

## Risk-based verification

### C++ world-rule change

```text
build affected target
-> focused native test
-> determinism/scenario test when relevant
-> affected Godot playtest when player-visible
```

### Godot presentation-only change

```text
Godot import/load
-> affected bounded playtest
-> screenshot/visible-state evidence
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

## CI

CI is an independent backstop, not the first feedback loop or a workflow state machine.

The first useful CI should remain bounded:

```text
checkout
-> acquire pinned toolchain/dependencies
-> configure
-> native build
-> fast unit/protocol/determinism tests
```

Do not add a large matrix or full graphical playtest farm before there is measured value.

## Definition of Done for a gameplay capability

A capability is done only when all applicable items are true:

- authoritative C++ implementation exists;
- Godot has no bypass truth;
- protocol expresses the input/output;
- deterministic behavior is tested where required;
- a targeted regression test protects important causality;
- player-facing feedback exists;
- the capability is actually reachable in the game;
- the bounded playtest demonstrates the expected outcome;
- relevant docs/model changed if and only if the real contract changed;
- unrelated refactoring is absent.

## Evidence reporting

Reports should distinguish facts, not perform a second workflow ceremony:

- **VERIFIED** — checks that actually ran and their results;
- **NOT VERIFIED** — checks not run or unavailable;
- **BLOCKERS** — concrete conditions preventing completion.

For gameplay evidence record the scenario, relevant input/action and observable screenshot/debug outcome.

Never say “fully verified” when only a subset of the required evidence exists.
