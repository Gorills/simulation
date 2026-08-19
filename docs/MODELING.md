# Simulation and modeling policy

This document owns causal world-modeling policy. Runtime dependency ownership lives in [`ARCHITECTURE.md`](ARCHITECTURE.md); product goals live in [`PRODUCT.md`](PRODUCT.md); evidence requirements live in [`VERIFICATION.md`](VERIFICATION.md).

## Determinism contract

For supported scenarios, equal seed, initial state, content version, protocol version, command sequence and simulation-step sequence must produce the same authoritative result under the declared platform/toolchain contract.

Authoritative logic must not depend on `std::random_device`, hidden system RNG, wall-clock time, frame rate, thread scheduling, unspecified iteration order, address-based ordering, locale-dependent parsing or unspecified filesystem enumeration order.

Use one explicit seeded PRNG state as part of world/subsystem state. Do not construct hidden RNGs inside NPCs or systems.

`std::unordered_map` is acceptable for lookup, but an authoritative outcome must not depend on its iteration order.

Load-bearing quantities such as money, item counts, simulation time, thresholds and charges should use integers/scaled integers/fixed-point by default. A floating-point value that changes an authoritative branch requires an explicit rationale and determinism tests.

Stable simulated identity uses explicit IDs such as `EntityId`; never derive identity/order from pointers, Godot instance IDs or scene paths.

The initial Simulation Core is single-threaded. Parallel execution must never be introduced merely as architecture preparation.

## Simulation time and state revision

Simulation time is an integer domain value such as `SimulationTick`.

Godot frame delta is presentation time, not world time. The world advances only through explicit simulation advancement. Pause, faster simulation and headless long steps are allowed, but load must not create a hidden extra tick.

State ordering is a different concern. `WorldRevision` is a monotonic revision of authoritative state and may advance when a command changes the world without advancing simulation time.

Do not use one counter to mean both “time elapsed” and “number/order of mutations”. Schedules, production and long-horizon systems depend on real simulation time; presentation reconciliation depends on ordered authoritative revisions.

## Entity identity and player/NPC parity

The player-controlled person is not a separate domain species.

All simulated people/actors use the same identity and world-rule model. Human control is a binding that produces intents for one actor; NPC decision logic produces intents for other actors.

```text
human control -> actor intent --+
                             |-> same authoritative rules
NPC decision -> actor intent --+
```

Do not implement systemic laws as `if is_player`. A player and NPC with the same relevant state/rights/resources must use the same transaction, ownership, combat, social, institution and world-action semantics.

Player-specific code is appropriate only for client/session/presentation concerns such as input mapping, camera, HUD, local prediction and which actor the session controls.

## Location is semantic world state

A Godot node transform is not the general location model.

World entities may have different location forms depending on the mechanic. For example an item may be spatially placed, contained in inventory/storage, equipped/carried, in transit or consumed/destroyed.

Spatial positions used for authoritative reachability/movement must belong to the Simulation model in a deterministic representation chosen for the actual world/terrain/navigation requirements.

The current `GridPosition` exists only for the Milestone 0 transport probe. It is not evidence that the final world is tile/grid based.

Do not let `CharacterBody3D.global_transform` silently decide an authoritative trade range, attack range, ownership boundary or semantic place.

## Protocol semantics

The application boundary expresses intent, not desired state:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents
  -> read-only Projections
