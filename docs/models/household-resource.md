# Model: Household Resource

Status: DRAFT

## Gameplay purpose

Milestone 2 begins the first authoritative household resource loop with one staple: unmilled grain represented in integer grain-units. The current implemented subset proves that a household can hold real stock, consume it through an actor-generic World rule, become short through bounded NPC autonomy, expose that state through the real client, move the same conserved grain between household stock and a bounded actor carry slot through Draw, Deposit and Gift laws, and create a bounded positive grain yield through an actor-generic field Work law.

M2.6 exposes the controlled actor's carry/member-household state and transfer laws through semantic protocol/GDExtension commands plus a minimal localized Godot affordance. M2.7 adds the native field/work assignment and shared World Work transition. M2.8 exposes that existing Work law through a purpose-built field projection, semantic controlled Work command, GDExtension translation and localized field cue/HUD without moving production authority into Godot.

This model is deliberately narrower than a general inventory, economy, market, farming, labor or needs framework.

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
- one bounded `FieldWorkAssignmentState` references an ordinary work-place `EntityId` and a durable destination-household `EntityId`, owns a positive fixture yield and a non-negative remaining-work count;
- Work takes only the acting actor identity, requires exact-spatial field occupancy, adds exactly the assignment yield to the destination household, decrements remaining work completions by one, advances `WorldRevision` exactly once and leaves `SimulationTick` unchanged;
- Work does not require household membership and does not select its destination from current shortage; the destination is durable content truth;
- missing spatial state, being outside the field, exhausted work capacity, invalid/dangling work content or checked stock overflow is a Work refusal with no mutation;
- snapshot/restore preserves carry/capacity, household resource quantities and field-work place/destination/yield/remaining count so later Consume/Draw/Deposit/Gift/Work outcomes continue deterministically;
- the acceptance village is created by one bounded Core content builder rather than by named protocol actor fields;
- protocol decision collection iterates the deterministic Core actor-id view, applies RestNeed movement to actors that have that state, and gives other exact-spatial non-controlled actors an idle locomotion intent;
- bounded autonomous Consume is considered only for non-controlled actors whose current authoritative state makes the action feasible;
- locomotion commits first; a successful autonomous Consume may then create a later `WorldRevision` on the same `SimulationTick` while the returned movement batch retains the locomotion commit revision;
- the next locomotion batch remains consecutive in `SimulationTick` and strictly later in `WorldRevision`, including after an explicit controlled resource or Work command creates an intervening revision;
- actor observation remains actor-scoped, while a separate village household-resource read exposes household membership, store identity/footprint, stock, threshold and derived shortage;
- a controlled-actor carry projection exposes authoritative carried grain/capacity plus optional authoritative member-household identity/stock;
- controlled Draw/Deposit/Gift protocol commands carry no amount; Gift carries only the receiving household identity and every accepted result reports the Core-owned moved/resulting quantities and tick/revision;
- a purpose-built field-work projection exposes the authoritative field identity/footprint, durable destination household, fixture yield and remaining completion count without becoming a mutable client job model;
- the controlled Work protocol command carries no payload and delegates to the same actor-generic `World::complete_field_work()` law; accepted results report produced grain, destination stock, remaining availability and tick/revision;
- the GDExtension translates resource/Work projections/results/errors without recomputing resource law or owning resource state;
- Godot resolves the tracked acceptance actor, household, shared rest/store position and field cue position from authoritative projections rather than fixture IDs or coordinates;
- the Godot HUD renders protocol/Core-owned shortage, stock, carry and remaining Work availability through localized RU/EN text and exposes bounded keyboard Draw/Deposit/Gift/Work affordances;
- the bounded `shortage` playtest advances only ordinary locomotion, submits no player economic command, observes the autonomous adequate-to-shortage transition and proves movement revision `R` followed by resource revision `R+1` on the same tick;
- the bounded `gift` playtest waits for that autonomous shortage, draws from the controlled actor's own household, reaches the receiving store through ordinary locomotion, Gifts the entire carry, observes authoritative stock/carry conservation and shortage relief, and observes the M1 RestNeed as `blocked` while the player occupies the shared store/rest footprint;
- the bounded `work` playtest waits for the same autonomous shortage, reaches the projected field through ordinary locomotion, completes exactly one Work action, observes destination stock increase by the projected fixture yield and remaining Work availability fall from one to zero, then proves a second Work refusal is non-mutating.

