# Simulation and modeling policy

This document owns causal world-modeling policy. Runtime dependency ownership lives in [`ARCHITECTURE.md`](ARCHITECTURE.md); product goals live in [`PRODUCT.md`](PRODUCT.md); evidence requirements live in [`VERIFICATION.md`](VERIFICATION.md).

Historical/counterfactual foundation:

- [`research/high-medieval-baseline-c1200.md`](research/high-medieval-baseline-c1200.md) — sourced non-magical baseline;
- [`decisions/0005-historical-counterfactual-and-causal-fidelity.md`](decisions/0005-historical-counterfactual-and-causal-fidelity.md) — accepted modeling decision.

## Modeling objective

Model the **smallest causal world that produces believable, explainable player-facing consequences**.

Realism is not the number of variables or the number of individually ticked NPCs. A model is useful when it preserves the constraints, incentives, flows, institutions, information limits and history needed to make observable opportunities and outcomes plausible.

The default non-magical research horizon is Latin/Western-Central Europe around **1180–1230**. It is a comparative starting point, not a universal medieval template and not a chronology the magical world must reproduce. A concrete mechanic must narrow region/settlement/institutional assumptions when regional differences affect its rules.

## Determinism contract

For supported scenarios, equal seed, initial state, content version, protocol version, command sequence and simulation-step sequence must produce the same authoritative result under the declared platform/toolchain contract.

Authoritative logic must not depend on hidden system RNG, wall-clock time, frame rate, thread scheduling, unspecified iteration order, address ordering, locale-dependent parsing or unspecified filesystem enumeration order.

Use explicit seeded PRNG state. Do not construct hidden RNGs inside NPCs or systems.

Load-bearing quantities such as money, item counts, simulation time, thresholds and charges should use integers/scaled integers/fixed-point by default. A floating-point value that changes an authoritative branch requires an explicit rationale and determinism evidence.

Stable simulated identity uses explicit IDs such as `EntityId`; never derive identity/order from pointers, Godot instance IDs or scene paths.

The initial Simulation Core is single-threaded. Parallel execution is not architecture preparation.

## Simulation time and state revision

`SimulationTick` is authoritative world-time progression. Godot frame/physics delta is presentation time, not world time.

`WorldRevision` is authoritative state ordering and may advance when an immediate action changes state without advancing time.

Do not use one counter for both concepts. Schedules, production, recovery and long-horizon systems depend on simulation time; presentation reconciliation depends on ordered revisions.

## Entity identity and player/NPC parity

The player-controlled person is not a separate domain species.

```text
human control -> actor intent --+
                             |-> same authoritative rules
NPC decision -> actor intent --+
```

A player and NPC with the same relevant state, rights, information and resources use the same transaction, ownership, combat, social, institutional and world-action semantics.

Player-specific code belongs only to session/presentation concerns such as input, camera, HUD, local prediction and which actor is controlled.

## Social position and roles are compositional

Do not make `Peasant`, `Merchant`, `Noble`, `Priest`, `Bandit`, `Mage` or another role enum the authoritative source of permissions or behavior.

Around the historical baseline, social position was not one universal rank. Model only the concrete dimensions a current rule needs, such as:

- property/control and material resources;
- land/territory/access rights;
- skills and knowledge;
- legal freedom/status where relevant;
- household/kin/patronage ties;
- debts and obligations;
- organization/institution membership;
- office and jurisdiction;
- reputation, trust, hostility, fear or standing where those alter behavior;
- coercive capability;
- magical capability/access;
- persistent history.

A UI may call an actor “merchant” or “bandit”. That label is a projection from state/history, not the source of world law.

Social mobility is therefore possible through real changes in those dimensions. Resistance to mobility also comes from real barriers — rights, property, patronage, knowledge, enforcement, coalitions, reputation and force — not a hardcoded caste ceiling.

## Location is semantic world state

A Godot transform is not the general location model.

An item may be spatially placed, contained, carried/equipped, stored, in transit, consumed or destroyed. An actor may have a semantic place and, when required, an authoritative spatial state.