```

Good inputs: `OfferTrade`, `GiveGift`, `AskForApprenticeship`, `ApplyForOffice`, `BuyItem`, `Attack`, `AdvanceTime`.

Bad inputs: `SetMoney(500)`, `SetRelationship(+10)`, `SetJob("blacksmith")`, `SetNpcPosition(...)`, `SetInventory(...)`.

The protocol is a small application contract, not a serialized copy of internal `WorldState`. Breaking protocol changes update an explicit protocol version and native/client-facing verification together.

### Purpose-built projections

Queries/projections are read models optimized for what a presentation feature needs. They contain no mutation logic.

Examples may include observed-local-world, shop, inventory, relationship, institution and journal projections as those capabilities are implemented.

Do not create one giant public `WorldProjection` that leaks every internal person, secret relationship, inventory and subsystem field to Godot.

Projections must respect information boundaries. Rendering a nearby merchant does not imply the client may read every fact the merchant knows.

Commands revalidate authoritative state at execution time. A UI showing “5 apples at 7 coins” cannot assume the stock/price is still valid when `BuyItem` executes.

## Observation and presentation materialization

The authoritative world can contain far more entities than the current Godot scene should instantiate.

The Simulation/protocol boundary produces a bounded presentation/observation set for the controlled context. Godot materializes scene representations for that set and can dematerialize them later without changing world existence.

Keep distinct:

- entity existence;
- semantic/spatial location;
- actor knowledge/visibility;
- presentation materialization;
- visual frustum/occlusion.

Godot may perform visual culling after it receives allowed presentation data. It does not decide which hidden authoritative facts become observable.

Leaving a settlement does not pause or delete it. Simulation continues; returning reveals current consequences through new projections.

Presentation interest filtering is part of the boundary contract, not permission to introduce unmeasured simulation LOD. Internal simulation fidelity optimizations require profiling evidence.

## NPC bounded rationality

NPCs act on information available to them, not hidden global truth.

Knowledge may come from observation, memory, conversation, rumor, letters, official announcements, markets, organizations or explicit magical information channels.

Only model state that changes current behavior. A minimal useful NPC may include identity, household, semantic location, carried/owned resources, relevant needs/health, active intention/task, relevant skills, relationships/obligations, known facts and institutional membership/status.

Do not design a complete personality ontology before gameplay requires it.

### Action model

```text
need / obligation / opportunity / threat / goal
  -> perceived options
  -> feasible option evaluation
  -> intention
  -> required place/resources/people
  -> travel/access
  -> process over simulation time
  -> consume/produce/transfer
  -> events/consequences
  -> new world state
```

Schedules can constrain opportunity; they must not replace causality with hidden scripts such as `08:00 -> FarmerWork()`.

Player-issued intents join the same world transition layer at the “intention/action” boundary; they do not bypass feasibility or consequence rules.

## Households

Households are useful social/economic aggregates, not universal entities.

A household may own or share housing rights, stores, tools, land access, obligations, debts and internal access rules. Do not model genetics or family detail until a concrete mechanic depends on it.

## Economy

Economic behavior must preserve physical and institutional causality. Distinguish resources, labor/time, tools and skills, access rights, ownership/control, transport, storage/loss, obligations/extraction and exchange when exchange actually occurs.

A useful early chain is:

```text
field access
  -> labor
  -> grain
  -> storage
  -> food processing
  -> household consumption
  -> shortage/work-capacity consequence
  -> player intervention
```

Trade is a concrete transaction between real participants or at a real market place with actual stock/resources, access/transport constraints and transaction history. Do not turn trade into a magical global price API.

Keep sale, barter, gift, debt, rent, tax/tribute, household sharing, wages, labor obligations and institutional allocation distinct when the distinction changes rules.

A human-controlled buyer/seller uses the same transaction law as an NPC buyer/seller. Godot renders offers/results but does not transfer stock or money.

## Politics and institutions

Politics emerges from real rights, restrictions, offices, resources of power, appointment/removal mechanisms, punishment, obligations, coalitions/support and information limits.

A minimal playable institution should make it possible to understand who can decide, why that actor has authority, who gains or loses, and how the decision can be changed, challenged or bypassed.

Do not build a state simulator before one local institution has real gameplay consequences.

## Social model

Do not reduce all relationships to one universal `-100..100` number.

Introduce dimensions only when they alter future action or cost, for example trust, obligation/debt, familiarity, group reputation, hostility/fear or authority/standing.

A social event that cannot change future opportunity, behavior or cost is decorative data.

Relationships are Simulation state. Godot may display a relationship projection or reaction animation; it must not calculate or persist the authoritative relationship value.

## Violence and visible/offscreen events

Violence is a world event, not a scene script.

Attack intent, participants, hostility/legal context, damage/death, loot and downstream social/institutional effects belong to Simulation.

When an attack is inside the current materialization projection, Godot represents it with movement/animation/VFX/audio and sends player intervention back as intent. When it is offscreen, Simulation resolves it without scene nodes.

Returning to the area materializes resulting survivors, injuries, deaths, damage, changed stock/ownership, law response or other state that actually occurred.

## Historical baseline

History defines the non-magical baseline. For a reference scenario record region, period, settlement type, climate/geography, political/institutional assumptions, trade context and technology baseline.

Do not freely mix practices from different centuries and regions. Load-bearing historical assumptions require sources.

Preferred evidence:

1. academic monographs;
2. peer-reviewed research;
3. critical editions / specialist datasets;
4. university or museum scholarly material;
5. primary sources with context.

Research is sufficient when gameplay has a causal baseline, plausible range, uncertainty, deliberate simplification and understood consequences of that simplification.

## Magic as an explicit counterfactual

Magic changes a stated world law; it is not an explanation for gaps.

For each serious magic capability define capability/access, acquisition, cost, range/duration, reliability/failure modes, observability, countermeasures, institutional control, and economic/political/social/long-term consequences.

If magic removes a load-bearing constraint, downstream assumptions must be reconsidered. Teleportation affects transport/trade/borders; healing affects mortality/labor/care; weather control affects agriculture/storage/power; prophecy affects information/crime/markets; food creation affects scarcity and land/labor value.

Godot displays magical effects/results. The authoritative changes to crops, prices, health, institutions, knowledge or other world state occur in Simulation.

## Model contracts

Create `docs/models/<mechanic>.md` for a serious mechanic whose causal model needs durable review.

```markdown
# Model: <name>

