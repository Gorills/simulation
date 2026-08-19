# ADR 0004: Authoritative world and Godot presentation boundary

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../MODELING.md`](../MODELING.md) · [`../engineering/simulation-godot-boundary.md`](../engineering/simulation-godot-boundary.md) · [`../engineering/SOURCES.md`](../engineering/SOURCES.md)

Supersedes ADR 0002 where that decision granted Godot ownership of authoritative local character position/collision state. ADR 0002 remains valid for input decomposition, camera ownership, control profiles and presentation-side responsiveness.

## Context

The game is a systemic RPG in which economy, politics, institutions, social relationships, magic, movement, ownership, inventory, violence and long-term consequences belong to one persistent world model.

Godot is the interactive renderer and input client for that world. It must not become a second simulation merely because an entity is currently visible.

The same rules must apply to a human-controlled actor and an NPC. A human-controlled person can buy goods, open a shop, own property, join an institution, form relationships, become a bandit, attack travelers or suffer the same consequences as another simulated person. The simulation must therefore model the person first and the source of control second.

A town also continues to exist when the player leaves it. Shops trade, people move, obligations mature, politics changes and violence can occur without Godot keeping scene nodes alive for those entities. Returning to the town reveals the resulting state rather than reconstructing a scripted approximation.

## Decision

### The Simulation Core is the only world authority

Authoritative state lives in C++ Simulation Core. This includes, when the corresponding mechanic exists:

- entity identity and existence;
- actor/item/place semantic location;
- movement state and authoritative spatial results;
- inventory, containment, ownership and access rights;
- money, stock, prices, production, consumption and trade;
- relationships, obligations, reputation, hostility and social knowledge;
- institutions, offices, laws, political power and enforcement;
- health, wounds, combat outcomes and deaths;
- magic capabilities and their economic/social/political consequences;
- simulation time, schedules, history and persistent causal state.

Godot may keep presentation caches and interpolation state. Those values are replicas or predictions, not world truth.

### Player is a control relationship, not a special domain species

There is no privileged `Player` entity class in the Simulation Core.

A simulated actor has a stable `EntityId` and domain state. Control is attached outside that state:

```text
human devices -> Godot PlayerControls -> protocol/session binding -> actor intent
NPC decision system -------------------------------------------> actor intent
                                                                  |
                                                                  v
                                                         same world rules
```

The human-controlled actor may receive lower-latency presentation treatment, but it does not receive privileged economic, social, political, inventory or combat rules.

A capability that exists for an NPC should be available to the player through the same domain operation when the player satisfies the same prerequisites. Differences come from information, resources, skills, rights, context and control source—not from `if is_player` branches in world law.

### Stable identity crosses the presentation boundary

Every materializable world entity uses a stable `EntityId` assigned by the simulation/content initialization path.

Godot keys presentation nodes by that identity. Scene-node lifetime does not define world lifetime:

```text
Simulation entity exists
    |
    +-- relevant to current presentation -> Godot node materialized
    |
    +-- not relevant ---------------------> no Godot node
```

Destroying a Godot representation because an entity left the observation/materialization set does not delete the simulated entity.

Likewise, spawning a visual scene is not permission to create an authoritative person, item or shop. Authoritative creation must happen in the simulation first.

### Commands express intent; projections express readable results

The boundary follows a small command/read-model separation without adopting a distributed enterprise architecture:

```text
Godot input / UI action
    -> protocol Command or Intent
    -> validation + authoritative world transition
    -> CommandResult + DomainEvents
    -> read-only Projections
    -> Godot presentation