Spatial reachability/movement must belong to Simulation in a deterministic representation chosen for actual terrain/navigation requirements. `GridPosition` remains a Milestone 0 transport probe only.

Do not let `CharacterBody3D.global_transform` decide authoritative trade range, attack range, ownership boundary or semantic place.

## Protocol semantics

The application boundary expresses intent, not desired state:

```text
Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents
  -> read-only Projections
```

Good: `OfferTrade`, `GiveGift`, `ApplyForOffice`, `BuyItem`, `Attack`, `AdvanceTime`.

Bad: `SetMoney`, `SetRelationship`, `SetJob`, `SetNpcPosition`, `SetInventory`.

Purpose-built projections expose only information needed and permitted for a presentation task. Do not create a public mutable `WorldState` or one giant projection containing every hidden relationship, inventory and subsystem field.

Commands revalidate current authoritative state. A UI displaying an offer does not guarantee that the transaction is still possible when submitted.

## Observation and presentation materialization

The authoritative world can contain far more entities than Godot materializes.

Keep distinct:

- authoritative existence;
- semantic/spatial location;
- actor knowledge/visibility;
- presentation observation/materialization;
- camera/frustum/occlusion.

Leaving a settlement does not pause or delete it. Godot node absence is not Simulation absence.

Presentation interest filtering is separate from **modeling resolution**. Godot can render a bounded subset while an offscreen entity remains causally important and identity-resolved.

## Adaptive causal fidelity

Simulation detail follows **causal relevance**, not camera distance and not a desire to model every real-world detail.

Choose the cheapest representation that preserves the facts needed by current gameplay, already-observed history and downstream causal chains.

Three conceptual resolution levels are useful:

1. **Identity-resolved** — persistent individual actors/items/institutions whose identity, choices, relationships or history matter.
2. **Aggregate-resolved** — stocks, populations, distributions or flows where collective state matters but current gameplay does not require individual identity.
3. **Deferred/derived** — details not stored/ticked because no current mechanic depends on them.

These are modeling choices, **not mandatory runtime base classes**. Do not create a generic fidelity manager, dynamic aggregation framework or regional LOD system merely because the vocabulary exists.

### When identity resolution is justified

Prefer identity-resolved state when one or more are true:

- the player can interact with the entity now;
- persistent identity/relationship/debt/ownership/history matters later;
- the entity participates in an unresolved causal chain that can reach gameplay;
- individual heterogeneity materially changes the outcome;
- an event/history must explain what happened;
- a current projection/mechanic exposes the individual.

Offscreen does not mean aggregate. A distant ruler, named debtor, owned caravan or dangerous enemy may remain identity-resolved.

Visible does not automatically mean lifelong micro-simulation. A crowd representation may be aggregate/deferred if no mechanic needs individual continuity.

### Promotion/demotion invariants

If a later mechanic changes representation resolution, preserve what the game can hold the world accountable for:

- conserved stocks/counts/resources;
- named or historically consequential identities;
- ownership, rights, debts and obligations;
- already-observed facts/events;
- distributions necessary for plausible later disaggregation;
- deterministic/replay guarantees.

Do not erase a merchant who owes the player money because the player left town. Do not later invent a healthy population that contradicts an aggregate famine/death trajectory.

### Modeling fidelity vs performance optimization

Choosing aggregate state because individual identity is **not causal** is a model decision and can be correct from the start.

Introducing runtime regional LOD, automatic aggregation/disaggregation, ECS, multithread jobs, sharding or distributed simulation is an implementation/scale decision and still requires a concrete mechanic plus measurement.

Do not confuse those two concerns.

## NPC bounded rationality

NPCs act on information available to them, not hidden global truth.

Knowledge may come from observation, memory, conversation, rumor, letters, markets, organizations, official announcements or explicit magical information channels.

Only model state that changes behavior. A useful actor may include identity, household, location, owned/carried resources, relevant needs/health, active intention, skills, relationships/obligations, known facts and institutional status. Do not design a complete personality ontology before gameplay requires it.

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

