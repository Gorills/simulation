# Simulation ↔ Godot boundary

This guide is the **implementation procedure** for mechanics that cross between authoritative C++ Simulation and Godot presentation/input. Durable ownership decisions live in [`../ARCHITECTURE.md`](../ARCHITECTURE.md) and the relevant ADRs/models; current protocol schemas live in source/tests.

Start with:

- authoritative world/presentation boundary: [`../decisions/0004-authoritative-world-presentation-boundary.md`](../decisions/0004-authoritative-world-presentation-boundary.md)
- spatial authority: [`../decisions/0006-authoritative-spatial-contract.md`](../decisions/0006-authoritative-spatial-contract.md)
- simulation authority / future decision sources: [`../decisions/0009-simulation-authority-and-decision-sources.md`](../decisions/0009-simulation-authority-and-decision-sources.md)
- exact proof requirements: [`../VERIFICATION.md`](../VERIFICATION.md)

## Shortest correct mental model

```text
Godot sends semantic intent.
Simulation decides what happened.
Godot renders the authoritative result.
```

Godot may make presentation smoother. It may not invent world truth.

## Before adding a boundary feature

Identify, in order:

1. authoritative entities/facts involved;
2. semantic action or read the client actually needs;
3. Simulation rule that decides the result;
4. smallest protocol input/result/projection shape;
5. Godot presentation/interaction that consumes that shape;
6. executable native and Godot evidence required.

If step 3 is missing, do not fake it in GDScript to unblock UI. Add the minimum real world rule first or classify the feature as premature under [`../CHANGE_ADMISSION.md`](../CHANGE_ADMISSION.md).

## Write path: intent into world change

A Godot feature that wants to change the world submits semantic intent through protocol/GDExtension.

Good shapes describe attempts/goals, such as `BuyItem`, `OfferTrade`, `Attack`, `Travel`, `GiveGift` or `Move(direction, pace)`. Bad shapes directly assign authoritative state such as `SetMoney`, `SetRelationship`, `SetPosition`, `SetVelocity` or `SetTransform`.

The client supplies only information it legitimately controls. Simulation revalidates current state, prerequisites, permissions, reachability and applicable laws before committing the result.

Exact current command/DTO/function names are source-owned. Do not copy the full public method list into this guide.

## Actor parity and decision sources

Human input, deterministic NPC policy and a future external policy are different **intent sources**, not different world laws.

```text
human input --------------------+
NPC policy ---------------------+-> structured intent/goal -> Simulation validation/rules
future external policy/LLM -----+
```

Do not add `is_player` branches to movement, economy, relationships, institutions, inventory, ownership, damage or another systemic rule merely because the source differs.

A future external/LLM policy proposes allowlisted high-level structured input from bounded actor-visible context; it does not mutate world fields or drive per-frame locomotion. See ADR 0009.

## Read path: purpose-built projections

Godot reads purpose-built projections/read models, not domain objects or mutable `WorldState`.

A projection answers one presentation question with only information the client is allowed to know. Do not introduce one universal projection containing every person, secret, inventory, market and subsystem field.

Adapters translate representation only; they do not infer hidden domain meaning that the protocol did not provide.

## Transition-result path

Some authoritative transitions need ordered result data rather than repeated polling. Continuous movement is the current example.

```text
semantic intent(s)
  -> one authoritative transition
  -> ordered result data with identity/order/continuity
  -> adapter conversion
  -> presentation validation/reconciliation
```

Result order/identity must let presentation reject stale, duplicate or malformed data before application. Do not invent an extra sequence counter preemptively. Before another production system independently advances simulation time, re-admit the ordering model under ADR 0009.

## Exact spatial state, observation and materialization

Keep these concepts independent:

```text
authoritative existence
semantic location
optional exact SpatialState
actor knowledge/observation
Godot materialization
frustum/occlusion
```

Godot materialization never creates world existence. Removing a scene representation never deletes the simulated entity or stops its authoritative causal work.