```

Commands describe tasks, not desired state.

Good examples:

- `BuyItem(buyer, seller, stock_entry, quantity)`
- `OfferTrade(actor, counterparty, terms)`
- `Attack(actor, target, method)`
- `OpenShop(actor, location, inventory)`
- `Travel(actor, destination)`
- `ApplyForOffice(actor, office)`

Bad examples:

- `SetMoney(500)`
- `SetRelationship(+20)`
- `SetMerchantInventory(...)`
- `SetNpcPosition(...)`
- `MarkBanditAttackResolved(true)`

Godot never receives a mutable `WorldState` object.

### Protocol projections are purpose-built read models

Godot consumes read-only projections shaped for presentation tasks rather than internal domain structures.

Examples of separate projections that may exist as mechanics are implemented:

- observed/local-world projection for materialized entities;
- controlled-actor projection;
- merchant/shop projection;
- inventory projection;
- relationship/social projection;
- institution/politics projection;
- journal/history projection.

A projection exposes only data that the presentation is allowed to know and needs to render the feature. Internal simulation objects are not serialized wholesale.

A shop projection can tell the UI that a merchant currently offers five apples for a concrete price. The purchase command still revalidates stock, money, access and any other authoritative rule at execution time. The UI cannot transfer the apple itself.

### Domain events explain change; they are not the source of truth

The simulation emits domain events for facts that occurred, such as:

- `ItemTransferred`;
- `TradeCompleted`;
- `ActorArrived`;
- `AttackStarted`;
- `ActorWounded`;
- `ActorDied`;
- `RelationshipChanged`;
- `OfficeChanged`;
- `LawViolated`;
- `MagicEffectApplied`.

Events are useful for animation/audio/UI feedback, causal explanation, tests and projection updates.

Durable state remains the authoritative world model/snapshot. This decision does **not** adopt full event sourcing. Event replay may be introduced only if a concrete persistence/debugging problem justifies it.

### Simulation time and presentation ordering are different concepts

`SimulationTick` means world time progression.

`WorldRevision` means monotonic authoritative state revision/order.

A player action can change the world without claiming that an hour/minute/tick of world time elapsed. Conversely, one simulation tick may produce many world changes.

Protocol projections carry enough revision/tick information for Godot to reject stale samples and present events in a sensible order.

### Location is semantic state, not a Godot transform

The final location model must distinguish at least the concepts that gameplay requires. A world item may be:

- spatially present at a world/place position;
- contained in a backpack/chest/shop stock;
- carried/equipped by an actor;
- in transit;
- destroyed/consumed.

Therefore a universal Godot `Transform3D` is not the domain location model.

For spatial actors/items, the simulation owns authoritative position/motion in a deterministic representation appropriate to the final world model. Godot converts the resulting projection into `Transform3D` for rendering.

The current `GridPosition`/cardinal move remains only a Milestone 0 transport probe and must not become the production third-person location model.

### Observed world and materialized world are bounded views

Godot must not receive and instantiate the entire simulated world every frame.

The simulation/protocol produces an observed/materialization projection for the controlled actor/context. It contains only entities whose state is currently relevant and permitted for presentation.

This is conceptually an interest set:

```text
full authoritative world
    -> observation/materialization policy
        -> bounded projection
            -> Godot presentation replica
```

The simulation owns information/visibility rules. Godot may additionally perform purely visual frustum/occlusion culling, but visual culling cannot reveal hidden simulation facts or decide whether an entity exists.

A presentation radius is not the same thing as NPC knowledge. The simulation may know an entity is physically nearby while the player character does not know its identity or hidden properties. Projections must respect those distinctions.

### Offscreen simulation continues without Godot nodes

Leaving a settlement removes its visual representation as appropriate. It does not pause or delete its simulation.

When the player returns, Godot materializes the current projection:

- a merchant may have sold stock;
- prices may have changed;
- an NPC may have moved, changed work, married, been injured or died;
- an institution may have changed officeholders;
- a bandit attack may have happened and altered local state.

The implementation may later use measured, simulation-internal fidelity strategies to reduce cost for distant areas. Any such optimization must preserve the same authoritative semantics and must not delegate world decisions to Godot.

No regional LOD framework is introduced by this ADR.

### Godot maintains a presentation replica, not a second world

For materialized entities Godot owns rendering-oriented objects keyed by `EntityId`:

```text
EntityId -> presentation node / animation state / interpolation samples / effects
```

A presenter performs three kinds of lifecycle operation based on authoritative projections:

- materialize: create/reuse a scene representation when an entity enters the current projection;
- update: feed authoritative samples/state into an existing representation;
- dematerialize: remove/pool the scene representation when the entity leaves the projection.

That presenter does not own inventory, relationships, price calculation, combat rules or actor existence.

### Smoothness is a presentation concern built over authoritative samples

Authoritative simulation and rendered frames do not need the same frequency.

Godot should normally render movement from timestamped/revisioned simulation samples using interpolation. The presentation can preserve previous/current authoritative transforms and render between them.

For the locally controlled actor, visual prediction may be used when required for responsiveness. Prediction must obey these rules:

1. it consumes the same semantic input intent sent to the simulation;
2. it is clearly presentation-only state;
3. the next authoritative sample reconciles it;
4. it cannot grant inventory, hits, access, interaction success or other systemic outcomes;
5. prediction complexity is added only after measured playtest need.

The existing `CharacterBody3D` third-person motor is therefore a temporary presentation/prediction shell during migration. It is no longer the authoritative source of player world position.

### Godot collision is presentation support unless simulation explicitly consumes a deterministic equivalent

Camera collision, particles, animation collision helpers and other visual engine facilities remain Godot concerns.

Authoritative movement/reachability/collision that changes world outcomes must eventually be decided by the simulation's spatial model or by a deterministic authoritative service owned behind the Simulation boundary. Godot physics contacts cannot silently become world truth.

The exact production spatial/collision representation is deliberately not chosen in this ADR; it requires the actual world/terrain/navigation constraints and performance measurements.

## Worked example: merchant and apples

Suppose actor `A` is controlled by the player and actor `M` is an NPC merchant.

Simulation state says:

- `M` is at the market;
- `M` controls a shop/container;
- that stock contains 5 apples;
- the current offer price is 7 coins each;
- `A` is close enough and permitted to trade;
- `A` owns 20 coins.

Godot receives a shop projection and renders “Apple ×5 — 7”.

Player presses Buy:

```text
Godot
  -> BuyItem(A, M, apple_stock_entry, 1)
