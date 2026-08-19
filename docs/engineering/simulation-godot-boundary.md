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

1. the authoritative entities/facts involved;
2. the semantic action or read the client actually needs;
3. the Simulation rule that decides the result;
4. the smallest protocol input/result/projection shape;
5. the Godot presentation/interaction that consumes that shape;
6. the executable native and Godot evidence required.

If step 3 is missing, do not fake it in GDScript to unblock the UI. Add the minimum real world rule first or classify the feature as premature under [`../CHANGE_ADMISSION.md`](../CHANGE_ADMISSION.md).

## Write path: intent into world change

A Godot feature that wants to change the world submits semantic intent through protocol/GDExtension.

Good shapes describe an attempt or goal, for example:

```text
BuyItem
OfferTrade
Attack
Travel
GiveGift
Move(direction, pace)
```

Bad shapes directly assign authoritative state:

```text
SetMoney
SetInventory
SetRelationship
SetPosition
SetVelocity
SetTransform
```

The client supplies only information it legitimately controls. Simulation revalidates current state, prerequisites, permissions, reachability and other applicable rules before committing the result.

The exact current command/DTO/function names are source-owned. Do not copy the full public method list into this guide.

## Actor parity and decision sources

Human input, deterministic NPC policy and a future external policy are different **intent sources**, not different world laws.

```text
human input --------------------+
NPC policy ---------------------+-> structured intent/goal -> Simulation validation/rules
future external policy/LLM -----+
```

Do not add `is_player` branches to movement, economy, relationships, institutions, inventory, ownership, damage or another systemic rule merely because the source of intent differs.

A future external/LLM policy belongs above this same boundary. It proposes allowlisted high-level structured input from bounded actor-visible context; it does not mutate world fields or drive per-frame locomotion. See ADR 0009.

## Read path: purpose-built projections

Godot reads purpose-built projections/read models, not domain objects or mutable `WorldState`.

A projection should answer one presentation question with only the information the client is allowed to know. Examples include observed identity/presence, controlled exact-spatial state, shop offers, inventory presentation, relationship presentation or institution state when those mechanics actually exist.

Do not introduce one universal projection containing every person, secret, inventory, market and subsystem field.

Adapters translate representation only; they do not infer hidden domain meaning that the protocol did not provide.

## Transition-result path

Some authoritative transitions need an ordered result stream rather than repeated polling of a read projection. Continuous movement is the current example.

The durable rule is:

```text
semantic intent(s)
  -> one authoritative transition
  -> ordered result data carrying enough identity/order/continuity information
  -> adapter conversion
  -> presentation validation/reconciliation
```

Result order/identity must be explicit enough that presentation can reject stale, duplicate or malformed data before applying it.

Do not invent an extra global sequence counter preemptively. Before another production system advances simulation time independently of the current movement stream, re-admit the ordering model under ADR 0009.

Exact result field names and current ordering assertions live in protocol/Godot source and tests.

## Exact spatial state and materialization

Keep these concepts independent:

```text
authoritative existence
semantic location
optional exact SpatialState
actor knowledge/observation
Godot materialization
frustum/occlusion
```

Godot materialization never creates world existence. Removing a scene representation never deletes the simulated entity.

When exact geometry matters, Simulation owns the authoritative spatial outcome. When it does not, an entity may continue causally without a microscopic pose. Do not manufacture exact coordinates for distant actors merely so every entity fits one presentation representation.

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

The adapter should remain small enough that a code review can distinguish translation from gameplay policy immediately.

## Godot presentation rule

Godot owns presentation identity binding, input sampling, interpolation/reconciliation, animation, audio, VFX, camera and UI.

Use one presentation identity owner keyed by authoritative `EntityId`. Do not build parallel feature-local registries for combat, HUD, inventory or NPC scripts.

Initial placement/discontinuous relocation and continuous movement are different presentation operations. Continuity identifiers must be respected so presentation does not interpolate through a teleport/respawn/other discontinuity.

Prediction is optional and evidence-driven. If introduced, predicted state stays separate from authoritative state, reconciles to authoritative results and cannot grant systemic success.

## Time and process boundaries

Godot render/physics frames are presentation scheduling facts, not authoritative world clocks.

External/network/LLM calls must never block a fixed authoritative Simulation step. External decisions arrive later as untrusted structured input and are validated against then-current state.

If a future transport decouples result arrival from local Godot physics timing, re-admit buffering/interpolation based on measured transport behavior rather than treating the current in-process schedule as networking architecture.

## Trading example

```text
Godot requests BuyItem
  -> protocol validates request shape/controller binding
  -> Simulation validates merchant/stock/funds/access and commits transaction
  -> result/events/projections cross adapter
  -> Godot updates inventory/shop presentation and feedback
```

The UI cannot create an item by mutating a local array. An NPC buyer uses the same transaction rule when equivalent prerequisites hold.

## Offscreen interaction example

```text
Simulation state says two actors can participate in an implemented causal event
  -> Simulation resolves authoritative consequence
  -> state/events change without any required Godot node
  -> later observation/materialization renders the resulting state
```

Camera absence is not world absence.

## Extension checklist

For a feature crossing the boundary, answer:

1. Which authoritative entities participate?
2. What semantic intent/read is needed?
3. Which Simulation state/rules decide it?
4. What result/error/event/projection is required?
5. Which fields are actually observable by the client?
6. Which parts are presentation-only?
7. Can an NPC/human-controlled actor use the same world capability when prerequisites match?
8. What deterministic native test proves the transition?
9. What bounded Godot smoke/playtest proves presentation of the result?
10. Does the change duplicate a current schema/status/test fact that belongs in source, `ROADMAP.md` or `VERIFICATION.md` instead?

If the answer begins with “Godot sets the world state”, stop and move the decision behind protocol.

## Do not prebuild

Without a concrete blocked capability, do not introduce:

- generic ECS;
- networking/server infrastructure;
- full event sourcing;
- regional runtime LOD/sharding;
- multithread world jobs;
- universal projection/event bus;
- universal entity-scene factory;
- production navigation/physics dependency;
- generic movement modifier/effect stacks;
- generic NPC planner/brain abstraction;
- LLM provider/prompt/memory framework;
- prediction/rollback architecture.

Use the first real requirement to admit the smallest durable addition.