The current verticals therefore expose one Core resource truth in the real client across autonomous shortage, Gift and Work intervention without introducing a client-side balance, generic inventory, scenario-only stock mutation or duplicate production law.

## Historical baseline and region

Acceptance context: England around 1200, an inland temperate agrarian village where household grain storage and agricultural labor are causally relevant.

The integer stocks, thresholds, consume amounts, consume budgets, carry capacities and field yields are acceptance fixtures, not historical ration, yield, carrying-capacity, price, acreage or bushel claims. The broader non-magical basis is [`../research/high-medieval-baseline-c1200.md`](../research/high-medieval-baseline-c1200.md). Any precise regional rate introduced later requires narrower research.

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

Current bounded Work chain:

```text
field work assignment
  + ordinary field place
  + durable destination household
  + positive fixture yield
  + remaining work completions
  + exact actor location
      -> Work validation
      -> destination stock increases by fixture yield
      -> remaining work completions decreases by one
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

Current player intervention path:

```text
World resource/carry/field-work truth
  -> purpose-built protocol projections
  -> semantic Draw / Deposit / Gift / Work command
  -> same actor-generic World law
  -> authoritative transition result
  -> GDExtension Dictionary translation
  -> localized resource/carry/Work HUD + field cue reconciliation
```

M2.8 adds no automatic Work application loop. Work remains an explicit semantic command over the M2.7 native law; the bounded remaining-completion field is not a scheduler or time source.

There is no hidden hunger timer, background economy clock, crop calendar, labor scheduler, second time stream, Godot-owned stock or client-owned carry balance.

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

The current resource place record contains a stable `EntityId`, local X/Z and non-negative per-axis tolerance. The same bounded record type represents household stores and the field work point. Resource/work occupancy uses exact arrival semantics: the actor's authoritative X/Z must fall within the stored tolerance on each axis. Unlike the M1 rest-body occupancy query, this check does not add actor body radius.

In the acceptance village the short household store intentionally uses the same X/Z/tolerance values as the M1 RestNeed target. That is one content fact shared by two mechanics, not a hidden protocol/Godot coordinate contract. The Godot client verifies this relationship from the two projections before caching the scenario target.

Store occupancy is intentionally non-exclusive. When the controlled actor occupies the short household's store to Gift, the existing M1 other-actor occupancy rule can make the RestNeed actor `blocked`; resource actions do not suppress or special-case that interaction.

The field is a separate ordinary place. M2.8 exposes its identity/footprint only through the purpose-built Work projection needed for the field cue and interaction; it does not introduce acreage, parcels, crop entities, navigation ownership or a generic place ontology.

### Actor

Actors carry one authoritative grain quantity and one authoritative grain carry-capacity quantity. Both are non-negative and carried grain cannot exceed capacity. This is a bounded one-staple carry slot, not a generic inventory, container, item-stack or equipment model.

Consume, Draw, Deposit, Gift and Work are ordinary actor-generic World laws. Consume/Draw/Deposit derive the actor's household from authoritative membership. Gift accepts a receiving household identity and refuses the actor's own household so Deposit remains the own-household operation. Work does not require household membership; the field assignment owns the destination household.

The current application policy excludes the controlled actor from autonomous Consume. This is a decision-source policy only; it does not grant NPCs a different World mutation law. Draw/Deposit/Gift have no automatic NPC application policy; M2.6 adds an explicit controlled-actor protocol command source over the shared laws. M2.8 likewise adds only an explicit controlled Work command source; equivalent actors remain governed by the same M2.7 World law and there is no automatic labor policy yet.

### Field work assignment

The current acceptance world owns exactly one optional bounded `FieldWorkAssignmentState`. It is content state, not a new entity kind and not an allocator/indexed task registry. It contains:

- referenced work-place `EntityId`;
- durable destination-household `EntityId`;
- positive fixture `yield_grain_units`;
- non-negative `remaining_work_completions`.

The assignment is snapshot truth because every field changes future Work outcomes. Its destination is not recomputed from live shortage, and its remaining count is not a clock or refill schedule.

## Acceptance composition

The current M2 acceptance village is code-defined Core content, not protocol-owned fixture state. It contains:

- one controlled actor in the surplus household with bounded positive grain-carry capacity;
- the existing M1 RestNeed actor in the household intended to become short;
- one additional surplus-household NPC with exact spatial state, idle locomotion intent and the same bounded positive grain-carry capacity as the controlled actor;
- one store place per household;
- one household aggregate per store;
- one separate field place;
- one bounded field-work assignment whose durable destination is the short household, with positive fixture yield and one remaining completion.

The short-household RestNeed actor keeps zero carry capacity in the current bounded content because no current decision policy asks that actor to carry grain; this is content, not a different actor law.

The builder owns concrete acceptance IDs and returns only the controlled-actor session binding. Protocol discovers actors through `World::actor_ids()` and households through `World::household_ids()`; it does not keep a growing list of feature-named NPC IDs. Godot similarly discovers the living-need actor's household/store through projections and discovers the field from `FieldWorkProjection`; it does not duplicate those acceptance IDs or coordinates.

This is deliberately not an ECS, entity registry, scenario DSL or data-driven content framework.

## Fidelity / representation level

Households are aggregate-resolved state because current gameplay needs shared stock and membership but not separately identified grain items. Actors remain identity-resolved. Grain is a scalar quantity in household stock or one actor carry slot rather than individual item entities. Field Work produces a bounded scalar fixture yield directly into the assignment's destination household; crops, harvested item lots and transport from field to store are not separately resolved at this stage.

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
- refusal leaves household stocks, actor carry, tick and revision unchanged.

For current bounded Work:

- fixture yield is strictly positive content truth;
- accepted Work adds exactly that yield to the durable destination household;
- accepted Work decrements remaining completions by exactly one and cannot make it negative;
- zero remaining completions is a refusal, preventing repeated revision-only unbounded creation;
- destination stock addition is checked before mutation, so overflow is atomic;
- refusal leaves destination stock, assignment state, tick and revision unchanged;
- no representation-level promotion/demotion exists yet.

Future standing-transfer mechanics must preserve the same stock/carry quantities rather than create parallel subsystem balances. Later production fidelity may replace the fixture abstraction only through an admitted causal model, not by silently adding a second balance.

## Magic sensitivity surface

Not implemented in this increment. The model is sensitive to future magic that changes:

- grain creation/transformation and scarcity;
- preservation/spoilage;
- transport, carry capacity or containment;
- labor/productive capacity;
- access to stores, fields or exact-spatial constraints.

A future magical capability must change concrete authoritative facts/rules and let shortage/transport/production respond causally; it must not add a global `magic_multiplier` or parallel magical inventory.

## Implemented magic deviations

None.

## Inputs

Consume, Draw and Deposit take only the acting `EntityId` at Core law level. The actor does not supply an amount, stock target, shortage flag or household balance. World derives the actor's household; Consume uses the household's authoritative consume amount, Draw derives the transferable quantity from current stock and free carry capacity, and Deposit uses the entire authoritative carry.

Gift takes the acting `EntityId` plus the receiving household `EntityId`. The caller does not supply a gift amount; World transfers the actor's entire authoritative carry after validating the receiving household/store and rejecting the actor's own household target.

Work takes only the acting `EntityId`. The caller does not supply yield, destination household, completion count or work duration. World reads those values from the authoritative field-work assignment and requires the actor's exact spatial state to occupy the referenced field place.

At the protocol boundary, controlled Draw and Deposit carry no payload and controlled Gift carries only the receiving household id. M2.8 adds controlled Work with no payload. `protocol::Simulation` binds the controlled actor and delegates all four commands directly to the existing World laws; the GDExtension translates typed outcomes without owning mutation rules. `FieldWorkProjection` is read-only discovery of the bounded assignment/place needed by the client and carries no write authority.

The application layer may propose autonomous Consume only after the ordinary locomotion transition and only when a read-only Core feasibility check says current authoritative state satisfies membership, exact store presence, remaining budget and stock. `World::consume_household_grain()` still revalidates those prerequisites before mutation. No automatic Work proposal is added by M2.8.

The Godot shortage scenario still submits only zero controlled locomotion while ordinary NPC policy advances. Gift uses the real controlled Draw/Gift commands and ordinary locomotion; it never sets stock/carry, teleports, or supplies a transfer amount. Work likewise uses ordinary locomotion to the projected field and the real controlled Work command; it never sets stock, teleports, or supplies yield/destination/amount.

## Transitions / scheduling

Consume, Draw, Deposit, Gift and Work are immediate authoritative transitions, not simulation-time schedules. Every accepted transition advances `WorldRevision` exactly once while leaving `SimulationTick` unchanged; every refusal leaves both unchanged.

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

An accepted Work:

1. validates actor identity/existence and the bounded field-work assignment;
2. resolves the assignment's work place and durable destination household;
3. validates destination household resource state;
4. requires exact actor spatial state inside the referenced field tolerance;
5. requires a positive remaining work count;
6. checks that `destination stock + fixture yield` fits the authoritative integer domain before mutation;
7. adds exactly the fixture yield to destination stock and decrements remaining work completions by one;
8. commits one revision-only mutation.

For bounded autonomous application, one call to `protocol::Simulation::advance_locomotion_tick()` retains the existing M2.3 ordering:

1. collect current controlled/NPC locomotion intents from the Core actor-id view;
2. attempt the full locomotion batch;
3. if locomotion fails, no autonomous Consume is attempted;
4. if locomotion succeeds, preserve and return the locomotion tick/revision in the movement batch;
5. against the post-movement World, consider feasible non-controlled actor Consume actions in the same bounded actor order;
6. each accepted Consume is an ordinary revision-only World transition and therefore may make a later resource read report the same tick with a greater revision;
7. an ordinary Consume refusal cannot retroactively fail or rewrite the successful movement batch.

Draw, Deposit, Gift and controlled Work are explicit immediate commands rather than automatic schedules. Any accepted command may create an intervening revision between locomotion ticks; the next locomotion batch advances from that latest world revision rather than rejecting a valid command history.

Work is an explicit bounded fixture exception to the normal expectation that production processes consume simulation time. M2.8 does not add a labor schedule, duration timer, crop calendar or refill; the remaining-work count is content capacity for this acceptance proof, not time progression.

The current acceptance content permits at most one autonomous Consume across the bounded scenario because only the short-household NPC has a positive remaining budget of one. It also permits exactly one accepted field Work transition because the assignment starts with one remaining completion. These bounds prove ordering/production rules without turning locomotion ticks into meals or field-work cycles. Recurring consumption or production over world time requires later time-system admission.

## Outputs / consequences

Current direct outputs are:

- updated authoritative household stock;
- updated actor carried grain after accepted Draw/Deposit/Gift;
- updated remaining consume budget after Consume;
- updated remaining field-work completions after Work;
- derived shortage state from the resulting household stock;
- typed Core transition results carrying actor/household/place identity, moved or produced quantity, resulting quantities and current tick/revision;
- unchanged `SimulationTick` and exactly one later `WorldRevision` for each accepted immediate resource/Work transition;
- a village-scoped household-resource protocol read with authoritative tick/revision/version context;
- a controlled-actor carry/member-household projection with authoritative carry/capacity and optional own-household stock;
- a purpose-built field-work projection with authoritative field footprint, destination, fixture yield and remaining completion count;
- semantic controlled Draw/Deposit/Gift/Work command results with no client-authored resource quantity or Work yield/destination;
- actor-scoped observed-world output computed from the deterministic actor-id view;
- GDExtension translation of household/carry/resource/Work projections, results and errors;
- localized Godot shortage/carry/Work feedback, projected field cue and bounded RU/EN shortage, Gift and Work scenario evidence.

The movement batch and a resource read taken immediately afterward may deliberately carry different revisions on the same tick. Presentation retains the movement revision for controlled spatial state while a later observed-world refresh may reconcile its latest world revision to a post-Consume or explicit command revision.

## Player-facing exposure

The purpose-built village household-resource, controlled carry and field-work projections are translated through the GDExtension and read by the Godot client. The client resolves household/store/field identity and positions from authoritative discovery, renders supplied status/quantities, and does not calculate or mutate resource state itself.

The bounded `shortage` scenario proves the autonomous scarcity vertical: the tracked household starts adequate, the NPC reaches its Core-owned store through ordinary locomotion, application-level autonomous Consume makes the household short, and RU/EN Godot feedback renders that authoritative result without a player economic command.

The bounded `gift` scenario proves the first player intervention vertical over M2.5 laws: the controlled actor draws at its own store, waits for the neighbour to become short, moves through the ordinary controlled locomotion boundary to the discovered receiving store, Gifts its whole carry, observes target stock increase and carry clear on one revision-only transition, and renders the resulting adequate household/carry state in RU and EN. Because that store is also the M1 RestNeed target, the same run observes `blocked` while the player occupies the footprint; Gift does not invent an exclusivity exception.

The bounded `work` scenario proves the second intervention path over the M2.7 law: the client first observes the neighbour become short, follows the projected field position through ordinary locomotion, completes one amount-less Work command, observes destination stock increase by the projected yield and remaining availability fall from one to zero on one revision-only transition, then proves a second Work is refused as exhausted without mutation. RU and EN render the field cue, Work availability/action feedback and resulting adequate household state.

The interactive client exposes only the smallest bounded affordance: E Draw, R Deposit, G Gift to the discovered tracked neighbour household, and F Work while at the projected field. This is an acceptance interaction surface, not a general inventory, job browser, target-selection UI or source of authoritative quantities.

This is not yet whole-milestone closure: standing household transfer remains a later capability.

## Uncertainty

The current resource unit, acceptance stocks, carry capacities and Work yield are intentionally abstract fixtures. They establish conservation, scarcity, bounded production and action prerequisites but do not claim historical household capacity, human carrying capacity, field productivity, labor duration, acreage or consumption rates.

## Simplifications

Current simplifications include:

- one staple only;
- one scalar grain carry slot per actor rather than general inventory/items/containers;
- perfect storage with no spoilage/pests;
- no item quality or individual grain objects;
- bounded acceptance consume opportunities instead of a meal schedule;
- one field place and one bounded fixture-yield assignment instead of crops, seasons, acreage or labor-time modeling;
- direct Work yield into the durable destination household rather than separately resolved harvest/transport/storage steps;
- no processing from grain to food;
- no prices, currency, credit or market clearing;
- no household internal allocation rule beyond the current Consume/Draw/Deposit operations;
- one bounded code-defined acceptance village rather than a general content authoring system;
- one tracked-neighbour Gift affordance and one projected-field Work affordance rather than general selection/task UI.

## Deliberately not simulated

Not yet represented:

- general inventory, item stacks, containers or equipment;
- automatic NPC Gift decision-source policy or scheduling;
- automatic actor Work policy, work duration/refill, crop calendars, seasonal yield, tools, skills, wages, tenure or acreage;
- standing household transfer pledges;
- rents, tithe or institutional allocation;
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
- recurring production whose timing/crop state cannot be represented by a bounded fixture completion;
- household allocation rules that require member-specific claims or obligations;
- storage loss/processing that materially changes shortage;
- a concrete magical capability that removes a load-bearing scarcity, storage, carry, labor or access assumption;
- regional historical evidence showing that a newly load-bearing assumption is unsuitable for the chosen scenario.