An **observed non-controlled actor may intentionally have no Godot binding/node**. When an ordered authoritative movement result contains a sample for that actor:

- validate the sample shape, identity/order/protocol and observed membership normally;
- do not require a presentation root merely to accept the authoritative transition;
- skip only the transform/presentation write while no node exists;
- keep controlled-actor presentation requirements intact for the current local client;
- continue rejecting samples for entities outside the observed set.

When presentation policy wants the actor back, apply a fresh observed-world projection to create a new hidden shell. The next authoritative sample supplies its current exact pose/velocity/continuity state and reveals the shell. Do not resurrect the old scene transform as world truth.

This separation is currently exercised by the bounded living-need `offscreen` scenario. It proves presentation-node absence is not a causal prerequisite; it does **not** introduce a scheduler, regional LOD, semantic travel model or time acceleration.

When exact geometry matters, Simulation owns the authoritative spatial outcome. When it does not, an entity may continue causally without a microscopic pose. Do not manufacture coordinates for distant actors merely so every entity fits one presentation representation.

The current bounded solver is an authority proof, not a commitment to a project-authored general physics/navigation engine. A future Godot-free dependency can sit behind the Simulation boundary when a concrete requirement justifies it.

## GDExtension adapter rule

Allowed:

- Godot primitives ↔ protocol DTOs;
- explicit unit conversion;
- enum/error conversion;
- projections/events/ordered result data ↔ Godot-friendly values;
- registration and diagnostics.

Not allowed:

- collision/movement resolution;
- actor capability formulas;
- prices/trade decisions;
- relationship changes;
- authoritative spawning;
- ownership/damage decisions;
- using scene transforms as world truth.

The adapter should remain small enough that review can distinguish translation from gameplay policy immediately.

## Godot presentation rule

Godot owns presentation identity binding, input sampling, interpolation/reconciliation, animation, audio, VFX, camera and UI.

Use one presentation identity owner keyed by authoritative `EntityId`. Do not build parallel feature-local registries for combat, HUD, inventory or NPC scripts.

Initial placement/discontinuous relocation and continuous movement are different presentation operations. Continuity identifiers must be respected so presentation does not interpolate through a teleport/respawn/other discontinuity.

Prediction is optional and evidence-driven. If introduced, predicted state stays separate from authoritative state, reconciles to authoritative results and cannot grant systemic success.

## Time and process boundaries

Godot render/physics frames are presentation scheduling facts, not authoritative world clocks.

External/network/LLM calls must never block a fixed authoritative Simulation step. External decisions arrive later as untrusted structured input and are validated against then-current state.

If a future transport decouples result arrival from local Godot physics timing, re-admit buffering/interpolation based on measured transport behavior rather than treating the current in-process schedule as networking architecture.

## Offscreen interaction example

```text
Simulation state says an implemented causal action/need continues
  -> Simulation commits authoritative state/result
  -> no Godot node is required for the non-controlled actor
  -> later observation/materialization renders current authoritative state
```

Camera/node absence is not world absence.

## Extension checklist

For a feature crossing the boundary, answer:

1. Which authoritative entities participate?
2. What semantic intent/read is needed?
3. Which Simulation state/rules decide it?
4. What result/error/event/projection is required?
5. Which fields are actually observable by the client?
6. Which parts are presentation-only?
7. Can NPC/human-controlled actors use the same world capability when prerequisites match?
8. What deterministic native test proves the transition?
9. What bounded Godot scenario proves presentation of the result?
10. Does the change duplicate a current schema/status/test fact owned elsewhere?

If the answer begins with “Godot sets the world state”, move the decision behind protocol.

## Do not prebuild

Without a concrete blocked capability, do not introduce generic ECS, networking/server infrastructure, full event sourcing, regional runtime LOD/sharding, multithread world jobs, universal projection/event buses, universal entity-scene factories, production navigation/physics dependencies, generic modifier stacks, generic NPC brain abstractions, LLM provider/prompt/memory frameworks or prediction/rollback architecture.

Use the first real requirement to admit the smallest durable addition.
