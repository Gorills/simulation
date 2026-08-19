# Simulation and modeling policy

This document owns causal world-modeling policy. Runtime dependency ownership lives in [`ARCHITECTURE.md`](ARCHITECTURE.md); product goals live in [`PRODUCT.md`](PRODUCT.md); evidence requirements live in [`VERIFICATION.md`](VERIFICATION.md).

## Determinism contract

For supported scenarios, equal seed, initial state, content version, protocol version, command sequence and simulation-step sequence must produce the same authoritative result under the declared platform/toolchain contract.

Authoritative logic must not depend on `std::random_device`, hidden system RNG, wall-clock time, frame rate, thread scheduling, unspecified iteration order, address-based ordering, locale-dependent parsing or unspecified filesystem enumeration order.

Use one explicit seeded PRNG state as part of world/subsystem state. Do not construct hidden RNGs inside NPCs or systems.

`std::unordered_map` is acceptable for lookup, but an authoritative outcome must not depend on its iteration order.

Load-bearing quantities such as money, item counts, simulation time, thresholds and charges should use integers/scaled integers/fixed-point by default. A floating-point value that changes an authoritative branch requires an explicit rationale and determinism tests.

The initial Simulation Core is single-threaded. Parallel execution must never be introduced merely as architecture preparation.

## Simulation time

Simulation time is an integer domain value such as `SimulationTick`.

Godot frame delta is presentation time, not world time. The world advances only through explicit simulation advancement. Pause, faster simulation and headless long steps are allowed, but load must not create a hidden extra tick.

## Protocol semantics

The application boundary expresses intent, not desired state:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

Good inputs: `OfferTrade`, `GiveGift`, `AskForApprenticeship`, `ApplyForOffice`, `AdvanceTime`.

Bad inputs: `SetMoney(500)`, `SetRelationship(+10)`, `SetJob("blacksmith")`.

The protocol is a small application contract, not a serialized copy of internal `WorldState`. Breaking protocol changes update an explicit protocol version and native/client-facing verification together.

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

## Politics and institutions

Politics emerges from real rights, restrictions, offices, resources of power, appointment/removal mechanisms, punishment, obligations, coalitions/support and information limits.

A minimal playable institution should make it possible to understand who can decide, why that actor has authority, who gains or loses, and how the decision can be changed, challenged or bypassed.

Do not build a state simulator before one local institution has real gameplay consequences.

## Social model

Do not reduce all relationships to one universal `-100..100` number.

Introduce dimensions only when they alter future action or cost, for example trust, obligation/debt, familiarity, group reputation, hostility/fear or authority/standing.

A social event that cannot change future opportunity, behavior or cost is decorative data.

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

Do not hardcode a specific NPC inside a general domain algorithm. Do not design a universal data DSL at the start; use simple typed definitions with validation.

## Save/load

Authoritative snapshots are versioned from the first persisted format and contain authoritative world state, seed, simulation time and content/protocol/schema version identifiers.

Do not persist camera, animation frame, hover state or open menu as world truth.

Load must not advance the world. Given the same loaded snapshot and subsequent commands, results must obey the determinism contract.

Do not implement a migration framework for versions that do not yet exist.

## Domain events

Events support player feedback, causal explanation, debugging, projections and tests. Examples include arrivals, work completion, item transfer, trade completion, debt creation, obligation fulfillment, office changes, law violation, magic outcomes and household shortage changes.

Do not adopt full event sourcing unless a real problem requires it.

## Performance policy

Do not optimize imagined scale.

Measure simulation-step latency, NPC decisions, allocations, projection serialization, GDExtension round-trip, save/load time, render frame time and active actors before introducing scale architecture.

Do not introduce ECS frameworks, multithread job systems, regional LOD, distributed simulation, sharding or networking architecture without measured evidence.

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

Weak answers mean the subsystem should not be added yet.
