# Verification and playtest contract

This document owns **proof obligations, verification entry points and what a passing check is allowed to claim**. It does not own product goals, mechanic specifications, current roadmap sequencing or dependency versions.

Exact implemented assertions belong in executable tests and validators. This document describes the evidence shape and routes to those executables rather than copying every fixture value into prose.

## Evidence pyramid

```text
many fast deterministic native tests
some protocol/native scenario tests
few bounded Godot runtime smoke scenarios
manual/exploratory playtest when useful
```

A claim is verified only by a check that actually ran on the exact revision being claimed. Missing tooling, an unbuilt target, an unperformed playtest or evidence from an earlier revision is **not** a pass.

Compile-green alone is not Definition of Done for player-facing gameplay.

## Ordinary local gates

```bash
python tools/check_architecture.py
python tools/check_localization.py
python tools/dev.py check --preset native
python tools/dev.py check --preset sanitize
python tools/dev.py check --preset dev
python tools/dev.py play --scenario smoke --locale ru
python tools/dev.py play --scenario smoke --locale en
```

Use only the subset required by the change, but do not claim a gate that did not run.

- `native` builds/tests the Godot-free native graph.
- `sanitize` runs the Godot-free graph with the configured sanitizer preset.
- `dev` additionally builds the GDExtension development graph.
- `play --scenario smoke` launches the real Godot project through the repository supervisor and validates the current boundary evidence plus screenshot artifact.

The exact Godot/tool versions are owned by lock/configuration files and [`engineering/VERSIONS.md`](engineering/VERSIONS.md), not duplicated here.

## Capability proof

A systemic player-facing capability normally needs:

1. authoritative native behavior;
2. semantic protocol input/result/projection or transition-result contract;
3. focused deterministic/regression evidence where relevant;
4. visible Godot feedback based on the authoritative result;
5. one bounded playtest proving the real round-trip;
6. for actor capabilities, evidence that human-controlled and NPC actors do not use contradictory world laws.

Engine-local presentation/control work does not need a fake native mechanic merely to satisfy this list, but it must be exercised in the real Godot client and must not create world authority in GDScript.

## Native and protocol tests

Native tests must remain deterministic, independent and Godot-free where the production owner is `src/sim`/`src/protocol`.

The target graph should make ownership visible:

- Simulation-domain tests link against Simulation without Godot;
- protocol tests exercise boundary validation/orchestration and authoritative results;
- architecture checks mechanically reject Godot leakage into Core/protocol;
- mechanic-specific detailed assertions live next to the mechanic tests rather than in this document.

Use behavior-oriented test names. Prefer direct domain values/state over mock-heavy tests when practical.

## Sanitizers

Run the sanitizer preset for memory/UB-sensitive native changes, crashes/corruption investigations and milestone verification. Sanitizer success is correctness evidence, not performance evidence and not proof that Godot runtime integration works.

## Godot smoke supervisor

`tools/play.py` is the single ordinary automated Godot playtest entry point. Do not create a second long-lived smoke runner for each feature.

```bash
python tools/play.py --scenario smoke --locale ru
python tools/play.py --scenario smoke --locale en
```

The supervisor owns:

- pinned Godot resolution/version validation;
- headless project metadata import before the visual run;
- a non-blocking repository playtest lock;
- one owned Godot process group;
- bounded timeout and cleanup;
- required debug/screenshot artifacts;
- semantic validation of the authoritative Simulation ↔ protocol ↔ GDExtension ↔ Godot round-trip.

The exact fixture values and debug JSON assertions are executable truth in `tools/play.py` and the relevant native/protocol tests. Do not copy those values into architecture/roadmap prose.

Each run writes under:

```text
.cache/play/<run-id>/
  run.json
  stdout.log
  stderr.log
  final.png
  debug.json
```

A smoke pass requires a zero Godot exit status and successful supervisor validation of the currently implemented boundary evidence. At minimum that evidence covers authoritative identity/presence, controlled exact-spatial state, an authoritative movement transition for the current scenario, presentation binding/reconciliation, duplicate/stale transition rejection where applicable, active locale probes and a rendered screenshot.

The smoke is boundary/runtime evidence. It does **not** prove subjective control feel, audio output, GPU-driver coverage or performance.

## Godot process/environment policy

Metadata import is non-visual CI work and uses Godot headless mode. The repository must not require X11, VSync, a physical GPU or an audio device merely to import project metadata.

