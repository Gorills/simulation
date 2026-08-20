# ADR 0010: Household resource composition and application ordering

Status: Accepted  
Date: 2026-08-20

Related contracts: [`0009-simulation-authority-and-decision-sources.md`](0009-simulation-authority-and-decision-sources.md) · [`../models/household-resource.md`](../models/household-resource.md) · [`../ARCHITECTURE.md`](../ARCHITECTURE.md)

## Context

ADR 0009 deliberately allowed the first Living Need protocol scenario to hardcode one controlled actor and one NPC, but required composition/observation ownership to be re-admitted before a scenario needed dynamic population composition or a second independent NPC. Milestone 2 Household Resource Loop is that first concrete requirement: the acceptance village needs three actors, two households and two store places, while the existing RestNeed NPC must continue to use the same actor/world laws.

Milestone 2 also introduces an immediate resource mutation that can occur after a successful locomotion transition without advancing `SimulationTick`. The application boundary therefore needs an explicit order so a revision-only Consume cannot retroactively invalidate a locomotion batch or accidentally become a second time stream.

## Decision

### Core content owns acceptance-village composition

The first M2 village is built by one bounded code-defined Simulation Core content builder. Core owns entity existence, authoritative IDs, household membership, store records and resource fixture values. The builder returns only the controlled-actor binding needed by the application/session layer.

`protocol::Simulation` no longer owns a feature-named Living Need NPC field or constructs authoritative actors directly. This replacement satisfies ADR 0009's first composition admission gate without introducing a scenario DSL, ECS, generic entity registry or data-driven content framework.

### Protocol collection is actor-generic and bounded

`protocol::Simulation` obtains the deterministic `World::actor_ids()` view once per locomotion application call and uses it to collect actor intents:

- the controlled actor receives the current accepted human movement intent;
- each non-controlled actor with `RestNeedState` receives the existing deterministic RestNeed decision;
- each other exact-spatial non-controlled actor receives an idle locomotion intent so the ordinary movement/presentation path can materialize it;
- actors without exact spatial state do not acquire a fake locomotion sample merely because they exist.

The same bounded actor-id view is reused for the post-locomotion autonomy pass. No `WorldSnapshot` enumeration, full unbounded entity scan or second composition owner is introduced.

### Observed world remains actor-scoped

`ObservedWorldProjection` is computed per read from `World::actor_ids()`. It contains actors only because its current consumer materializes actor presentation shells. Households and places do not become observed actors.

Household/resource discovery uses a separate purpose-built village projection derived from `World::household_ids()`, household state and referenced store state. That projection exposes household identity, member actor IDs, store identity/footprint, stock, shortage threshold and derived shortage together with tick/revision/protocol context.

### Autonomous Consume is post-locomotion and revision-only

One `Simulation::advance_locomotion_tick()` application call has this ordering:

1. collect movement intents from the pre-transition authoritative World;
2. attempt the complete authoritative locomotion batch;
3. if locomotion fails, return the movement error and perform no autonomous resource mutation;
4. if locomotion succeeds, retain that locomotion commit's tick/revision in the returned movement batch;
5. inspect bounded non-controlled actors against the post-movement World;
6. suppress Consume proposals whose current authoritative state cannot satisfy membership, exact store presence, remaining consume budget or stock;
7. execute feasible Consume proposals only through the existing actor-generic `World::consume_household_grain()` rule, which revalidates authority;
8. an accepted Consume advances `WorldRevision` but not `SimulationTick`.

Therefore a household-resource read immediately after the call may report the same `SimulationTick` as the returned movement batch with a greater `WorldRevision`. The next locomotion batch remains consecutive in tick and strictly later in revision.

An ordinary Consume refusal can never rewrite or turn an already successful locomotion batch into failure.

### The acceptance budget bounds the application burst

The current acceptance content gives a positive Consume budget only to the short-household NPC and only for one successful Consume. This proves autonomous depletion and ordering without treating every locomotion tick as a meal cadence or introducing a scheduler/economy clock.

## Consequences

- M1 RestNeed movement remains actor-generic and keeps its existing causal semantics while the population grows to three actors.
- Protocol no longer scales through feature-named actor IDs.
- A third NPC can materialize through the ordinary observed/movement path with an idle authoritative sample.
- Household/resource discovery is separate from actor observation and does not force household/place entities into actor presentation.
- Immediate resource mutations can sit between locomotion revisions on the same tick without requiring a second time stream or movement sequence counter.
- Future slower time systems still remain subject to ADR 0009's separate time-ordering admission gate; this decision covers revision-only resource transitions only.
- The bounded linear collection strategy is appropriate for the current acceptance village. A production scaling requirement must justify a new index/scheduler rather than pre-installing one now.

## Deliberately not introduced

This decision does not introduce:

- a generic NPC brain, behavior tree, GOAP or scheduler;
- a generic entity/component system or scenario DSL;
- an independent economy tick or event queue;
- actor carry, Gift, Work, transfer pledges, markets or currency;
- GDExtension/Godot household-resource UI;
- persistent stored shortage flags;
- a second locomotion/result sequence counter.

## Verification obligations

The bounded implementation must prove:

- startup household discovery, observed actors and controlled spatial state share one unchanged revision;
- observed actors come from Core composition and include the third actor;
- the third actor receives an idle movement sample through the ordinary batch;
- existing RestNeed behavior remains semantically intact;
- the first autonomous Consume changes shortage only after locomotion succeeds;
- movement batch revision stays at the locomotion commit while the immediate household read sees the same tick at `revision + 1` for that Consume;
- the following locomotion batch has `tick + 1` and revision strictly greater than the resource mutation;
- exhausted/infeasible Consume state does not create repeated revision churn.
