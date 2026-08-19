# Runtime architecture

This document owns **durable runtime boundaries, dependency direction, ownership and integration seams**. It intentionally does not mirror current protocol versions, test inventories, fixture numbers, milestone status or the exact public method list of a particular revision.

Canonical detail lives in:

- product outcomes and playable invariants: [`PRODUCT.md`](PRODUCT.md)
- simulation causality/fidelity/determinism: [`MODELING.md`](MODELING.md)
- current milestone status and sequencing: [`ROADMAP.md`](ROADMAP.md)
- proof obligations and evidence: [`VERIFICATION.md`](VERIFICATION.md)
- documentation ownership: [`INDEX.md`](INDEX.md)
- Simulation ↔ Godot implementation procedure: [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md)
- exact spatial semantics: [`models/spatial-location.md`](models/spatial-location.md)
- grounded locomotion semantics: [`models/grounded-locomotion.md`](models/grounded-locomotion.md)
- first living-need semantics: [`models/living-need.md`](models/living-need.md)
- accepted architectural rationale: [`decisions/`](decisions/)

Current source, build configuration, executable tests and lock files are authoritative for what is actually implemented.

## Runtime dependency graph

The dependency direction is one-way:

```text
Godot presentation/input/UI
    |
    v
GDExtension adapter
    |
    v
Application protocol
    |
    v
C++23 Simulation Core

Native tools/tests ---> protocol / Simulation Core
Content data -------> Simulation Core
```

Hard boundary:

```text
godot/ -> src/adapters/gdextension -> src/protocol -> src/sim

src/sim      !-> src/protocol / Godot / godot-cpp / GDExtension
src/protocol !-> Godot / godot-cpp / GDExtension
godot/       !-> authoritative world state or systemic outcomes
```

The CMake target graph and architecture checks are the executable expression of this direction. Prose must not invent a dependency edge that the build does not contain.

## Ownership by layer

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | entity/world truth, causal rules, authoritative time/state transitions, semantic and exact location when required, deterministic domain outcomes | protocol DTOs, Godot types, input devices, render/UI state |
| `src/protocol` | controller/session binding, semantic commands/intents, validation/translation, results/events/projections/ordered transition data | duplicated world rules, rendering, mutable exported `WorldState` |
| `src/adapters/gdextension` | Godot-facing ↔ protocol translation, unit/enum conversion, registration and diagnostics | prices, relationships, collision/movement resolution, authoritative spawning or other world decisions |
| `godot/` | input sampling, presentation replicas, interpolation/reconciliation, scenes, camera, audio, animation, VFX and UI | entity existence, ownership, economy, social/political state, damage, authoritative movement or other systemic outcomes |
| tools/tests | verification, scenarios, developer orchestration and diagnostics | an alternate simulator |

The authoritative world exists once: in Simulation.

## Authority is ownership of truth, not authorship of every algorithm

Simulation deciding an authoritative result does not imply every low-level algorithm must be handwritten by this repository. A future Godot-free physics, collision or navigation library may be used behind a Simulation-owned contract when a concrete capability requires it.

The dependency remains an implementation detail rather than a second authority when Simulation owns inputs/state/results, library types do not leak through domain boundaries, headless verification remains possible, and determinism/replay requirements are evaluated explicitly.

Do not grow the current bounded spatial solver into a general engine merely because more physics or navigation could exist someday. See [`decisions/0009-simulation-authority-and-decision-sources.md`](decisions/0009-simulation-authority-and-decision-sources.md).

## Actor identity and decision sources

A human-controlled actor and an NPC are ordinary simulated actors with the same world capabilities when the same prerequisites hold. Human control is a relationship outside world identity, not a privileged actor species.

```text
human input --------------------+
                                |
deterministic NPC policy -------+-> structured intent/goal
                                |          |
future external policy/LLM -----+          v
                                      Simulation rules
                                           |
                                           v
                                   authoritative result
```

Decision technology does not own actor truth. A future external/LLM policy may propose bounded structured high-level actions, but it must not directly mutate positions, resources, relationships or other authoritative state. External output is validated like any other untrusted input, and time-critical Simulation steps never wait on a provider call.

The durable external-decision contract is owned by ADR 0009; provider/API implementation is deliberately deferred.

## Protocol boundary

Clients express **intent**, never desired systemic state:

```text
input/UI/policy intent
  -> protocol validation/binding
  -> authoritative Simulation transition
  -> result/events/ordered transition data
  -> purpose-built read projections
```

A command or intent describes what an actor attempts. Simulation determines the result after current world rules and prerequisites.

Read models are purpose-built. Do not export mutable domain objects or a universal projection containing unrelated inventories, secrets, relationships, markets and internal subsystem state.

Transition-result streams such as movement samples are results of an authoritative transition, not client-authored state and not polling aliases for projections.

