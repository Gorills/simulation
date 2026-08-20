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
python tools/dev.py play --scenario shortage --locale ru
python tools/dev.py play --scenario shortage --locale en
python tools/dev.py play --scenario gift --locale ru
python tools/dev.py play --scenario gift --locale en
python tools/dev.py play --scenario work --locale ru
python tools/dev.py play --scenario work --locale en
python tools/dev.py play --scenario transfer --locale ru
python tools/dev.py play --scenario transfer --locale en
python tools/dev.py play --scenario offscreen --locale ru
python tools/dev.py play --scenario rest_interference --locale ru
```

Use only the subset required by the change, but do not claim a gate that did not run.

- `native` builds/tests the Godot-free native graph.
- `sanitize` runs the Godot-free graph with the configured sanitizer preset.
- `dev` additionally builds the GDExtension development graph.
- `play --scenario smoke` launches the real Godot project and validates the ordinary boundary/screenshot evidence.
- `play --scenario shortage` proves the first Milestone 2 vertical path from authoritative household discovery through autonomous post-movement Consume to localized shortage feedback without a player economic command.
- `play --scenario gift` proves the controlled actor draws rule-defined grain from its own store, later gifts the entire carry at the short household's store, changes authoritative carry/stock through semantic commands, and preserves the existing RestNeed interference at that shared place.
- `play --scenario work` proves the controlled actor discovers the authoritative field, reaches it through ordinary locomotion, completes one amount-less Work command, increases the configured destination stock by the Core-owned yield, exhausts the bounded Work availability, and receives localized field/HUD feedback.
- `play --scenario transfer` proves the controlled actor executes the surplus household's standing transfer pledge at its own store, moves the entire remaining pledged grain into the durable destination household, clears the pledge, and receives localized pledge/HUD feedback.
- `play --scenario offscreen` proves an observed living-need NPC can lose its Godot node, continue receiving authoritative movement while absent, and rematerialize from fresh observation plus a later authoritative sample.
- `play --scenario rest_interference` proves the real client observes the first need as `traveling`, creates a `blocked` outcome through ordinary controlled locomotion, renders localized blocked feedback, then removes the obstruction through the same movement path and observes later `satisfied` feedback.

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

For the first Living Need, native evidence owns the actual satisfaction/blocking world-law assertions. Protocol evidence additionally proves the purpose-built `LivingNeedProjection` is derived from the authoritative NPC and tracks current tick/revision without becoming mutable need state.

For the first player resource intervention, native evidence owns carry/conservation/refusal/overflow law. Protocol evidence proves carry/member-household state is read from Core, Draw/Deposit/Gift accept no client-authored amount, accepted commands report the authoritative moved quantity and revision-only result, and a resource revision between locomotion ticks does not poison the next movement batch.

For bounded Work, native evidence owns field assignment validation, exact field occupancy, fixture yield, finite exhaustion, overflow/refusal atomicity, snapshot continuation and equivalent-actor parity. Protocol evidence proves the field projection comes from that Core assignment/place, controlled Work accepts no yield/destination/amount payload, one accepted command reports the authoritative produced/resulting quantities on one revision-only transition, exhausted retry is non-mutating, and locomotion can continue afterward.

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

Individual scenarios may add bounded named screenshots such as `blocked.png` when a transient visible state is itself acceptance evidence.

### Baseline smoke

The `smoke` scenario validates authoritative identity/presence, controlled exact-spatial state, one current authoritative movement transition, presentation binding/reconciliation, duplicate/stale transition rejection, active locale probes and a rendered screenshot.

This is boundary/runtime evidence. It does **not** prove subjective control feel, audio output, GPU-driver coverage or performance.

### Autonomous shortage

The `shortage` scenario is the first Milestone 2 vertical resource checkpoint. It uses only ordinary runtime advancement and read-only projections; it has no scenario-only stock setter or player economic command.

It must prove on one bounded run that:

- initial village household discovery, observed-world state, controlled spatial initialization and LivingNeed discovery are read before runtime advancement from one unchanged world revision;
- the client resolves the tracked NPC, its household and the shared rest/store position from authoritative projections rather than fixture EntityIds or coordinates in Godot;
- the tracked household begins adequate;
- ordinary locomotion advances the NPC to its authoritative store and application-level autonomous Consume occurs only afterward;
- the final movement batch retains its movement revision while the resource projection reports the same `SimulationTick` at exactly one later `WorldRevision`;
- the household's protocol status becomes `shortage` with lower authoritative stock and an unchanged threshold;
- Godot does not derive or mutate shortage, but renders the status supplied by the resource projection;
- RU and EN runs render localized shortage/scenario feedback and capture the standard screenshot artifact;
- presentation retains the movement revision for controlled spatial state while its latest observed world revision reconciles to the post-Consume resource revision.

This scenario proves autonomous scarcity can become player-visible through the real boundary. It does not prove recurring meal cadence, production cadence, standing transfers or markets.

### Gift shortage relief

The `gift` scenario is the M2.6 controlled-actor intervention checkpoint. It uses the ordinary acceptance village, locomotion path, existing autonomous Consume policy and M2.5 World transfer laws; it has no stock setter, teleport or client-supplied grain amount.

It must prove on one bounded RU/EN run that:

- controlled carry/member-household projection starts on the same unchanged startup revision as village resource discovery;
- Draw at the controlled actor's own store moves the authoritative rule-defined amount into carry and commits exactly one revision without advancing simulation time;
- the neighbour household becomes short through the already-proven autonomous Consume path before the player approaches it;
- the controlled actor reaches the receiving household's exact store tolerance through ordinary authoritative locomotion;
- Gift targets a discovered other household identity, transfers the entire authoritative carry, clears carry, increases receiving stock by exactly the moved quantity, and changes `WorldRevision` exactly once without changing `SimulationTick`;
- the short household becomes adequate in the bounded fixture after the transfer;
- the living-need projection is `blocked` at the Gift location because the receiving store shares the M1 rest footprint and occupancy is intentionally non-exclusive;
- the movement batch immediately before Gift remains the controlled spatial revision while the post-Gift read/presentation reconciles to the later resource revision;
- RU and EN render localized carry/action/scenario feedback plus the authoritative post-Gift household status and capture the standard screenshot artifact.

This proves one real player intervention path using the same Core laws as ordinary actors. It does not introduce amount selection, trade, standing pledges, market state or a general inventory system.

### Work shortage relief

The `work` scenario is the M2.8 controlled-actor production checkpoint. It uses the M2.7 bounded Work law and ordinary locomotion; it has no stock setter, teleport, client-supplied amount, client-supplied yield or client-selected destination.

It must prove on one bounded RU/EN run that:

- field-work discovery exposes the authoritative work place/footprint, durable destination household, positive fixture yield and remaining completion count from the same startup world state as household-resource discovery;
- the configured destination household becomes short through the already-proven autonomous Consume path before player Work;
- Godot derives the field cue/target from the field projection rather than a hardcoded field coordinate;
- the controlled actor reaches exact field tolerance through ordinary authoritative locomotion;
- one semantic Work command adds exactly the projected fixture yield to the durable destination household and decrements remaining Work availability exactly once;
- the Work result reports the same `SimulationTick` as the pre-command field/resource read and exactly one later `WorldRevision`;
- the bounded destination becomes adequate after the fixture yield while its shortage threshold stays unchanged;
- field projection after Work retains field/destination/yield identity and reports zero remaining completions on the Work result revision;
- a second Work attempt returns typed `work_exhausted` and changes neither field availability nor resource revision/state;
- the movement batch immediately before Work remains the controlled spatial revision while presentation/resource reads reconcile to the later Work revision;
- RU and EN render localized field cue, Work availability/action/scenario feedback and authoritative post-Work household status, with the standard screenshot artifact.

This proves a second player intervention path through the same actor-generic Core Work law. It does not prove labor duration, recurring production, crop calendars, wages, tenure or automatic NPC Work policy.

### Household transfer

The `transfer` scenario is the M2.10 controlled-actor standing-transfer checkpoint. It uses the M2.9 household-transfer law and ordinary locomotion; it has no stock setter, teleport, client-supplied amount or live shortage rebinding.

It must prove on one bounded RU/EN run that:

- pledge discovery exposes remaining pledged grain and the durable destination household from the same startup world state as household-resource discovery;
- the configured destination household becomes short through the already-proven autonomous Consume path before player execution;
- the controlled actor occupies its own household store through ordinary presence (the acceptance spawn already places that actor there);
- one semantic pledge-execution command moves the entire remaining pledged quantity from source stock into the destination household, sets remaining pledge to zero, and advances `WorldRevision` exactly once without changing `SimulationTick`;
- the destination becomes adequate after the transfer while its shortage threshold stays unchanged;
- a second execution returns typed `pledge_zero` and changes neither stocks nor revision;
- RU and EN render localized pledge/inventory/action/scenario feedback plus the authoritative post-transfer household status, with the standard screenshot artifact.

This proves the third player intervention path through the same actor-generic Core transfer law. It is not trade, payment, or a second commodity.

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

### Rest interference/help

The `rest_interference` scenario is the Milestone 1 player-exposure proof.

It must use the ordinary controlled movement boundary — no scenario-only world setter — and prove:

- the purpose-built need projection begins `traveling`;
- the controlled actor enters the NPC's assigned rest tolerance through authoritative locomotion;
- Core later derives `blocked` when the controlled actor occupies the rest footprint and the NPC would otherwise walk onto it;
- the blocked projection carries current identity/tick/revision/version context;
- Godot renders localized blocked feedback and captures `blocked.png`;
- the controlled actor then leaves through the same authoritative movement path;
- a later need projection becomes `satisfied`;
- the final localized HUD shows the satisfied outcome;
- the final need projection, authoritative movement batch and presentation tick/revision align.

The scripted controller is acceptance automation over the real client boundary. It does not claim subjective keyboard/gamepad feel and does not introduce a second gameplay command.

### Milestone 1 acceptance gate

Milestone 1 may be marked accepted only when the exact candidate revision has green evidence for:

- native/sanitizer verification including RestNeed blocking/shared-movement behavior;
- protocol projection/build verification;
- RU + EN baseline Godot smoke;
- bounded offscreen continuation;
- bounded RU rest-interference/help with localized blocked/satisfied feedback and rendered artifacts.

If the candidate fails the interference scenario, M1 remains in progress. Do not weaken the scenario merely to make the milestone green.

### Milestone 2 acceptance gate

Milestone 2 may be marked accepted only when the exact candidate revision has green evidence for:

- native/sanitizer verification including Consume/Draw/Deposit/Gift/Work/HouseholdTransfer validation, empty/full/exhausted/zero-pledge/overflow refusal, snapshot continuation, actor parity, and the Gift/Work/Transfer permutation claim;
- protocol projection/command verification for village discovery, carry, field work, standing transfer, and revision-only resource commands;
- architecture check forbidding Godot in `src/sim` / `src/protocol`;
- localization catalog integrity for new `UI_*` keys;
- one bounded release-configuration measurement of locomotion-call cost (including post-movement Consume) and bridge/projection cost against [`PERFORMANCE.md`](PERFORMANCE.md) budgets;
- RU + EN baseline Godot smoke;
- bounded RU/EN autonomous shortage, including Consume while the living-need NPC Godot node is absent;
- bounded RU/EN Gift, Work and household-transfer verticals with localized feedback;
- bounded offscreen continuation and RU rest-interference/help with unchanged M1 semantics.

If any required path is missing or a check did not run, M2 remains in progress. Do not claim acceptance from compile-green or from a Godot-only stock label.

### M2.11 measured performance

Recorded on Linux 6.14.0-37-generic x86_64, Intel Core i7-4770 @ 3.40 GHz (8 logical CPUs), CMake `release` preset (`CMAKE_BUILD_TYPE=Release`), 256 samples after 32 warmup ticks of the acceptance village:

```text
./build/release/protocol_tests --gtest_filter='M2Performance.*'
M2 acceptance-village locomotion p99_ms=0.001821 budget_ms=4
bridge/projection p99_ms=0.001064 budget_ms=1
```

Both p99 values are inside the [`PERFORMANCE.md`](PERFORMANCE.md) 4 ms locomotion and 1 ms bridge budgets on this machine. This is Core/protocol call cost, not a Godot frame-time measurement.

## Godot process/environment policy

Metadata import must remain non-interactive and bounded. The ordinary/default path uses Godot headless mode. On the current Linux CI runner, pinned Godot 4.7.1 aborts during this project's `--headless --import`; the persistent smoke lane therefore performs that import under the same Xvfb display already required for screenshot evidence.

The CI display-import workaround reads the renderer from canonical `godot/project.godot`, uses the `Dummy` audio driver and disables VSync. It must not introduce a second renderer setting or make CI scene state authoritative. If a later pinned Godot revision makes the headless path reliable on CI, prefer removing the workaround rather than preserving it by habit.

The visual scenarios likewise run under Xvfb, keep their renderer owned by `project.godot`, use `Dummy` audio and disable VSync. Missing ALSA/Pulse devices and virtual-display VSync limitations therefore do not masquerade as gameplay failures.

CI software rendering is compatibility evidence only. A successful llvmpipe/Mesa run does not prove real-GPU performance or driver-specific rendering correctness.

Unexpected engine `ERROR` lines should still be investigated; the pass/fail contract is the process result plus validated artifacts, not a rule that arbitrary stderr output is harmless.

## Godot smoke CI lane

The repository carries persistent `.github/workflows/godot-smoke.yml` runtime integration evidence. It installs the pinned Godot version from `tools/toolchain.lock.json`, builds the development/GDExtension graph, runs RU and EN baseline smoke, bounded RU/EN autonomous shortage, bounded RU/EN Gift, bounded RU/EN Work, bounded RU/EN household transfer, bounded RU offscreen continuation and bounded RU rest-interference/help, then uploads `.cache/play` evidence even on failure.

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