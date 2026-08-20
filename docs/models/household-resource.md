# Model: Household Resource

Status: DRAFT

## Gameplay purpose

Milestone 2 begins the first authoritative household resource loop with one staple: unmilled grain represented in integer grain-units. The current implemented subset proves that a household can hold real stock, consume it through an actor-generic World rule, and become short without a player-authored state mutation.

This model is deliberately narrower than a general inventory, economy, market, farming or needs framework.

## Observable patterns / fit-for-purpose criteria

For the current bounded Core subset:

- household grain stock is authoritative `World` state;
- shortage is derived as `stock < shortage_threshold` and is never stored separately;
- an actor can consume only for the household they actually belong to;
- Consume requires exact-spatial presence inside that household's store place;
- one accepted Consume removes the positive content-owned consume amount, decrements the finite consume budget, advances `WorldRevision` exactly once and does not advance `SimulationTick`;
- zero budget, insufficient stock, missing membership/spatial presence, or being outside the store are refusals and leave the world unchanged;
- snapshot/restore preserves the quantities that determine later Consume outcomes and deterministic continuation.

Player-visible projection and Godot feedback are intentionally not part of this native-only increment; the first vertical M2 checkpoint will expose this same authoritative state rather than introduce a client-side duplicate.

## Historical baseline and region

Acceptance context: England around 1200, an inland temperate agrarian village where household grain storage is causally relevant.

The integer stocks, thresholds, consume amounts and consume budgets are acceptance fixtures, not historical ration, yield, capacity, price or bushel claims. The broader non-magical basis is [`../research/high-medieval-baseline-c1200.md`](../research/high-medieval-baseline-c1200.md). Any precise regional rate introduced later requires narrower research.

## Causal model

Current authoritative chain:

```text
household membership
  + household store place
  + exact actor location
  + grain stock
  + positive consume amount
  + remaining consume budget
      -> Consume validation
      -> stock decreases
      -> consume budget decreases
      -> derived shortage may change
      -> WorldRevision advances
```

There is no hidden hunger timer, background economy clock or Godot-owned stock.

## Entities / state / scales

### Household

Current household state contains:

- stable `EntityId`;
- member actor IDs;
- referenced store-place `EntityId`;
- non-negative grain stock;
- non-negative shortage threshold;
- positive content-owned consume amount;
- remaining bounded consume budget.

An actor belongs to at most one household. Household membership lists are authoritative; any reverse membership lookup is derived runtime work.

### Place

The current resource place record contains a stable `EntityId`, local X/Z and non-negative per-axis tolerance. Resource occupancy uses exact arrival semantics: the actor's authoritative X/Z must fall within the stored tolerance on each axis. Unlike the M1 rest-body occupancy query, this check does not add actor body radius.

### Actor

Consume requires an ordinary actor with exact `SpatialState`. Human and NPC decision sources use the same World rule when the same state and prerequisites hold.

## Fidelity / representation level

Households are aggregate-resolved state because current gameplay needs shared stock and membership but not separately identified grain items. Actors remain identity-resolved. Grain is a scalar conserved stock for the current slice rather than individual item entities.

No automatic aggregation/disaggregation framework is introduced.

## Conservation and promotion-demotion invariants

For current Consume:

- stock never becomes negative;
- accepted Consume removes exactly the configured positive amount;
- remaining consume budget decreases by exactly one;
- refusal changes neither stock nor budget;
- shortage is recomputed from stock and threshold;
- no representation-level promotion/demotion exists yet.

Later transfer/work mechanics must preserve this stock rather than create parallel client or subsystem balances.

## Magic sensitivity surface

Not implemented in this increment. The model is sensitive to future magic that changes:

- grain creation/transformation and scarcity;
- preservation/spoilage;
- transport or containment;
- labor/productive capacity;
- access to stores or exact-spatial constraints.

A future magical capability must change concrete authoritative facts/rules and let shortage respond causally; it must not add a global `magic_multiplier` or parallel magical inventory.

## Implemented magic deviations

None.

## Inputs

Current semantic mutation input is the acting `EntityId` for Consume. The actor does not supply an amount, stock target, shortage flag or household balance. World derives the actor's household and uses the household's authoritative consume amount.

## Transitions / scheduling

Consume is an immediate authoritative transition, not a simulation-time schedule.

An accepted Consume:

1. validates actor identity and existence;
2. resolves authoritative household membership;
3. validates the household resource state;
4. requires exact actor spatial state inside the referenced store place;
5. requires remaining consume budget and sufficient stock;
6. subtracts the configured amount and decrements the budget;
7. advances `WorldRevision` exactly once while leaving `SimulationTick` unchanged.

This bounded Consume budget proves finite autonomous depletion without inventing a meal cadence or independent economy clock. Recurring consumption over world time requires later time-system admission rather than treating locomotion ticks as meals.

## Outputs / consequences

Current direct outputs are:

- updated authoritative household stock;
- updated remaining consume budget;
- derived shortage state;
- unchanged `SimulationTick`;
- one later `WorldRevision` on success.

## Player-facing exposure

None in the current native-only increment. A later vertical M2 checkpoint will expose purpose-built household resource reads and localized feedback. Godot must read those projections and must not calculate or mutate stock/shortage itself.

## Uncertainty

The current resource unit and acceptance quantities are intentionally abstract fixtures. They establish conservation, scarcity and action prerequisites but do not claim historical household capacity or consumption rates.

## Simplifications

Current simplifications include:

- one staple only;
- perfect storage with no spoilage/pests;
- no item quality or individual grain objects;
- bounded acceptance consume opportunities instead of a meal schedule;
- no processing from grain to food;
- no prices, currency, credit or market clearing;
- no household internal allocation rule beyond the current Consume operation.

## Deliberately not simulated

Not yet represented:

- actor carry/inventory;
- Draw, Deposit or Gift;
- work/production and field assignment;
- standing household transfer pledges;
- wages, rents, tithe, tenure or institutional allocation;
- transport logistics;
- independent economy time progression;
- hunger/fatigue/metabolism.

These remain later bounded capabilities and are not implied by the current state shape.

## Sources

- [`../research/high-medieval-baseline-c1200.md`](../research/high-medieval-baseline-c1200.md) — project historical baseline and modeling consequences for household/rural production.
- [`../MODELING.md`](../MODELING.md) — deterministic quantity, household/economy and historical-source policy.
- [`../decisions/0008-core-snapshot-restore.md`](../decisions/0008-core-snapshot-restore.md) — authoritative snapshot/restore obligations.
- [`../decisions/0009-simulation-authority-and-decision-sources.md`](../decisions/0009-simulation-authority-and-decision-sources.md) — actor parity, time/order admission and composition boundary.

## Falsifiers

Revise this model if current gameplay requires any of the following and the existing state cannot express it without contradiction:

- a meaningful distinction between stored grain lots/items;
- consumption cadence as real world-time behavior rather than a bounded acceptance transition;
- household allocation rules that require member-specific claims or obligations;
- storage loss/processing that materially changes shortage;
- a concrete magical capability that removes a load-bearing scarcity, storage or access assumption;
- regional historical evidence showing that a newly load-bearing assumption is unsuitable for the chosen scenario.