The visual screenshot smoke intentionally runs with a virtual display in Linux CI. The smoke runner explicitly selects the project's `gl_compatibility` rendering method, uses the `Dummy` audio driver and disables VSync. This keeps missing ALSA/Pulse devices, Vulkan surface support and virtual-display VSync limitations from masquerading as gameplay failures.

CI software rendering is compatibility evidence only. A successful llvmpipe/Mesa run does not prove real-GPU performance or driver-specific rendering correctness.

Unexpected engine `ERROR` lines should still be investigated; the pass/fail contract is the process result plus validated artifacts, not a rule that arbitrary stderr output is harmless.

## Godot smoke CI lane

The repository carries a persistent `.github/workflows/godot-smoke.yml` lane for runtime integration changes. It:

1. installs the Godot version from `tools/toolchain.lock.json` rather than duplicating a version literal in workflow prose;
2. builds the development/GDExtension graph;
3. runs the Russian smoke under Xvfb;
4. runs the English smoke under Xvfb;
5. uploads `.cache/play` evidence even when the job fails.

Documentation-only changes are excluded from this lane. The lane is deliberately bounded: it does not add narrow-layout matrices, interactive input feel, performance thresholds or cross-platform GPU matrices without a demonstrated need.

A green Godot smoke check applies only to the exact commit it tested.

## Native CI lane

`.github/workflows/native.yml` runs clean `native` and `sanitize` configure/build/test passes on pull requests and pushes to `main`.

Native CI is independent evidence, not a substitute for local verification. A local failure must still be fixed locally rather than deferred with “CI will catch it.”

## Manual third-person verification

Control feel is empirical. Material input/camera/movement-presentation changes require a real local playtest of the affected device path in addition to automated smoke.

Check the relevant subset of:

- keyboard movement is camera-relative and releases without drift;
- pointer capture/release and mouse look behave correctly;
- gamepad sticks have usable deadzones/range and no rest drift;
- ordinary movement/sprint select semantic intent rather than a Godot-local speed law;
- presentation follows authoritative movement without a competing local position path;
- turning/camera remain responsive without changing authoritative world truth;
- camera collision behaves correctly around presentation geometry;
- same-continuity movement does not introduce obvious rendering jitter.

Automated smoke must not be reported as proof of subjective movement feel.

## Localization verification

Catalog integrity is necessary but not sufficient for player-visible text changes.

A material localization/UI change should verify:

```text
catalog parity
-> Godot import/load
-> render affected UI in ru
-> render affected UI in en
-> inspect glyph coverage, clipping and reflow
```

Use pseudolocalization or additional viewport evidence when the layout risk actually requires it; do not make every ordinary runtime change pay for a speculative matrix.

## Risk-based verification

### C++ world-rule change

```text
build affected target
-> focused native test
-> determinism/scenario test when relevant
-> affected Godot smoke/playtest when player-visible
```

### Simulation ↔ Godot boundary change

```text
native/protocol tests
-> development adapter build
-> headless Godot import
-> bounded Godot smoke
-> inspect evidence/artifacts when the change affects presentation
```

### Godot presentation/control change

```text
Godot import/load
-> bounded smoke
-> affected keyboard/mouse/gamepad playtest
-> screenshot/visible-state inspection when useful
-> confirm no authoritative state moved into Godot
```

### Localization / player-visible text change

```text
catalog integrity
-> Godot import/load
-> affected locale renders
-> layout/glyph inspection
```

### Protocol / GDExtension change

```text
native protocol tests
-> development adapter build
-> Godot import
-> one semantic command/result/projection-or-transition round-trip smoke
```

### Persistence change

```text
save/load round-trip
-> replay/determinism evidence
-> player-facing save/load scenario
```

### Dependency/version change

Follow [`engineering/VERSIONS.md`](engineering/VERSIONS.md): verify native code, adapter build, pinned Godot load and the smallest affected playable scenario.

## Debug surface

Development diagnostics should be read-only and sufficient to explain a playtest: build/protocol identifiers, controlled identity, authoritative vs presentation state, time/revision/continuity identifiers, relevant observed/materialized identities, last authoritative result/events/samples and scenario/locale state.

The debug surface must not expose authoritative mutation methods. Scenario setup belongs to explicit initialization, not debugger cheats.

## Definition of Done

For the bounded task being completed:

- relevant source/tests/contracts were inspected before the change;
- the smallest coherent implementation was made;
- focused executable checks passed on the exact claimed revision, or missing evidence is reported explicitly;
- player-visible/boundary work received the appropriate Godot evidence;
- no unrelated refactor or speculative framework was mixed into the change;
- documentation was updated only at the canonical owner of the changed fact.