Schedules constrain opportunity; they do not replace causality with hidden scripts such as `08:00 -> FarmerWork()`.

Player-issued intents enter the same feasibility/consequence layer.

## Households

Households are useful social/economic aggregates when shared housing, stores, land access, tools, obligations, debts or access rules matter.

They are not an excuse to model genetics or every domestic detail before gameplay needs it.

## Economy

Economic behavior preserves physical and institutional causality. Distinguish only where rules require it:

- resources/stocks;
- labor/time and relevant skills;
- tools/capital;
- access and ownership/control;
- transport/storage/loss;
- obligations/extraction;
- exchange/credit.

A useful early chain is:

```text
field access -> labor -> grain -> storage -> food processing
             -> household consumption -> shortage/work consequence
             -> player/NPC intervention
```

Trade is a transaction between real participants or through a real institution with actual stock/resources and constraints. Do not turn trade into a magical global price API.

Sale, barter, gift, debt, rent, tax/tribute, household sharing, wages, labor obligations and institutional allocation stay distinct when that distinction changes outcomes.

## Politics and institutions

Do not model politics as one rank ladder.

Power emerges from concrete offices, jurisdiction, recognized rights/claims, material resources, coercive capacity, legitimacy, information, obligations, appointment/removal rules, coalitions and enforcement.

Kingship, lordship, ecclesiastical institutions, towns/communes and local communities may overlap rather than forming one modern unitary state.

A minimal institution should let the player understand who can decide, why their authority works, who benefits/loses, and how it can be influenced, challenged or bypassed.

Institutions react to disruptive actors — including magically powerful ones — through capabilities they actually possess. There is no institutional plot armor. If recruitment, law, patronage, coercion, legitimacy, alliances or countermeasures are insufficient, power can genuinely shift.

## Social model

Do not reduce all relationships to one `-100..100` value.

Introduce dimensions only when they alter future action/opportunity/cost, for example trust, obligation/debt, familiarity, group reputation, hostility/fear or authority/standing.

A social fact that can never affect future behavior is decorative data and normally should not be authoritative state.

## Violence and visible/offscreen events

Violence is a world event, not a scene script.

Attack intent, participants, legal/social context, wounds/death, loot and downstream consequences belong to Simulation.

If an attack is observed, Godot renders it and player intervention returns as intent. If it is offscreen, the causal event still resolves without scene nodes when it matters to the authoritative world.

## Historical baseline and source discipline

History defines the non-magical reference constraints. The project baseline is documented in [`research/high-medieval-baseline-c1200.md`](research/high-medieval-baseline-c1200.md).

Do not freely mix centuries or regions. For a serious mechanic record region, period, settlement type, geography/climate, institutional assumptions, trade context and technology assumptions at the precision the mechanic needs.

Preferred evidence:

1. academic monographs;
2. peer-reviewed research;
3. critical editions/specialist datasets;
4. university or museum scholarly material;
5. primary sources with appropriate context.

Research is sufficient when the model has a causal baseline, plausible uncertainty/range, deliberate simplifications and understood consequences of those simplifications.

Historical sources constrain the **non-magical baseline**. They do not force the magical world to return to real chronology after its causal conditions diverge.

## Magic participation contract

Magic is not a late subsystem and not an explanation for missing causality.

Every serious model must state its **magic sensitivity surface**: which mundane assumptions/constraints, if changed by a future magical capability, would alter this model.

Common review surfaces include:

- health, mortality, fertility, recovery and work capacity;
- resource production/transformation/scarcity;
- transport, travel time, distance and communication;
- information, secrecy, verification and prediction;
- coercion, defense, violence and security;
- property, containment, access and theft prevention;
- weather, water, soil and environmental hazards;
- skill, learning, memory and capability acquisition;
- legitimacy, religion and recognized authority;
- institutional enforcement/countermeasures.

This is a review checklist, **not a universal magic taxonomy**.

Do not add `magic_multiplier` to subsystems or a global effect bus. A concrete magical capability changes concrete authoritative facts/mechanisms; dependent systems respond to those changed facts.