Status: DRAFT | ACCEPTED | REVISE

## Gameplay purpose
## Causal model
## Historical baseline
## Magic deviations
## Inputs
## State
## Transitions
## Outputs / consequences
## Player-facing exposure
## Uncertainty
## Simplifications
## Deliberately not simulated
## Sources
## Falsifiers
```

Do not create a model document for every function.

## Content and laws

Rules define what can happen. Content defines the concrete people, places, households, items, institutions, occupations, recipes, magic capabilities and scenario initial conditions.

Authoritative entities are created from Simulation/content processes, not because a Godot scene requests an NPC/item to exist.

Do not hardcode a specific NPC inside a general domain algorithm. Do not design a universal data DSL at the start; use simple typed definitions with validation.

## Save/load

Authoritative snapshots are versioned from the first persisted format and contain authoritative world state, stable entity identities, seed, simulation time and content/protocol/schema version identifiers.

Do not persist camera, animation frame, interpolation samples, hover state or open menu as world truth.

Load must not advance the world. Given the same loaded snapshot and subsequent commands, results must obey the determinism contract.

Do not implement a migration framework for versions that do not yet exist.

## Domain events

Events support player feedback, causal explanation, debugging, projections and tests. Examples include arrivals, work completion, item transfer, trade completion, debt creation, obligation fulfillment, office changes, law violation, attacks, wounds/deaths, magic outcomes and household shortage changes.

Events describe facts that occurred. Durable current state remains in the authoritative world/snapshot.

Do not adopt full event sourcing unless a real problem requires it.

## Performance policy

Do not optimize imagined scale.

Measure simulation-step latency, NPC decisions, allocations, projection generation/serialization, observation-set size, GDExtension round-trip, save/load time, render frame time and materialized actors before introducing scale architecture.

Do not introduce ECS frameworks, multithread job systems, regional simulation LOD, distributed simulation, sharding or networking architecture without measured evidence.

Bounded Godot materialization is not simulation LOD: it avoids rendering the entire world while Simulation remains authoritative for offscreen entities.

## Long-horizon simulation

Long-horizon tests are justified for systems with feedback loops such as demographics, resource depletion, debt, wealth concentration, production/market feedback, political concentration, magic accumulation or institutional change.

A long run must explain trajectory through events/metrics. “It did not crash for 100 years” is not sufficient evidence.

## Anti-overmodeling gate

Before adding a subsystem answer:

1. What will the player see or be able to do?
2. Which current observable result is wrong without it?
3. Can an F1 model solve the problem with a fraction of the complexity?
4. How can it be verified in the current playable build?
5. Which data are deliberately not modeled?
6. Does it create a second source of truth?
7. Does it require abstractions with no current second use case?
8. Does the player use the same domain capability as an equivalent NPC?
9. What projection exposes the result without leaking the whole world model?

Weak answers mean the subsystem should not be added yet.
