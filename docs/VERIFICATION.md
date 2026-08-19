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
python tools/dev.py play --scenario offscreen --locale ru
```

Use only the subset required by the change, but do not claim a gate that did not run.

- `native` builds/tests the Godot-free native graph.
- `sanitize` runs the Godot-free graph with the configured sanitizer preset.
- `dev` additionally builds the GDExtension development graph.
- `play --scenario smoke` launches the real Godot project and validates the ordinary boundary/screenshot evidence.
- `play --scenario offscreen` proves an observed living-need NPC can lose its Godot node, continue receiving authoritative movement while absent, and rematerialize from fresh observation plus a later authoritative sample.

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

## Godot playtest supervisor

`tools/play.py` is the single ordinary automated Godot playtest entry point. Add bounded scenarios to this supervisor rather than creating feature-specific long-lived runners.

The supervisor owns pinned Godot validation, metadata import, the repository playtest lock, one owned Godot process group, timeout/cleanup, required artifacts and scenario-specific semantic validation.

The default/local metadata-import path is headless. Linux CI may explicitly opt into the bounded Xvfb import path through the repository runtime helper when the pinned Godot editor cannot complete `--headless --import` on that runner. That exception is CI infrastructure, not a second gameplay runner.

Exact fixture values and debug JSON assertions are executable truth in `tools/play.py` and relevant tests. Do not copy them into architecture/roadmap prose.

Each run writes under:

```text
.cache/play/<run-id>/
  run.json
  stdout.log
  stderr.log
  final.png
  debug.json
```

### Baseline smoke

The `smoke` scenario validates authoritative identity/presence, controlled exact-spatial state, one current authoritative movement transition, presentation binding/reconciliation, duplicate/stale transition rejection, active locale probes and a rendered screenshot.

This is boundary/runtime evidence. It does **not** prove subjective control feel, audio output, GPU-driver coverage or performance.

### Offscreen continuation

The `offscreen` scenario is a presentation-lifecycle proof, not a scheduler or large-world simulation benchmark.

It must prove on one bounded run that:

- the living-need NPC is initially observed/materialized;
- presentation-only dematerialization removes its node/binding without removing observation or authoritative identity;
- the next authoritative locomotion tick succeeds while that NPC node is absent;
- the authoritative NPC sample progresses during that absent tick;
- absence does not silently rematerialize the NPC;
- fresh observation rematerializes a hidden shell;
- a later authoritative sample supplies current state and makes the shell visible again;
- controlled presentation ordering/revision/continuity guards remain valid throughout.

This proves **Godot-node absence does not stop the already implemented RestNeed causal path**. It does not prove regional scheduling, semantic travel, reduced-fidelity world simulation or time acceleration.

## Godot process/environment policy

Metadata import must remain non-interactive and bounded. The ordinary/default path uses Godot headless mode. On the current Linux CI runner, pinned Godot 4.7.1 aborts during this project's `--headless --import`; the persistent smoke lane therefore performs that import under the same Xvfb display already required for screenshot evidence.

The CI display-import workaround reads the renderer from canonical `godot/project.godot`, uses the `Dummy` audio driver and disables VSync. It must not introduce a second renderer setting or make CI scene state authoritative. If a later pinned Godot revision makes the headless path reliable on CI, prefer removing the workaround rather than preserving it by habit.

The visual scenarios likewise run under Xvfb, keep their renderer owned by `project.godot`, use `Dummy` audio and disable VSync. Missing ALSA/Pulse devices and virtual-display VSync limitations therefore do not masquerade as gameplay failures.

CI software rendering is compatibility evidence only. A successful llvmpipe/Mesa run does not prove real-GPU performance or driver-specific rendering correctness.

Unexpected engine `ERROR` lines should still be investigated; the pass/fail contract is the process result plus validated artifacts, not a rule that arbitrary stderr output is harmless.

## Godot smoke CI lane

The repository carries persistent `.github/workflows/godot-smoke.yml` runtime integration evidence. It installs the pinned Godot version from `tools/toolchain.lock.json`, builds the development/GDExtension graph, runs the RU and EN baseline smoke scenarios, runs the bounded RU offscreen continuation scenario, and uploads `.cache/play` evidence even on failure.

Documentation-only changes are excluded. The lane remains deliberately bounded: no narrow-layout matrix, interactive input-feel automation, performance thresholds or cross-platform GPU matrix without a demonstrated need.

A green Godot smoke check applies only to the exact commit it tested.

## Native CI lane

`.github/workflows/native.yml` runs clean `native` and `sanitize` configure/build/test passes on pull requests and pushes to `main`.

Native CI is independent evidence, not a substitute for local verification. A local failure must still be fixed locally rather than deferred with “CI will catch it.”

## Manual third-person verification

Control feel is empirical. Material input/camera/movement-presentation changes require a real local playtest of the affected device path in addition to automated smoke.

Check the relevant subset of keyboard release/drift, pointer lifecycle/mouse look, gamepad deadzones/range, semantic run/sprint selection, authoritative movement following, presentation turning/camera behavior, camera collision and visible jitter.

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
-> bounded Godot metadata import
-> affected bounded Godot scenario
-> inspect evidence/artifacts when presentation changes
```

### Godot presentation/control change

```text
Godot import/load
-> affected bounded scenario
-> keyboard/mouse/gamepad playtest when control feel changed
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
-> one semantic command/result/projection-or-transition round-trip scenario
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
