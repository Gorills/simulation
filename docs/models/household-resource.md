# Model: Household Resource

Status: DRAFT

## Gameplay purpose

Milestone 2 begins the first authoritative household resource loop with one staple: unmilled grain represented in integer grain-units. The current implemented subset proves that a household can hold real stock, consume it through an actor-generic World rule, become short through bounded NPC autonomy, expose that state through a purpose-built protocol read, and render localized feedback in the real Godot client without a player-authored economic mutation.

This model is deliberately narrower than a general inventory, economy, market, farming or needs framework.

## Observable patterns / fit-for-purpose criteria

For the current bounded Core/application/vertical subset:

- household grain stock is authoritative `World` state;
- shortage is derived as `stock < shortage_threshold` and is never stored separately;
- an actor can consume only for the household they actually belong to;
- Consume requires exact-spatial presence inside that household's store place;
- one accepted Consume removes the positive content-owned consume amount, decrements the finite consume budget, advances `WorldRevision` exactly once and does not advance `SimulationTick`;
- zero budget, insufficient stock, missing membership/spatial presence, or being outside the store are refusals and leave the world unchanged;
- snapshot/restore preserves the quantities that determine later Consume outcomes and deterministic continuation;
- the acceptance village is created by one bounded Core content builder rather than by named protocol actor fields;
- protocol decision collection iterates the deterministic Core actor-id view, applies RestNeed movement to actors that have that state, and gives other exact-spatial non-controlled actors an idle locomotion intent;
- bounded autonomous Consume is considered only for non-controlled actors whose current authoritative state makes the action feasible;
- locomotion commits first; a successful autonomous Consume may then create a later `WorldRevision` on the same `SimulationTick` while the returned movement batch retains the locomotion commit revision;
- the next locomotion batch remains consecutive in `SimulationTick` and strictly later in `WorldRevision`;
- actor observation remains actor-scoped, while a separate village household-resource read exposes household membership, store identity/footprint, stock, threshold and derived shortage;
- the GDExtension translates that existing protocol projection without recomputing shortage or owning resource state;
- Godot resolves the tracked acceptance actor, household and shared rest/store position from authoritative projections before scripted runtime advancement instead of carrying fixture EntityIds or coordinates;
- the Godot HUD renders the protocol-owned `adequate`/`shortage` status with authoritative stock/threshold quantities through localized RU/EN text;
- the bounded `shortage` playtest advances only ordinary locomotion, submits no player economic command, observes the autonomous adequate-to-shortage transition and proves movement revision `R` followed by resource revision `R+1` on the same tick.

The first vertical M2 checkpoint therefore exposes the same Core/protocol resource truth in the real client; it does not introduce a client-side balance, shortage calculation or scenario-only stock mutation.

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

Current application ordering:

```text
Core actor-id view
  -> collect controlled + NPC locomotion intents
  -> commit one locomotion tick/revision
  -> preserve that movement batch revision
  -> re-read bounded Consume feasibility on post-movement World
  -> optional actor-generic Consume
  -> same SimulationTick, later WorldRevision
```

Current first vertical read path:

```text
World household/resource truth
  -> VillageHouseholdResourceProjection
  -> GDExtension Dictionary translation
  -> Godot read-only household discovery
  -> localized HUD status + bounded screenshot/debug evidence
```

There is no hidden hunger timer, background economy clock, second time stream or Godot-owned stock.

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

In the acceptance village the short household store intentionally uses the same X/Z/tolerance values as the M1 RestNeed target. That is one content fact shared by two mechanics, not a hidden protocol/Godot coordinate contract. The Godot client verifies this relationship from the two projections before caching the scenario target.

### Actor

Consume requires an ordinary actor with exact `SpatialState`. Human and NPC decision sources use the same World rule when the same state and prerequisites hold.

The current application policy excludes the controlled actor from autonomous Consume. This is a decision-source policy only; it does not grant NPCs a different World mutation law.

## Acceptance composition

The current M2 acceptance village is code-defined Core content, not protocol-owned fixture state. It contains:

- one controlled actor in the surplus household;
- the existing M1 RestNeed actor in the household intended to become short;
- one additional surplus-household NPC with exact spatial state and an idle locomotion intent;
- one store place per household;
- one household aggregate per store.