The exact current DTO names, fields, protocol version and public method surface live in `src/protocol`, adapter code and executable tests. Broad architecture prose should describe their durable role, not duplicate their current schema.

## Time, revision and spatial continuity

Keep distinct concepts distinct:

- `SimulationTick` — simulation-time progression/context;
- `WorldRevision` — authoritative mutation ordering;
- `SpatialEpoch` — continuity identity for interpolation across exact-spatial samples.

A mutation need not advance simulation time. A discontinuous relocation may change spatial continuity. Presentation frames and wall-clock timestamps are not substitutes for any of these contracts.

The current locomotion stream may impose ordering assumptions that are valid for the current in-process path. Before slower systems, offscreen continuation or another production transition advances simulation time independently, re-admit that ordering contract as required by ADR 0009 instead of silently treating locomotion-specific consecutiveness as a universal clock rule.

## Spatial state and materialization

These are separate states:

```text
authoritative entity existence
semantic location
optional exact SpatialState
actor knowledge/observation
Godot materialization
visual frustum/occlusion
```

An entity can exist and remain causally active without an exact 3D pose. Exact spatial state is retained when current causality depends on geometry, movement, reachability or interaction.

Godot materialization never creates or deletes authoritative existence. When an entity becomes exact-spatial, Simulation must provide/derive the authoritative pose; a scene node cannot invent it.

Detailed coordinate, unit and movement semantics belong to the spatial and locomotion model documents and their ADRs, not here.

## Offscreen simulation

The simulated world continues when parts of it are not represented in Godot. Economy, relationships, politics, needs, travel and other implemented causal systems advance according to Simulation rules at the fidelity required by causality.

Do not equate presentation distance with model fidelity, and do not require universal per-frame exact-spatial updates for distant actors. Presentation materialization, causal-model resolution and performance architecture are separate concerns.

ADR 0005 owns causal fidelity; ADR 0009 clarifies selective exact spatial work and future decision-source boundaries.

## GDExtension seam

Godot crosses into native gameplay through one deliberately boring adapter seam:

1. receive a Godot-facing semantic request or read;
2. translate primitive/DTO values into protocol values;
3. invoke protocol/application behavior;
4. translate results/projections/events/transition data back to Godot-friendly values;
5. expose diagnostics without embedding domain policy.

Unit conversion belongs at this boundary. World rules, collision resolution, economy, relationships and authoritative spawning do not.

If a systemic rule is implemented inside a `GDCLASS`, GDScript UI node or serialization helper, the boundary is violated.

## Godot client

Godot is the interactive presentation client. It may sample input, choose presentation affordances, interpolate/reconcile authoritative samples, rotate visual children, drive animation/audio/VFX, manage camera behavior and render UI.

`WorldPresentation` is the single owner of Simulation identity ↔ presentation binding. Feature scripts must not create competing `EntityId -> Node` registries or assign authoritative identities themselves.

Presentation may predict or smooth when measured experience requires it, but predicted state remains separate from authoritative state and cannot grant ownership, damage, access, purchases or another systemic success.

The UI uses the project-wide design system; feature UI does not become a second source of economy/social/inventory truth. See ADR 0003 and [`engineering/ui-design-system.md`](engineering/ui-design-system.md).

## Bootstrap/probe paths

Temporary bootstrap transport probes may exist to prove a boundary. Their names or data shapes are not production domain contracts and must not silently become the implementation of later gameplay.

When a production capability supersedes a probe, keep the probe isolated until it can be removed cleanly. Current source/tests own the exact probe surface.

## Vertical capability rule

A coherent player-facing capability crosses layers in this order:

```text
minimal authoritative world rule
  -> semantic protocol boundary
  -> adapter translation
  -> Godot affordance/feedback
  -> focused executable proof
  -> bounded real playtest
```

Do not add several milestones of invisible simulation infrastructure ahead of a playable causal loop. Do not implement the same rule again in Godot to make the presentation work sooner.

## External AI Layer boundary

`Gorills/ai-layer` is a development control plane, not a runtime/build dependency of the game repository. Repository code/tests/docs own game contracts; external development tooling may own workflow/context outside the runtime graph.

A future in-game external/LLM decision source is a different concern and is governed by ADR 0009. Do not conflate development orchestration with runtime NPC decision policy.

## Mechanical verification

Prefer executable boundaries to duplicated prose:

- the target graph enforces dependency direction;
- native tests exercise Simulation without Godot;
- protocol tests exercise the application boundary;
- architecture checks reject Godot leakage into Core/protocol;
- bounded Godot smoke verifies the real adapter/presentation round-trip;
- model-specific tests own detailed mechanic invariants.

Exact commands and current evidence belong to [`VERIFICATION.md`](VERIFICATION.md). Current status belongs to [`ROADMAP.md`](ROADMAP.md).