Simulation
  -> revalidate participants/location/stock/price/funds/access
  -> transfer 7 coins A -> M
  -> transfer 1 apple shop -> A inventory
  -> emit TradeCompleted + ItemTransferred (+ economic consequences)
  -> produce updated shop/inventory projections
Godot
  -> update UI/animation/audio from results
```

An NPC customer can submit the same transaction through its decision system. The economic law is not duplicated for the player.

## Worked example: bandit attack

A bandit group decides/receives an intent to attack travelers or a settlement. The simulation resolves movement, participants, threats, combat/social/legal consequences and resulting state.

If the controlled actor's materialization projection includes the event participants, Godot represents them with movement, combat animation, barks, VFX and UI. Player actions become commands/intents into the same simulation.

If the player is elsewhere, no combat scene needs to exist in Godot. The simulation still resolves the attack. Returning later materializes survivors, corpses/damage/state changes, changed stock, relationships, law responses or other consequences that actually resulted.

## Migration from the current bootstrap

The current executable code establishes the first pieces of this contract:

- `EntityId` is a stable domain identity;
- `World` owns actors by identity and has no `player_position_` field;
- protocol/session code binds human control to an actor outside the domain;
- `WorldRevision` is separated from `SimulationTick`;
- actor projections carry identity/tick/revision;
- the legacy cardinal-grid command remains only a GDExtension smoke probe.

Next migration work should introduce the real spatial/observation projection and make the third-person presenter consume authoritative movement samples. It should not add economy/combat shortcuts to Godot while that bridge is incomplete.

## Source basis

The architecture adapts established ideas without importing their infrastructure wholesale:

- command/query separation and purpose-built read models from Microsoft's CQRS architecture guidance;
- adapter/anti-corruption boundaries from Microsoft's Anti-Corruption Layer guidance;
- interest management as a way to filter large simulated worlds to relevant presentation subsets, as surveyed in distributed virtual-environment research;
- agent-based modeling's bottom-up treatment of heterogeneous autonomous entities and emergent interactions;
- Godot 4.7 interpolation guidance, including the note that externally timed samples can require custom interpolation.

Exact sources are recorded in [`../engineering/SOURCES.md`](../engineering/SOURCES.md).

## Consequences

Positive:

- player and NPC systemic capabilities naturally share world rules;
- leaving an area no longer implies pausing or deleting its world state;
- Godot can be optimized/replaced without migrating the authoritative world;
- UI/HUD can use stable presentation projections instead of reading domain internals;
- future economy/social/politics/magic systems can interact through one causal model;
- visual smoothness can evolve independently from simulation tick rate.

Costs:

- the protocol must define real read models instead of exposing internal objects;
- authoritative spatial movement is harder than a local `CharacterBody3D` motor and needs a deliberate later implementation;
- presentation must handle materialize/update/dematerialize and stale/out-of-order samples;
- some immediate local feedback may require prediction/reconciliation once measured playtests justify it.

## Rejected alternatives

### Godot owns nearby truth; simulation owns distant truth

Rejected because crossing the visibility boundary would transfer authority between two rule systems and create divergent outcomes.

### Player is a separate world class with privileged actions

Rejected because it prevents true NPC/player parity and encourages duplicate economic/social/combat paths.

### Mirror the entire Simulation `WorldState` into Godot

Rejected because it leaks hidden/internal state, couples presentation to domain storage and creates a second mutable world cache.

### Full event sourcing now

Rejected because domain events are useful without making the event log the persistence model. Existing snapshots and deterministic simulation are sufficient until a measured need appears.

### Instantiate every world entity in Godot

Rejected because world existence and visual materialization are different concerns; large persistent worlds require bounded presentation sets.

### Build simulation LOD/sharding/distributed execution now

Rejected because scale optimizations require profiling. The architecture keeps those options open without committing infrastructure before evidence.