The builder owns concrete acceptance IDs and returns only the controlled-actor session binding. Protocol discovers actors through `World::actor_ids()` and households through `World::household_ids()`; it does not keep a growing list of feature-named NPC IDs. Godot similarly discovers the living-need actor's household/store through projections rather than duplicating those acceptance IDs or coordinates.

This is deliberately not an ECS, entity registry, scenario DSL or data-driven content framework.

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

The application layer may propose autonomous Consume only after the ordinary locomotion transition and only when a read-only Core feasibility check says current authoritative state satisfies membership, exact store presence, remaining budget and stock. `World::consume_household_grain()` still revalidates those prerequisites before mutation.

The first vertical Godot shortage scenario submits only a zero controlled locomotion intent while the ordinary NPC decision path advances. It does not submit a resource/economic intent, set stock, set shortage or teleport the NPC.

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

For bounded autonomous application, one call to `protocol::Simulation::advance_locomotion_tick()` has explicit ordering:

1. collect current controlled/NPC locomotion intents from the Core actor-id view;
2. attempt the full locomotion batch;
3. if locomotion fails, no autonomous Consume is attempted;
4. if locomotion succeeds, preserve and return the locomotion tick/revision in the movement batch;
5. against the post-movement World, consider feasible non-controlled actor Consume actions in the same bounded actor order;
6. each accepted Consume is an ordinary revision-only World transition and therefore may make a later resource read report the same tick with a greater revision;
7. an ordinary Consume refusal cannot retroactively fail or rewrite the successful movement batch.

The current acceptance content permits at most one autonomous Consume across the bounded scenario because only the short-household NPC has a positive remaining budget of one. This proves the ordering contract without turning locomotion ticks into a recurring meal cadence; the generic application loop remains bounded by the finite actor set and each household's remaining budget.

This bounded Consume budget proves finite autonomous depletion without inventing a meal cadence or independent economy clock. Recurring consumption over world time requires later time-system admission rather than treating locomotion ticks as meals.

## Outputs / consequences

Current direct outputs are:

- updated authoritative household stock;
- updated remaining consume budget;
- derived shortage state;
- unchanged `SimulationTick` for the Consume transition;
- one later `WorldRevision` on successful Consume;
- a village-scoped household-resource protocol read with authoritative tick/revision/version context;
- actor-scoped observed-world output computed from the deterministic actor-id view;
- GDExtension translation of household identity/membership/store/resource/status fields;
- localized Godot household-resource feedback and bounded RU/EN shortage scenario evidence.

The movement batch and a resource read taken immediately afterward may therefore deliberately carry different revisions on the same tick. Presentation retains the movement revision for the controlled actor's spatial sample while a later observed-world refresh may reconcile its latest world revision to the post-Consume resource revision.

## Player-facing exposure

The purpose-built village household-resource projection is now translated through the GDExtension and read by the Godot client. The client resolves the living-need actor's household and store from authoritative discovery, renders the supplied `adequate`/`shortage` status with stock/threshold quantities in the localized diagnostics HUD, and never calculates or mutates the resource state itself.

The bounded `shortage` scenario proves the first real vertical loop: the tracked household starts adequate, the NPC reaches its Core-owned store through ordinary locomotion, application-level autonomous Consume makes the household short, the resource read reports the same tick at one later revision, and RU/EN Godot feedback renders that authoritative result without a player economic command.

This is the first M2 player-visible checkpoint. It is not yet the milestone's multiple-path player intervention acceptance: carry, Gift, Work/production and standing transfer remain later capabilities.

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
- no household internal allocation rule beyond the current Consume operation;
- one bounded code-defined acceptance village rather than a general content authoring system.

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
- [`../decisions/0010-household-resource-composition-and-application-order.md`](../decisions/0010-household-resource-composition-and-application-order.md) — admitted M2 composition owner, generic actor collection and same-tick resource ordering.

## Falsifiers

Revise this model if current gameplay requires any of the following and the existing state cannot express it without contradiction:

- a meaningful distinction between stored grain lots/items;
- consumption cadence as real world-time behavior rather than a bounded acceptance transition;
- household allocation rules that require member-specific claims or obligations;
- storage loss/processing that materially changes shortage;
- a concrete magical capability that removes a load-bearing scarcity, storage or access assumption;
- regional historical evidence showing that a newly load-bearing assumption is unsuitable for the chosen scenario.
