# Model: Household Resource

Status: DRAFT

## Gameplay purpose

Milestone 2 begins the first authoritative household resource loop with one staple: unmilled grain represented in integer grain-units. The current implemented subset proves that a household can hold real stock, consume it through an actor-generic World rule, become short through bounded NPC autonomy, expose that state through the real client, and now move the same conserved grain between household stock and a bounded actor carry slot through native Draw, Deposit and Gift laws.

The first Godot checkpoint still exposes shortage only. Carry and transfer commands remain native until the next bounded player-facing increment.

This model is deliberately narrower than a general inventory, economy, market, farming or needs framework.

## Observable patterns / fit-for-purpose criteria

For the current bounded Core/application/vertical subset:

- household grain stock is authoritative `World` state;
- shortage is derived as `stock < shortage_threshold` and is never stored separately;
- actor carried grain and carry capacity are authoritative `ActorState` with `0 <= carried <= capacity`;
- an actor can consume only for the household they actually belong to;
- Consume requires exact-spatial presence inside that household's store place;
- one accepted Consume removes the positive content-owned consume amount, decrements the finite consume budget, advances `WorldRevision` exactly once and does not advance `SimulationTick`;
- zero budget, insufficient stock, missing membership/spatial presence, or being outside the store are Consume refusals and leave the world unchanged;
- Draw at the actor's own household store moves as much grain as possible toward the actor's carry capacity, limited by available household stock;
- Deposit at the actor's own household store moves the actor's entire carried grain back into household stock;
- Gift at a selected other household's store moves the actor's entire carried grain into that receiving household and refuses the actor's own household target;
- Draw, Deposit and Gift do not accept caller-authored quantities; their moved amount is derived from authoritative state;
- a full carry, empty source/carry, invalid location/identity/membership, own-household Gift target or checked-addition overflow is a typed refusal with no mutation;
- every accepted Draw, Deposit or Gift changes real grain state, advances `WorldRevision` exactly once and leaves `SimulationTick` unchanged;
- snapshot/restore preserves carry/capacity and the quantities that determine later Consume/Draw/Deposit/Gift outcomes and deterministic continuation;
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

The first vertical M2 checkpoint therefore exposes the same Core/protocol resource truth in the real client; native carry/transfer state extends that truth without introducing a client-side balance, generic inventory or scenario-only stock mutation.

## Historical baseline and region

Acceptance context: England around 1200, an inland temperate agrarian village where household grain storage is causally relevant.

The integer stocks, thresholds, consume amounts, consume budgets and carry capacities are acceptance fixtures, not historical ration, yield, carrying-capacity, price or bushel claims. The broader non-magical basis is [`../research/high-medieval-baseline-c1200.md`](../research/high-medieval-baseline-c1200.md). Any precise regional rate introduced later requires narrower research.

## Causal model

Current consumption chain:

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

Current carry/transfer chain:

```text
actor carry + capacity
  + household membership/store identity
  + exact actor location
  + household grain stock
      -> Draw: own stock decreases == actor carry increases
      -> Deposit: actor carry decreases == own stock increases

actor carry
  + selected other household/store
  + exact actor location
      -> Gift: actor carry decreases == receiving stock increases
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

There is no hidden hunger timer, background economy clock, second time stream, Godot-owned stock or client-owned carry balance.

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

An actor belongs to at most one household. Household membership lists are authoritative; the current reverse membership lookup is derived bounded runtime work rather than persisted/indexed truth.

### Place

The current resource place record contains a stable `EntityId`, local X/Z and non-negative per-axis tolerance. Resource occupancy uses exact arrival semantics: the actor's authoritative X/Z must fall within the stored tolerance on each axis. Unlike the M1 rest-body occupancy query, this check does not add actor body radius.

In the acceptance village the short household store intentionally uses the same X/Z/tolerance values as the M1 RestNeed target. That is one content fact shared by two mechanics, not a hidden protocol/Godot coordinate contract. The Godot client verifies this relationship from the two projections before caching the scenario target.

### Actor

Actors carry one authoritative grain quantity and one authoritative grain carry-capacity quantity. Both are non-negative and carried grain cannot exceed capacity. This is a bounded one-staple carry slot, not a generic inventory, container, item-stack or equipment model.

Consume, Draw, Deposit and Gift are ordinary actor-generic World laws. Consume/Draw/Deposit derive the actor's household from authoritative membership. Gift accepts a receiving household identity and refuses the actor's own household so Deposit remains the own-household operation.

The current application policy excludes the controlled actor from autonomous Consume. This is a decision-source policy only; it does not grant NPCs a different World mutation law. Draw/Deposit/Gift have no autonomous application policy yet.

## Acceptance composition

The current M2 acceptance village is code-defined Core content, not protocol-owned fixture state. It contains:

- one controlled actor in the surplus household with bounded positive grain-carry capacity;
- the existing M1 RestNeed actor in the household intended to become short;
- one additional surplus-household NPC with exact spatial state, idle locomotion intent and the same bounded positive grain-carry capacity as the controlled actor;
- one store place per household;
- one household aggregate per store.

The short-household RestNeed actor keeps zero carry capacity in the current bounded content because M2.5 only stages the actors required for the next player/NPC Gift parity checkpoint; this is content, not a different actor law.

The builder owns concrete acceptance IDs and returns only the controlled-actor session binding. Protocol discovers actors through `World::actor_ids()` and households through `World::household_ids()`; it does not keep a growing list of feature-named NPC IDs. Godot similarly discovers the living-need actor's household/store through projections rather than duplicating those acceptance IDs or coordinates.

This is deliberately not an ECS, entity registry, scenario DSL or data-driven content framework.

## Fidelity / representation level

Households are aggregate-resolved state because current gameplay needs shared stock and membership but not separately identified grain items. Actors remain identity-resolved. Grain is a scalar conserved quantity in household stock or one actor carry slot rather than individual item entities.

No automatic aggregation/disaggregation framework is introduced.

## Conservation and promotion-demotion invariants

For current Consume:

- stock never becomes negative;
- accepted Consume removes exactly the configured positive amount;
- remaining consume budget decreases by exactly one;
- refusal changes neither stock nor budget;
- shortage is recomputed from stock and threshold.

For current carry/transfer laws:

- `0 <= carried_grain_units <= grain_carry_capacity_units` always holds;
- accepted Draw decreases the actor's own household stock by exactly the amount added to actor carry;
- Draw moves `min(store stock, free carry capacity)` and therefore cannot be an accepted no-op;
- accepted Deposit moves the entire carry to the actor's own household and clears carry to zero;
- accepted Gift moves the entire carry to a different selected household and clears carry to zero;
- Deposit/Gift validate checked stock addition before mutating either side, so overflow is an atomic refusal;
- own-household Gift is refused rather than becoming a second Deposit spelling;
- refusal leaves household stocks, actor carry, tick and revision unchanged;
- no representation-level promotion/demotion exists yet.

Future Work/production and standing-transfer mechanics must preserve these same quantities rather than create parallel subsystem balances.

## Magic sensitivity surface

Not implemented in this increment. The model is sensitive to future magic that changes:

- grain creation/transformation and scarcity;
- preservation/spoilage;
- transport, carry capacity or containment;
- labor/productive capacity;
- access to stores or exact-spatial constraints.

A future magical capability must change concrete authoritative facts/rules and let shortage/transport respond causally; it must not add a global `magic_multiplier` or parallel magical inventory.

## Implemented magic deviations

None.

## Inputs

Consume, Draw and Deposit take only the acting `EntityId`. The actor does not supply an amount, stock target, shortage flag or household balance. World derives the actor's household; Consume uses the household's authoritative consume amount, Draw derives the transferable quantity from current stock and free carry capacity, and Deposit uses the entire authoritative carry.

Gift takes the acting `EntityId` plus the receiving household `EntityId`. The caller does not supply a gift amount; World transfers the actor's entire authoritative carry after validating the receiving household/store and rejecting the actor's own household target.

The application layer may propose autonomous Consume only after the ordinary locomotion transition and only when a read-only Core feasibility check says current authoritative state satisfies membership, exact store presence, remaining budget and stock. `World::consume_household_grain()` still revalidates those prerequisites before mutation.

The first vertical Godot shortage scenario submits only a zero controlled locomotion intent while the ordinary NPC decision path advances. It does not submit a resource/economic intent, set stock, set shortage or teleport the NPC. Draw/Deposit/Gift are not yet exposed through protocol/GDExtension/Godot commands.

## Transitions / scheduling

Consume, Draw, Deposit and Gift are immediate authoritative transitions, not simulation-time schedules. Every accepted transition advances `WorldRevision` exactly once while leaving `SimulationTick` unchanged; every refusal leaves both unchanged.

An accepted Consume:

1. validates actor identity and existence;
2. resolves authoritative household membership;
3. validates the household resource state;
4. requires exact actor spatial state inside the referenced store place;
5. requires remaining consume budget and sufficient stock;
6. subtracts the configured amount and decrements the budget;
7. commits one revision-only mutation.

An accepted Draw:

1. validates the actor, carry state and own household resource state;
2. requires exact actor presence inside the own household store;
3. requires both free carry capacity and non-empty household stock;
4. derives `min(stock, capacity - carried)` without caller quantity input;
5. subtracts that amount from household stock and adds the same amount to carry;
6. commits one revision-only mutation.

An accepted Deposit:

1. validates the actor, carry state and own household resource state;
2. requires exact actor presence inside the own household store and non-empty carry;
3. checks that `stock + carry` fits the authoritative integer domain before mutation;
4. moves the entire carry into own household stock and clears carry;
5. commits one revision-only mutation.

An accepted Gift:

1. validates the actor and selected receiving household identities/state;
2. requires non-empty carry and rejects the actor's own household as a target;
3. requires exact actor presence inside the receiving household store;
4. checks that `receiving stock + carry` fits the authoritative integer domain before mutation;
5. moves the entire carry into receiving stock and clears carry;
6. commits one revision-only mutation.

For bounded autonomous application, one call to `protocol::Simulation::advance_locomotion_tick()` retains the existing M2.3 ordering:

1. collect current controlled/NPC locomotion intents from the Core actor-id view;
2. attempt the full locomotion batch;
3. if locomotion fails, no autonomous Consume is attempted;
4. if locomotion succeeds, preserve and return the locomotion tick/revision in the movement batch;
5. against the post-movement World, consider feasible non-controlled actor Consume actions in the same bounded actor order;
6. each accepted Consume is an ordinary revision-only World transition and therefore may make a later resource read report the same tick with a greater revision;
7. an ordinary Consume refusal cannot retroactively fail or rewrite the successful movement batch.

M2.5 does not add automatic Draw, Deposit or Gift scheduling. Their decision-source and client command integration belongs to the next bounded vertical increment.

The current acceptance content permits at most one autonomous Consume across the bounded scenario because only the short-household NPC has a positive remaining budget of one. This proves the ordering contract without turning locomotion ticks into a recurring meal cadence. Recurring consumption over world time requires later time-system admission rather than treating locomotion ticks as meals.

## Outputs / consequences

Current direct outputs are:

- updated authoritative household stock;
- updated actor carried grain after accepted Draw/Deposit/Gift;
- updated remaining consume budget after Consume;
- derived shortage state from the resulting household stock;
- typed transition results carrying actor/household identity, moved quantity, resulting quantities and current tick/revision;
- unchanged `SimulationTick` and exactly one later `WorldRevision` for each accepted immediate resource transition;
- a village-scoped household-resource protocol read with authoritative tick/revision/version context for the already exposed shortage vertical;
- actor-scoped observed-world output computed from the deterministic actor-id view;
- GDExtension translation of household identity/membership/store/resource/status fields;
- localized Godot household-resource feedback and bounded RU/EN shortage scenario evidence.

The movement batch and a resource read taken immediately afterward may therefore deliberately carry different revisions on the same tick. Presentation retains the movement revision for the controlled actor's spatial sample while a later observed-world refresh may reconcile its latest world revision to the post-Consume resource revision.

## Player-facing exposure

The purpose-built village household-resource projection is translated through the GDExtension and read by the Godot client. The client resolves the living-need actor's household and store from authoritative discovery, renders the supplied `adequate`/`shortage` status with stock/threshold quantities in the localized diagnostics HUD, and never calculates or mutates the resource state itself.

The bounded `shortage` scenario proves the first real vertical loop: the tracked household starts adequate, the NPC reaches its Core-owned store through ordinary locomotion, application-level autonomous Consume makes the household short, the resource read reports the same tick at one later revision, and RU/EN Godot feedback renders that authoritative result without a player economic command.

Actor carry plus Draw/Deposit/Gift are currently **native-only World capabilities**. M2.5 intentionally does not add protocol commands, GDExtension mutation methods, Godot controls or carry UI. The next bounded vertical increment must expose these existing laws rather than duplicate them in the client.

This is not yet the milestone's multiple-path player intervention acceptance: Gift player/NPC parity, Work/production and standing transfer remain later capabilities.

## Uncertainty

The current resource unit, acceptance stocks and carry capacities are intentionally abstract fixtures. They establish conservation, scarcity and action prerequisites but do not claim historical household capacity, human carrying capacity or consumption rates.

## Simplifications

Current simplifications include:

- one staple only;
- one scalar grain carry slot per actor rather than general inventory/items/containers;
- perfect storage with no spoilage/pests;
- no item quality or individual grain objects;
- bounded acceptance consume opportunities instead of a meal schedule;
- no processing from grain to food;
- no prices, currency, credit or market clearing;
- no household internal allocation rule beyond the current Consume/Draw/Deposit operations;
- one bounded code-defined acceptance village rather than a general content authoring system.

## Deliberately not simulated

Not yet represented:

- general inventory, item stacks, containers or equipment;
- player/NPC Gift decision-source integration and player-facing carry/transfer commands;
- work/production and field assignment;
- standing household transfer pledges;
- wages, rents, tithe, tenure or institutional allocation;
- transport logistics beyond exact actor movement plus the bounded carried-grain quantity;
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

- a meaningful distinction between stored or carried grain lots/items;
- multiple independently constrained carried resource/container types;
- consumption cadence as real world-time behavior rather than a bounded acceptance transition;
- household allocation rules that require member-specific claims or obligations;
- storage loss/processing that materially changes shortage;
- a concrete magical capability that removes a load-bearing scarcity, storage, carry or access assumption;
- regional historical evidence showing that a newly load-bearing assumption is unsuitable for the chosen scenario.