Example:

```text
healing capability
  -> morbidity / mortality / recovery / work capacity
  -> household labor + care burden
  -> production / income / population pressures
  -> prices / taxes / military availability / legitimacy
  -> institutional and political response
```

When a magical capability removes a load-bearing constraint, downstream assumptions must be reconsidered and regression/long-horizon tests should cover the new trajectory.

For each implemented magic capability define access/acquisition, cost, range/duration, reliability/failure, observability, countermeasures, institutional response and downstream consequences.

Godot displays effects/results. Crops, health, prices, knowledge, relationships, authority and other causal state change in Simulation.

## Model contracts

Create `docs/models/<mechanic>.md` for a serious mechanic whose causal model needs durable review.

```markdown
# Model: <name>

Status: DRAFT | ACCEPTED | REVISE

## Gameplay purpose
## Observable patterns / fit-for-purpose criteria
## Historical baseline and region
## Causal model
## Entities / state / scales
## Fidelity / representation level
## Conservation and promotion-demotion invariants
## Magic sensitivity surface
## Implemented magic deviations
## Inputs
## Transitions / scheduling
## Outputs / consequences
## Player-facing exposure
## Uncertainty
## Simplifications
## Deliberately not simulated
## Sources
## Falsifiers
```

This structure intentionally borrows the useful discipline of ODD — purpose, entities/state/scales, processes/scheduling and evaluation — without requiring an ecology-simulation framework.

Do not create a model document for every function.

## Content and laws

Rules define what can happen. Content defines concrete people, places, households, items, institutions, recipes, occupations/capabilities and scenario initial conditions.

Authoritative entities are created through Simulation/content initialization, not because a Godot scene requests an NPC/item.

Do not hardcode one named NPC inside a general rule. Do not design a universal data DSL before repeated content demonstrates the need.

## Save/load

Authoritative snapshots are versioned and contain authoritative world state, stable identities, seed, simulation time and relevant content/protocol/schema versions.

Do not persist camera, animation frame, interpolation samples, hover state or open menu as world truth.

Load must not advance the world. Equal loaded state plus equal subsequent commands/steps must obey the determinism contract.

## Domain events

Events explain facts that occurred and support presentation, history, debugging, projections and tests.

Durable current state remains authoritative world state; do not adopt full event sourcing without a demonstrated need.

## Performance policy

Do not optimize imagined scale.

Measure simulation-step latency, NPC decisions, allocations, projection generation, observation-set size, GDExtension round-trip, save/load and rendered/materialized actors before introducing scale architecture.

Do not add ECS frameworks, automatic regional LOD, multithread job systems, distributed simulation, sharding or networking architecture without measured evidence.

Adaptive causal fidelity is **not** a performance loophole: it decides what the model needs to represent. Performance architecture decides how that model is executed.

## Long-horizon and counterfactual simulation

Long-horizon tests are justified for feedback systems such as shortage, debt, wealth concentration, demographic pressure, production/market feedback, political concentration, institutional adaptation or magic accumulation.

A long run must be explainable through events/metrics. “It did not crash for 100 years” is not evidence of a plausible world.

Useful counterfactual tests compare a sourced non-magical baseline with one changed capability/constraint and verify that downstream consequences arise through explicit mechanisms rather than hardcoded outcome targets.

## Anti-overmodeling gate

Before adding a subsystem answer:

1. What can the player see, understand or do because of it?
2. Which current observable opportunity/outcome is wrong without it?
3. What is the smallest causal model that produces the needed pattern?
4. Which entities must be identity-resolved, which can be aggregate-resolved, and which detail is deferred?
5. Which facts must survive any future resolution change?
6. What historical baseline/source constrains it?
7. What is its magic sensitivity surface?
8. Can equivalent NPC/player actors use the same world capability?
9. What projection exposes the result without leaking the whole world?
10. How is the causal transition/trajectory falsified or verified?
11. Does the proposed implementation introduce an abstraction with no current second use?

Weak answers mean the subsystem should not be added yet.
