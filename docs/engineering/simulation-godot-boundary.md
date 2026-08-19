# Simulation ↔ Godot boundary

This guide is the implementation route for the authoritative-world decision in [`../decisions/0004-authoritative-world-presentation-boundary.md`](../decisions/0004-authoritative-world-presentation-boundary.md).

Use it when adding a mechanic that crosses from C++ Simulation into Godot presentation or from Godot input/UI into an authoritative world action.

## The shortest correct mental model

```text
Godot sends intent.
Simulation decides what happened.
Godot renders the result.
```

Godot may make presentation smoother. It may not invent authoritative state.

## Actor parity

A human-controlled actor and an NPC are simulated actors with stable `EntityId`s.

Control source is outside the actor's world identity:

```text
PlayerControls -> protocol control binding --+
                                           |
NPC decision -------------------------------+-> same domain action path
```

Do not add `is_player` branches to economy, relationships, institutions, inventory, ownership, damage or other world laws.

If a feature is possible for an NPC but impossible for the player only because the code uses a different domain API, the architecture is wrong.

## Write path: intent into world change

A Godot feature that wants to change the world submits a semantic command/intent through `SimFacade`/protocol.

Example:

```text
UI Buy button
  -> BuyItem command
  -> protocol validates boundary data
  -> Simulation checks real stock/funds/location/rights
  -> domain transition
  -> result + events
```

The UI does not pre-apply the transaction to authoritative state. Optimistic visuals are allowed only when explicitly reconciled.

Prefer verbs that describe the actor's attempted action.

Good:

```text
BuyItem
OfferTrade
Attack
Travel
ApplyForOffice
GiveGift
```

Bad:

```text
SetMoney
SetInventory
SetRelationship
SetPosition
SetShopOpen
```

## Read path: projections into presentation

Godot reads purpose-built projections, not domain objects.

A projection is:

- read-only;
- shaped for a presentation task;
- stamped with `SimulationTick`/`WorldRevision` where ordering matters;
- filtered to information the client is permitted to know;
- disposable/rebuildable from authoritative state.

Do not keep arbitrary protocol Dictionaries as a second long-lived world model in GDScript.

Typical future projections:

```text
ObservedWorldProjection
ControlledActorProjection
ShopProjection
InventoryProjection
RelationshipProjection
InstitutionProjection
JournalProjection
```

Do not create a universal `WorldProjection` that serializes every domain subsystem.

## Domain events vs projections

Use projections for current readable state.

Use domain events for facts that just happened and need explanation/feedback.

Example:

```text
Projection: actor now owns 4 apples and 13 coins
Events: TradeCompleted, ItemTransferred, MoneyTransferred
```

Godot can use events for animation, sound, notifications and causal history. It should render durable state from projections.

Do not make the event stream the save format unless a later ADR explicitly adopts event sourcing.

## Identity and presentation replicas

Godot presentation objects are keyed by simulation `EntityId`.

A presentation registry may conceptually own:

```text
EntityId -> scene node + render/interpolation state
```

Its operations are presentation lifecycle only:

```text
materialize(entity projection)
update(entity projection)
dematerialize(entity id)
```

Dematerialize means “stop representing this entity here”, not “delete this entity from the world”.

Never generate a new authoritative NPC/item because a scene wants one. Scene factories instantiate representations of already-existing projected entities.

## Observation / materialization boundary

The full simulation may contain far more entities than Godot needs to represent.

The production local-world projection should be bounded around the controlled context and filtered by simulation-owned observation/information rules.

Godot may additionally cull visually for performance. That visual culling cannot change simulation existence or reveal information omitted by the projection.

Keep these concepts distinct:

- authoritative existence;
- semantic location;
- actor knowledge/visibility;
- presentation materialization;
- camera frustum/occlusion.

Do not collapse them into one `visible` boolean.

## Spatial movement migration

The current cardinal `GridPosition` path is only a Milestone 0 transport probe.

The production third-person path must eventually become:

```text
PlayerControls semantic movement intent
    -> protocol actor intent
    -> authoritative spatial simulation
    -> revisioned movement samples/projection
    -> Godot presentation interpolation
```

The exact authoritative collision/navigation representation is not selected yet. Choose it from real world/terrain constraints and deterministic/performance requirements, not because `CharacterBody3D` already exists.

Until that migration is complete, `ThirdPersonPlayer` is a presentation/prediction shell only. Do not use its transform as proof of authoritative reachability, ownership, combat range or location-sensitive transaction success.

## Smooth presentation

Simulation tick rate and render frame rate are different clocks.

Keep authoritative samples with identity and ordering. Presentation can interpolate between previous/current samples.

Prediction is optional and normally limited to the locally controlled actor when real playtest latency requires it.

Prediction rules:

- use the same semantic intent sent to the simulation;
- store prediction separately from authoritative samples;
- reconcile on authoritative update;
- never predict systemic success such as item transfer, hit confirmation, ownership or access;
- prefer interpolation before adding more complex prediction.

## Trading example ownership

| Concern | Owner |
| --- | --- |
| merchant exists | Simulation |
| merchant is at market | Simulation |
| merchant owns/controls stock | Simulation |
| apple quantity | Simulation |
| current offer price | Simulation |
| buyer money | Simulation |
| whether trade is permitted | Simulation |
| transaction result | Simulation |
| shop list layout | Godot UI |
| hover/focus/button state | Godot UI/design system |
| coin/apple transfer animation | Godot presentation |
| purchase sound | Godot presentation |

## Bandit attack example ownership

| Concern | Owner |
| --- | --- |
| bandits choose/accept an attack intent | Simulation decision/action path |
| attackers/targets exist and are located | Simulation |
| hostility/legality/relationships | Simulation |
| damage/death/loot/consequences | Simulation |
| whether event falls in current materialization projection | Simulation/protocol observation policy |
| actor scene nodes | Godot presenter |
| combat animation/VFX/audio | Godot presentation |
| camera shake | Godot presentation |

Offscreen attacks do not need Godot nodes to resolve.

## Time vs revision

Do not use one counter for both concepts.

- `SimulationTick`: authoritative world time progression.
- `WorldRevision`: monotonic ordering of authoritative state changes.

An immediate command can increase revision without advancing time. Advancing time increases tick and can produce one or many revisions as systems act.

This distinction matters for schedules, prices, production, relationships, combat and stale presentation samples.

## GDExtension adapter rule

The adapter translates only.

Allowed:

- Godot primitives ↔ protocol DTOs;
- enums/error codes ↔ Godot-friendly values;
- projections/events ↔ Dictionaries/Arrays or later typed Godot-facing wrappers;
- diagnostics.

Not allowed:

- computing shop prices;
- deciding whether trade succeeds;
- changing relationships;
- spawning authoritative NPCs;
- resolving attacks;
- deciding ownership;
- treating scene transforms as authoritative world state.

If adapter code needs a world rule, move the rule into Simulation and expose a protocol operation/result.

## Extension checklist for a new feature

For a feature that crosses the boundary, answer in this order:

1. Which actor/entity IDs participate?
2. What semantic intent/command is being attempted?
3. Which authoritative state and rules decide it?
4. What result/error must the caller receive immediately?
5. Which domain events explain what happened?
6. Which read projection does Godot need afterward?
7. Does the projection expose only allowed information?
8. Which parts are presentation-only animation/interpolation/UI?
9. Can an NPC use the same domain capability without a second rule path?
10. What deterministic test proves the authoritative transition?
11. What Godot playtest proves the result is represented correctly?

If the answer starts with “Godot sets…”, re-check the ownership boundary.

## Deliberately not introduced yet

This foundation does not introduce:

- an ECS framework;
- a message broker;
- networking/server infrastructure;
- full event sourcing;
- regional simulation LOD;
- sharding;
- multithreaded world jobs;
- generic reflection/serialization frameworks;
- a universal projection/event bus.

Add those only when measurements or a concrete mechanic justify them.
