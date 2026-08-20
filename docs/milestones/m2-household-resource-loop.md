## Milestone 2 — Household Resource Loop

**Specification status:** target contract for the whole Milestone 2 capability. This document is deliberately broader than any single implementation task. M2 is implemented through bounded tasks below; a native-only task is evidence or an enabling step, not Milestone acceptance by itself.

**Integration rule:** do not turn M2 into a simulation waiting room. Native-only foundations may be developed and verified locally (or on an M2 feature branch), but the first landed M2 product slice must still be vertical and observable: composition/application ownership + authoritative consume/shortage + protocol discovery/read + localized Godot feedback + bounded proof. Later resource paths follow the same rule: short native-first law work may precede its presentation step, but must not accumulate into a hidden general economy.

### Product outcome / конечный результат

A small village can develop a grain shortage from ordinary household consumption without player intervention. The player can then reduce or clear that shortage through **more than one** real world path — gift, household transfer, and work — while every stock change remains authoritative Simulation state. Godot only presents projections and submits semantic intents.

This milestone delivers the whole Milestone 2 capability from `docs/ROADMAP.md`, not a RestNeed extension and not a general economy engine.

Success looks like the product loop:

```
world cause (household consumption / failed or absent production)
  -> authoritative stock and derived shortage
  -> player/NPC opportunity (gift, household transfer, work)
  -> visible Godot feedback
  -> player choice through shared transaction rules
  -> persistent world stock change

```

`main` stays playable throughout. Milestone 1 RestNeed remaining behavior stays intact.

### Current source baseline

These are current-repository facts the specification must reconcile, not desired architecture. M2.1–M2.11 are landed in source; whole-milestone acceptance evidence is recorded in `docs/VERIFICATION.md` on the candidate revision.

- `src/sim/World` owns actors, places, households, optional field-work assignment, standing transfer pledges, `tick_`, `revision_` and `seed_`. `ActorState` carries optional exact `SpatialState`, optional `RestNeedState`, actor grain carry/capacity, `ActorLocomotionCapability` and `GroundedLocomotionContinuation`. `snapshot()` builds a `WorldSnapshot` whose schema version is the executable `kWorldSnapshotSchemaVersion` (currently 9).
- `protocol::Simulation` binds control to the Core-owned acceptance village rather than spawning named feature NPCs. Observation and RestNeed collection use the deterministic Core actor-id view.
- Godot reads purpose-built projections and must not own world quantities. Draw/Deposit/Gift/Work/HouseholdTransfer cross protocol → GDExtension → Godot.
- Resource transitions are revision-only. The only production `SimulationTick` stream remains fixed locomotion. `World::advance_one_tick()` is native-test-only and must not be used to force a stock change.

### Player-visible capability

After this Epic, a short playable session must make the following understandable without a quest flag:

1. Who holds the staple (household store, not a Godot inventory).
2. That the local problem can appear while the player does nothing.
3. That shortage is a derived world fact, shown as localized HUD/status.
4. That the player can address it by giving carried grain, executing the surplus household's standing transfer pledge that moves grain into the short household, or working at the production place.
5. That an equivalent NPC is subject to the same transfer/work rules when it has the same prerequisites, proven by shared World rules and native tests rather than by an in-client NPC economy.
6. That inventory/pledge UI never mutates stock or invents money.

The player remains an ordinary simulated actor. Control source must not create a privileged economic path.

### Functional requirements / функциональные требования

Unless noted, every requirement is actor-generic: the same World rule applies to human-controlled and NPC actors with the same prerequisites.

- **FR1 Household stock and membership.** World owns household aggregates with `EntityId`, member actors, non-negative integer grain stock, shortage threshold, and a store place. Household member lists are the single authoritative membership source for M2: an actor belongs to **zero or one** household, member ids are unique, every member id resolves to an existing actor, and restore/composition validation rejects duplicate cross-household membership or dangling members. A reverse actor→household lookup may exist only as a derived index rebuilt from household state.
- **FR2 Derived shortage.** Shortage is computed as `stock < threshold`. It is not a stored flag and is not owned by Godot.
- **FR3 Consume.** A member occupying the household store can apply an atomic `Consume` that decreases household stock by the household's positive content-owned consume amount and decrements the household's remaining consume budget by one. It refuses without mutation when the budget is zero, stock is below the consume amount, membership/occupancy is invalid, or any invariant fails. Accepted `Consume` always changes state; zero-amount content is invalid rather than a successful no-op.
- **FR4 Carry and store access.** An actor has authoritative non-negative `carried_grain` and authoritative non-negative `grain_carry_capacity`, with invariant `carried_grain <= grain_carry_capacity`; both are snapshot truth. A member occupying the store can `Draw` (fill the carry slot to its bound, limited by available stock) or `Deposit` (move the entire carried amount into the store). Neither command carries a client-supplied quantity. `Draw` refuses if nothing can move (carry already full or store empty); `Deposit` refuses if carry is empty; accepted operations always change state.
- **FR5 Gift.** An actor occupying the **receiving household's store** tolerance can unilaterally transfer its **entire carried grain** into that household stock. `Gift` refuses when carry is empty or when the acting actor is itself a member of the target household: that case is `Deposit`, and the refusal is what keeps the two transitions distinct. There is no client-supplied amount and no separate hand-off place in this milestone.
- **FR6 Household transfer.** A **member of the source/surplus household** occupying that household's store executes its standing transfer pledge. Membership is the M2 authorization rule: a passer-by or member of another household cannot spend the source household's stock merely by occupying the store. The pledge stores a durable content `destination_household` `EntityId` (the short household in the acceptance village), not a live `stock < threshold` lookup. Execution is **all-or-nothing for the entire remaining pledged quantity**: World atomically decreases source stock and increases the destination household's stock by that amount, and the pledge becomes zero. There is no client-supplied amount and no partial execution. The actor is the executor, not the grain destination. It refuses when the pledge is already zero, source stock does not cover it, destination is invalid/self, membership/occupancy fails, or destination addition would overflow. No second commodity and no payment-in-grain: this is a one-way household-to-household transfer, not a priced exchange.
- **FR7 Work.** An actor occupying the field work tolerance can complete one bounded labor action that adds positive fixture yield to a designated household store without taking another household's stock. Designation is a durable content `destination_household` `EntityId` on the field/work assignment. The assignment also owns a non-negative `remaining_work_completions` count; each accepted `Work` decrements it by one, and `Work` refuses once it reaches zero. Yield, destination, and remaining completions are snapshot truth. In the acceptance village the destination **is the short household**, not whoever currently has shortage. This bound prevents repeated revision-only commands from creating unbounded grain while simulation time is intentionally closed.
- **FR8 Autonomy.** The short household can become short through NPC `Consume` without any player economic intent, including while that NPC's Godot node is absent. Consume occupancy uses the short household store, which in the acceptance village is the same place as that NPC's RestNeed rest point. `Simulation::advance_locomotion_tick` remains the application cadence that gives this bounded policy a decision opportunity, but **movement commits first**. After a successful locomotion transition, the application layer evaluates deterministic resource policy against the new authoritative post-movement World state and may invoke the same actor-generic `Consume` rule. Ordinary Consume refusal does not retroactively fail or roll back an already successful locomotion batch. The policy proposes nothing when the consume budget is zero or when `Consume` is currently infeasible (for example insufficient stock or missing occupancy), so an impossible consume does not become a perpetual 60 Hz refusal loop. The consume budget is snapshot truth and is only a bounded acceptance fixture, not a hunger clock. RestNeed itself still emits only movement; it is not extended into a needs framework.
- **FR9 Time and ordering.** Every accepted resource transition advances `WorldRevision` exactly once and does not advance `SimulationTick`; every refused resource transition leaves tick, revision, and resource state unchanged. A successful locomotion call may therefore produce one movement batch at `(tick T+1, revision R+1)` and then one or more bounded post-movement autonomous resource mutations at the **same tick** with later revisions. Resource projections may legitimately report a later revision than the movement batch returned by that call. Resource actions are not a second time stream and are not scheduled merely because 60 Hz exists.
- **FR10 Composition and bounded discovery.** Simulation Core/content owns which entities exist (actors, households, places) and their identities. M2 loads one code-defined acceptance village; for this milestone, “village-scoped” means the typed entities belonging to that bounded composition, not an open-ended regional query. `World` exposes deterministic-order read-only actor ids for observation/decision collection and deterministic-order household ids for the resource discovery projection; `snapshot()` is not an enumeration API. Places are reached from household/work-assignment references and do not need a general place-discovery API in M2. The observed set remains **actor-scoped** and is computed per read in `src/protocol`; it is not owned by `src/sim` and not a growing list of hardcoded named actor fields on `protocol::Simulation`.
- **FR11 Projections.** Godot reads purpose-built shortage, carry/inventory, and transfer-pledge projections and submits semantic intents. It must not set stock or invent money.
- **FR12 Localization.** Player-visible resource UI uses gettext `UI_*` keys for `ru` and `en`. Core/protocol remain language-independent.
- **FR13 Snapshot.** Every landed authoritative M2 value is included in `WorldSnapshot`: household membership/stock/threshold/store/consume amount+budget, actor carried grain+carry capacity, place state, standing pledge state when introduced, and field destination/yield/remaining-work state when introduced. Derived shortage, derived actor→household indexes, projections, and the observed set are not persisted. Schema may bump more than once across M2 as new authoritative state lands; do not reserve speculative future fields merely to avoid later schema bumps.
- **FR14 Parity and playable main.** No `is_player` economy branch. `main` remains playable after every landed increment.
- **FR15 Milestone 1 preserved.** Existing RestNeed behavior and the `rest_interference` / `offscreen` / RU+EN smoke proofs remain valid **as semantics**. Those proofs currently assert exact composition fixtures (the two-entity observed set, sample counts, and exact revision constants in `tools/play.py`, plus the same observed set in `tests/protocol/simulation_test.cpp`), and the first vertical checkpoint legitimately changes all of them by adding a third actor plus household and place records. Rebaselining those fixture constants to the new composition is **authorized**; it is not the scenario-weakening `docs/VERIFICATION.md` forbids. Deleting, relaxing, or skipping an assertion would be. The scenarios must still prove the same behavior: travelling, blocked, satisfied, offscreen continuation, and localized feedback.

### Village scenario

The M2 acceptance world is a bounded rural settlement consistent with the circa-1200 baseline and `PRODUCT.md`'s first village, **without** dialogue, office, or magic.

Historical/context envelope for this acceptance fixture: **England, circa 1200; inland temperate agrarian village; household grain storage and nearby field labor are causally relevant; detailed tenure, manorial obligations, tithe/rent, market institutions, and jurisdiction are deliberately not simulated because no M2 rule branches on them.** The standing transfer pledge is an explicit gameplay/content simplification for neighbourly household relief, not a claim that a universal formal “pledge” institution existed in this form. Precise yields, rations, prices, legal rights, and regional agrarian rates still require targeted research before becoming model constants.

Composition (minimum real content, not a scenario DSL):

- two households: a **short** household and a **surplus** household;
- one production place (field) with an assigned local X/Z work point and occupancy tolerance;
- one store place per household (home store), using the same occupancy pattern already proven by RestNeed rather than a navigation engine;
- for the short household, the store place **is the same assigned X/Z and per-axis tolerance as that NPC's RestNeed rest point**, so RestNeed travel and Consume occupancy coexist without a second need/planner;
- three actors: the human-controlled actor as a member of the surplus household, the existing RestNeed NPC as a member of the short household, and one surplus-household NPC whose role is to prove that household membership and the transfer/work rules are not player-privileged (it is the actor used by the native parity tests and appears in projections as an ordinary member; it runs no economic policy in the client this milestone). Because a presentation only becomes visible once a movement sample arrives, that NPC is entered into the locomotion batch with an idle intent, so it materializes as a visible villager instead of the permanently invisible node this Epic refuses for households and places.

The staple is **unmilled grain in integer grain-units**. Quantities and shortage thresholds are **acceptance fixtures**, not historical yields, rations, prices, or bushels. Targeted regional research is required before any of those become model constants.

The fixtures carry bounded algebraic constraints rather than historical rate claims:

- shortage onset: `initial_short_stock >= shortage_threshold`, `consume_amount > 0`, `initial_short_stock >= consume_amount`, and after the intended bounded consume sequence the stock is `< shortage_threshold`;
- gift/pledge permutation: each of the three player paths executed **once**, in any permutation, with at most one `Draw` per `Gift`, must succeed; concretely `surplus_stock - carry_capacity >= pledge_quantity`;
- controlled actor carry capacity is positive and initially has enough free capacity for the intended `Draw`;
- standing pledge quantity is positive;
- field fixture has positive yield and at least one remaining work completion;
- all fixture additions remain inside the authoritative quantity type and protocol-export range used by the acceptance scenario.

Beyond that bound, repeated draw-and-gift can legitimately drain the surplus store and strand the pledge: `Work` yields to the short household and M2 adds no surplus refill path. That is an authoritative consequence of giving your own household's grain away, not a defect, and it must not be papered over with a hidden refill. Definition of done requires the bounded permutation claim, not indefinite replayability.

No coin, credit, wage-rate table, or second commodity is introduced in this milestone. The third path is a surplus-to-short household grain transfer executed from a standing pledge, not grain-for-grain into actor carry and not a priced exchange.

### Causal model

Canonical mechanic owner after implementation: `docs/models/household-resource.md`.

#### Household as aggregate

A household is World-owned authoritative state, not an actor and not a UI folder.

It has:

- a durable `EntityId` in the same identity space as actors;
- member actor IDs;
- grain stock (non-negative integer);
- a content-owned shortage threshold (integer);
- a store place used for consume/draw/deposit occupancy.

Membership is concrete state. It is not a `Peasant`/`Merchant` class. A UI label such as “neighbor household” is a projection. Household member lists are the one persisted membership truth; an actor may appear in at most one list. A reverse membership index is derived runtime structure only.

Actors may carry a **bounded** grain quantity of the same staple. `carried_grain` and `grain_carry_capacity` are actor-owned authoritative values with `0 <= carried_grain <= grain_carry_capacity`, and both survive snapshot/restore. This is not a general inventory/item ontology: one resource, one carry slot, one household store.

#### Derived shortage

Shortage is derived on read:

```
short = household.stock < household.shortage_threshold

```

Do not persist `is_short`. After restore, shortage is recomputed from stock and threshold exactly as RestNeed satisfaction is recomputed from position.

#### Discrete resource transitions

All resource mutations are semantic, atomic at the individual transition boundary, and validated against current World state. Ordinary refusal uses typed results (`std::expected` / protocol error), not exceptions. **Accepted transitions must change authoritative state; expected zero-work/no-resource cases are refusals, not successful no-ops.** Refusal never advances `WorldRevision`. Additions use checked arithmetic: overflow is refusal, never wrap or clamp.

Required transition kinds:

| Kind | Meaning | Parties / authorization | Spatial gate | Bounded/refusal conditions |
| --- | --- | --- | --- | --- |
| `Consume` | Remove the household's positive content-owned consume amount from stock and decrement remaining consume budget by one. | Household + member actor | Member has exact `SpatialState` inside its household store tolerance | Refuse on zero budget, insufficient stock, invalid content, membership/occupancy failure. |
| `Draw` | Move enough grain from the member household store to fill acting actor carry to capacity, limited by stock. | Household + member actor | Same store occupancy | Refuse if carry already full or stock empty. |
| `Deposit` | Move the acting member's entire carried grain into its household store. | Household + member actor | Same store occupancy | Refuse if carry empty or destination addition would overflow. |
| `Gift` | Move giver's **entire carried grain** to a household the giver is **not** a member of. | Giver actor + receiving household | Giver occupies receiving household **store** tolerance | Refuse on empty carry, own-household target, invalid target, or destination overflow. |
| `HouseholdTransfer` | Move the source household pledge's entire remaining amount from source stock into durable `destination_household`, then set pledge remaining to zero. | **Source-household member** actor + source household + destination household | Executor occupies source household store tolerance | Refuse on zero pledge, insufficient source stock, invalid/self destination, non-member executor, or destination overflow. |
| `Work` | Produce fixture yield into durable field-assignment `destination_household`; decrement `remaining_work_completions` by one. No other household stock is consumed. | Any actor; field/work assignment + destination household | Actor occupies field work tolerance | Refuse on exhausted work completions, invalid/non-positive yield, invalid destination, or destination overflow. |

Standing transfer pledges are source-household state: a non-negative pledged grain quantity plus a durable content `destination_household` `EntityId`. Only a member of the source household may execute the pledge in M2. When execution succeeds, the entire remaining quantity transfers into the destination household and the pledge becomes zero. Pledges are not Godot shop inventory, not grain-for-grain into actor carry, not a second commodity, and not a query for whoever is currently short.

**Naming contract.** With one staple, no coin, and a destination fixed by durable content, this transition has no counter-consideration: it is a one-way household-to-household transfer (neighbourly relief), not an exchange. Model, protocol, and result names must say that — `HouseholdTransfer`, `StandingTransferPledge`, `execute_household_transfer_pledge` — and must **not** be named `trade`, `buy`, `sell`, `price`, or `shop`. Real bilateral exchange needs a second good, coin, or wage-in-kind, all of which are M2 non-goals, so exchange naming is reserved for the later milestone that actually implements it. Do not create that name now as a stand-in.

Work yield per successful labor action is an acceptance fixture, not a historical harvest rate. Land access for this slice is: any exact-spatial actor who occupies the field may work. The field assignment carries a bounded `remaining_work_completions`; M2 does not let repeated revision-only commands manufacture unlimited grain. Do not add lordship, tenure, or wage law yet; record them as magic- and institution-sensitive absences.

#### NPC autonomy without the player

Deterministic NPC policy may invoke the same structured World rules as a human decision source. The short-household NPC must be able to `Consume` while its Godot node is absent, using the already accepted existence ≠ observation ≠ materialization split.

For M2 the application step is deliberately two-phase:

1. collect one locomotion intent for each exact-spatial actor in the bounded acceptance composition: controlled actor intent where session-bound, RestNeed movement where applicable, otherwise an idle intent; no third named NPC field/branch is introduced;
2. commit the ordinary authoritative locomotion batch; if locomotion fails, no autonomous resource policy runs for that call;
3. against the **post-movement** World, evaluate the bounded short-household consume policy in deterministic actor order;
4. invoke actor-generic `Consume` only when its current prerequisites are satisfied; accepted Consume is a separate revision-only World transition, and ordinary refusal does not turn the already committed locomotion batch into failure.

The village “develops a problem without the player” by this causal sequence, not by a background clock:

1. short household stock starts at or above its threshold and has positive consume budget;
2. locomotion places the short-household NPC inside its store/rest tolerance;
3. post-movement deterministic policy sees a currently feasible Consume and Core accepts it;
4. stock falls below threshold after the bounded configured consume sequence;
5. shortage projection becomes short at the same `SimulationTick` but a later `WorldRevision` than the movement batch that enabled it;
6. Godot shows localized shortage without computing it.

Recurring seasonal calendars, daily hunger accumulation, spoilage, and independent production clocks are **out of scope**. One (or a small bounded number of) discrete consume events is enough to prove independence from the player.

Be explicit about the resulting shape: the policy receives one decision opportunity after each successful locomotion tick while the acceptance NPC remains eligible, so a budget greater than one can drain over consecutive locomotion calls once the NPC reaches its store. This is a **bounded burst on/after arrival**, not a claim about realistic meal cadence. The policy does not submit impossible Consume attempts (zero budget, insufficient stock, missing occupancy), so it cannot spin forever on an ordinary refusal. Do not “fix” the burst with a hidden timer or hunger accumulator; gradual consumption needs the simulation-time re-admission this Epic explicitly defers.

#### Time contract (ADR 0009)

`Consume`, `Draw`/`Deposit`, `Gift`, `HouseholdTransfer`, and `Work` are immediate authoritative resource transitions. Each **accepted** transition advances `WorldRevision` exactly once and **must not** advance `SimulationTick` merely to make resource state change. Refusal advances neither counter.

Do not introduce a scheduler, event queue, day/night cycle, or second time stream. Do not consume grain every locomotion tick. If a bounded M2 task finds that M2 acceptance truly cannot be met without non-locomotion time advancement, stop and surface that as a human architecture decision; do not silently reuse 60 Hz ticks as an economy clock.

`MODELING.md` says production/schedules normally depend on `SimulationTick`. M2 `Work` is an explicit fixture exception: one occupancy-gated labor completion yields grain immediately via `WorldRevision` only. That is an accepted trade-off for this milestone, not a silent override of the general time policy. Because `ROADMAP.md` authorizes revision-without-tick for the first stock/consumption slice rather than for production, this exception must be recorded durably in the repository: state it under **Transitions / scheduling** in `docs/models/household-resource.md` and add a one-line pointer in `MODELING.md`'s time section, so the deviation lives with a canonical owner instead of only inside this Epic.

Locomotion remains the only current `SimulationTick` stream. Presentation consecutive-tick guards stay until an explicit later re-admission. A successful `advance_locomotion_tick` may return a movement batch captured at the movement commit revision and then apply bounded post-movement `Consume`, leaving World/resource projections at a later revision on the same tick. The movement batch is **not rewritten** to pretend it happened after the resource transition. This is compatible with the current presentation guard because the next movement tick is still previous movement tick + 1 and its revision is strictly greater than every earlier mutation.

Pin the ordering contract with protocol tests:

- movement succeeds, then post-movement Consume advances revision without advancing tick;
- the resource projection immediately after that call may have the same tick and a higher revision than the returned movement batch;
- a revision-only resource transition between two locomotion ticks does not make the next movement batch rejected;
- ordinary autonomous Consume refusal never changes an already successful movement result into a protocol-level locomotion failure.

### Architecture

#### Composition and observation owner

The first **landed vertical M2 slice** must replace `protocol::Simulation`'s hardcoded two-actor fixture with the minimum World-owned composition/observation owner required by this scenario. Native-only M2.1/M2.2 work may exist locally beforehand, but it must not land as hidden product depth that preserves the old application fixture:

- World/content owns which entities exist (actors, households, places) and their IDs;
- session/control binding still chooses which existing actor is human-controlled;
- observation is a bounded set computed per read, not a stored registry and not a growing list of named fields on `protocol::Simulation`;
- RestNeed decision collection is “actors that currently have `RestNeedState`”, not `living_need_npc_` as a permanent protocol field.

Composition is **code-defined concrete records in Simulation Core** (plain C++ constants/builder for the acceptance village), not a data file, resource format, or scenario DSL.

The observed set stays **actor-scoped**: it is the actors in the bounded acceptance composition, computed per read in `src/protocol` rather than stored. Current `World` exposes only per-id actor reads and no typed enumeration, so M2 admits two bounded read-only Core enablers required by real reads: deterministic actor ids for observation/decision collection and deterministic household ids for village resource discovery. `snapshot()` must **not** be used as either enumeration API because it copies authoritative state by value and would turn hot application/read paths into serialization-shaped work. Places are discovered through household/store and field-assignment references; no global place registry/query is exposed to presentation in M2.

Households and places are **not** observed entities and must not enter `ObservedWorldProjection`, because `WorldPresentation` instantiates an actor presentation scene for every non-controlled observed id. They reach Godot only through purpose-built resource projections. No kind discriminator is added to the observed projection this milestone, and the observed set is deliberately not snapshot truth.

Per-tick movement intent collection is generic over the bounded actor-id view: controlled binding supplies the controlled actor's requested intent, actors with `RestNeedState` receive their deterministic RestNeed movement, and otherwise exact-spatial actors receive an idle intent so they still produce authoritative samples/materialize without a feature-named branch. Before population scale makes that bounded full actor pass inappropriate, re-admit the collection strategy under `PERFORMANCE.md`; do not prematurely add ECS/dirty scheduling now.

This is not permission to add ECS, a scenario DSL, GOAP, or a generic entity framework. Concrete household/place/actor records plus a small observation set are enough.

Actors, households, and places share one `EntityId` identity space, and IDs are unique across kinds. Do **not** introduce an id allocator or counter: `EntityId` values are caller-supplied today (`src/sim/types.hpp`) and `spawn_actor` validates identity, spatial state, locomotion capability and rest need before rejecting duplicates, so M2 adds World-enforced cross-kind uniqueness that refuses a duplicate the same way `spawn_actor` returns `duplicate_entity`, plus the same duplicate/identity validation on restore. A hidden mutable counter would become new snapshot truth under ADR 0008 for no current need. A household is **not** an `ActorState` and must not be spawned through `spawn_actor` merely to hold stock.

A store or field is a World **place record** with its own `EntityId`, assigned X/Z, and per-axis occupancy tolerance (the RestNeed occupancy pattern). Households and the field **reference** those place ids; occupancy coordinates are not duplicated as ad-hoc literals on protocol feature branches. Snapshot captures the place records.

Composition/restore validation is referential, not only structural: household store ids resolve to place records; member ids resolve to actors; an actor appears in at most one household; standing-pledge destinations resolve to a different household when that state is introduced; field destinations resolve to households; carry values satisfy capacity; consume/work content satisfies the positivity/budget invariants required for enabled actions. Invalid references or duplicate membership reject the composition/restore atomically rather than surviving until the first gameplay command.

#### Authority and layers

```
Godot intent (gift / pledge execution / work / draw)
  -> protocol validation/session binding
  -> src/sim resource rules
  -> WorldRevision++
  -> purpose-built projections
  -> GDExtension translation
  -> localized Godot UI

```

Forbidden: Godot-owned stock/money, `is_player` economy branches, setters such as `SetStock`, scene nodes creating households, or RestNeed flags reused as food/economy state.

`src/sim` and `src/protocol` remain Godot-free. Adapter translates only.

#### Snapshot (ADR 0008)

New authoritative state must join `WorldSnapshot` with a schema-version bump:

- households (id, members, stock, threshold, store place, remaining consume budget, positive content-owned consume amount);
- actor `carried_grain` and `grain_carry_capacity`;
- standing transfer pledges when that path is introduced (remaining amount + durable `destination_household`);
- field work assignment when introduced (durable `destination_household`, positive fixture yield, `remaining_work_completions`);
- production/store place records needed by implemented decisions.

Every content value that decides a transition outcome must be captured, because `restore` builds a fresh `World` and moves it over the current one: anything absent from the snapshot is destroyed rather than preserved.

Derived shortage, actor→household reverse membership, the observed set, projections, and Godot nodes are not snapshot truth. Restore remains atomic and rejects unknown/invalid schemas and invalid cross-references. Derived indexes are rebuilt. Each bounded task bumps the schema only for authoritative state it actually lands; no future M2 placeholders are persisted in advance.

Note: ADR 0008 tracks the executable snapshot schema. Current source wins over any stale schema-version sentence.

#### Spatial coupling

Reuse the proven occupancy pattern (assigned X/Z + per-axis tolerance, exact `SpatialState` required). Do not admit a navigation/physics library, body collision, or production spatial index for M2. Work, consume, gift, and transfer fail closed if the actor lacks exact spatial state or is outside tolerance.

Be precise about \*which\* tolerance, because current source contains two different readings. Resource transitions use **arrival semantics**: per-axis unsigned distance against the place's assigned X/Z compared to the place tolerance, with no body radius — the same reading that makes the M1 NPC “arrived”, which matters because the short household's store coincides with its rest point. The existing `is_planar_position_occupied_by_other_actor` predicate answers the opposite question (it excludes the acting actor and adds a body radius), so M2 admits one small **self-inclusive** Core predicate: “actor X is inside place P”. The tolerance value itself stays an acceptance fixture, tunable for reachability without changing the semantics.

Occupancy is required but **not exclusive**. Several actors may stand inside the same store tolerance and each may act. Because the short household's store is the M1 NPC's rest point (decision 8), a player standing there to `Gift` makes that NPC's derived rest status `blocked` through the existing `World::is_planar_position_occupied_by_other_actor` rule. That is the accepted Milestone 1 interference mechanic, not a defect: rest satisfaction stays derived, `Consume` and `Gift` must not add an exclusivity rule to suppress it, and acceptance evidence must show the interaction rather than weakening `rest_interference`.

Places may exist authoritatively without being Godot-unique meshes beyond what presentation needs to make the loop readable.

#### Integer quantities (MODELING.md quantity default / ADR 0007 boundary range)

Grain units, thresholds, carry/capacity, pledged amounts, consume budgets, work yields, and remaining work completions are integers, per `MODELING.md`'s integer/scaled-integer default for load-bearing quantities. Core additions use checked arithmetic. ADR 0007 governs the Godot/protocol signed boundary: export uses the existing checked conversion, and an operation whose required protocol result cannot be represented fails closed instead of wrapping or clamping.

#### Performance constraints (`docs/PERFORMANCE.md`)

Performance is part of correctness here, so M2 must not smuggle in per-frame or full-world work:

- derived shortage is computed over the acceptance village's bounded household set, never by scanning the whole world, and never once per frame per household;
- post-locomotion NPC consume policy reuses the bounded acceptance actor collection and evaluates only relevant member actors with exact `SpatialState`; it proposes nothing when budget is zero or Consume is currently infeasible, and it must not add a second independent world scan;
- resource reads cross the bridge as bounded per-village/per-household projections, never one cross-language call per entity per frame and never full-world state;
- the shortage/carry/pledge HUD refreshes on a decimated cadence (the debug HUD's existing 0.25 s gate is the reference). Treat this as a **new** requirement rather than an existing pattern: the current living-need projection is read on every physics tick, and the village-scoped M2 reads must not copy that;
- no resource transition may add a synchronous multi-millisecond job to the Godot main thread.

Because FR8 adds bounded application work around the 60 Hz locomotion path and FR11 adds new bridge reads, acceptance must include one bounded measurement of locomotion-call cost (including post-movement Consume policy) and bridge cost with the village loaded, compared against the existing `PERFORMANCE.md` budgets. A later increment that needs more than this needs measurement, not an assumption.

### Protocol and presentation

Add the smallest purpose-built commands/results/projections that answer presentation questions. Do not export `World` or a universal DTO.

Required reads (names are semantic; the bounded implementation task binds them to source):

- a **village-scoped** shortage/household-resource projection: the acceptance village's households in deterministic order, each with household id, stock, threshold and derived short/adequate status, plus tick, revision and protocol version. Each entry also carries its member actor ids, so presentation can distinguish the player's own household from a neighbour's without an id-to-label map. This is the **discovery read** that the first vertical checkpoint lands: Godot learns household identities from it instead of holding literals, and no per-household id has to be known in advance;
- an inventory/carry projection for the controlled actor: carried grain, optional member-household stock if the actor is a member;
- a transfer-pledge projection for the surplus household: remaining pledged grain-units and the destination household, not mutable shop state and not actor inventory.

Required writes: semantic gift, transfer-pledge execution, work, and draw/deposit intents. **No M2 command carries a client-supplied quantity**: every transition moves a rule-defined amount (household consume amount, carry-capacity draw, whole carried amount for deposit and gift, whole remaining pledge, fixture work yield), so the UI needs no amount widget and the refusal taxonomy needs no amount errors. Simulation still revalidates membership, occupancy, availability/budgets, destination validity, checked arithmetic, and carry bounds.

Additive protocol change is preferred. Existing movement/need projections keep their meaning. Godot HUD/inventory/pledge UI uses design-system layout and gettext keys (`UI_*`), locales `ru` (default) and `en`. Core/protocol emit no localized gameplay strings. New `UI_*` keys ship in the same increment as the widget that references them, because the localization gate fails on catalog keys that no scene or script uses.

Debug/playtest JSON remains machine-oriented; on-screen labels are localized.

Because the first vertical checkpoint replaces the hardcoded composition fixture, Godot playtest scenarios must resolve the entities and places they target from authoritative projections instead of keeping literals such as `main.gd`'s `LIVING_NEED_NPC_ENTITY_ID := 2` and `REST_TARGET_M`. Decision 19 makes the rest point/store coordinate a single composition constant, so the client must not keep its own copy. A scenario may hold a scenario-scoped id or coordinate only after reading it from a projection.

The first vertical checkpoint also fixes startup read order. Presentation initialization rejects a controlled-spatial projection whose `tick`/`revision` do not match values stored from the observed-world read. Therefore initial village discovery, observed-world read and controlled-spatial initialization must be taken before runtime advancement and from one unchanged revision. Once runtime starts, resource projections are allowed to be later revisions at the same simulation tick; startup equality must not be generalized into a rule that all projections always share one revision.

### Magic-sensitive assumptions

Record in the mechanic model, do not implement magic:

- grain exists only through stock, transfer, or labor at the field (no creation ex nihilo);
- labor and land occupancy are mundane bottlenecks;
- stores preserve grain perfectly for this slice (spoilage/pests absent);
- transport is carry-by-actor over local tolerances, not carts/roads;
- no office extracts tithe/rent yet.

Future magic that changes yield, preservation, transport, or labor must hit these same stocks and rules rather than a parallel mana-economy.

### Non-goals

Out of this Epic:

- extending RestNeed into hunger, fatigue, sleep duration, or a generic needs/planner framework;
- currency, credit, prices-as-market-engine, multiple item types, containers, equipment, or a generic inventory ontology;
- institutions, lordship, tenure, wages, taxes, reputation, dialogue, combat, magic;
- independent simulation-time systems, schedulers, time acceleration, seasonal calendars;
- production navigation/physics engines;
- LLM/external NPC policy;
- disk save/load (Milestone 7); Core snapshot coverage is required, product saves are not;
- breaking or weakening Milestone 1 `rest_interference` / `offscreen` evidence.

### Bounded implementation plan inside the milestone

M2 is one product milestone, but it is **not one implementation task**. Use the following causal boundaries. A task is complete only for its own declared surface; do not begin the next one before review/continuation. Native-only tasks are deliberately short and are not standalone product acceptance.

#### M2.0 — Contract closure (this specification)

No production code. Freeze the first-resource invariants before implementation: membership cardinality, carry capacity ownership, refusal/no-op semantics, checked arithmetic, bounded Work, pledge authorization, post-locomotion Consume ordering, snapshot obligations, acceptance-village context, and landing policy. Output is an implementation-ready target contract, not a new generic economy design.

#### M2.1 — Core composition foundation (native-only development task)

Add only the minimum Core types/validation needed to represent actors + households + store places with one shared `EntityId` space, single-household membership, cross-kind uniqueness, deterministic actor/household id views, referential validation, and snapshot/restore. Do **not** add carry, Gift, Work, pledge, market, or Godot resource UI. Focused native tests + relevant sanitizer/self-review.

This task may be developed/committed locally or on the M2 feature branch, but by default it is **not merged to `main` alone** merely because structs/tests are green. It is a bounded enabler for the first vertical M2 slice.

#### M2.2 — Core stock + Consume + shortage law (native-only development task)

Add household stock/threshold/store, positive consume amount, bounded consume budget, derived shortage, actor-inside-place predicate, Consume validation/refusal/revision semantics, and snapshot/determinism coverage. Still no carry, Work, pledge, resource UI, or client command. Prove shortage can emerge from the configured bounded Consume sequence in headless Core.

Again, this is local/native evidence, not a player-facing landing by itself.

#### M2.3 — Application composition + autonomy/order integration

Wire the acceptance village into `protocol::Simulation` without named actor feature fields: controlled binding + generic actor collection, RestNeed movement or idle sample collection, deterministic post-locomotion Consume policy, actor-scoped observed world, and household discovery read. This is the first task that touches the existing protocol/presentation composition contract, so existing protocol and relevant Godot M1 regressions must be exercised even though no new resource widget exists yet. Pin the same-tick/later-revision ordering contract.

M2.1–M2.3 are one admission-critical foundation sequence; if maintained as separate local commits, they should normally land together with M2.4 rather than leaving `main` with invisible simulation depth.

#### M2.4 — First vertical checkpoint: autonomous shortage

Close the real first M2 slice:

```text
acceptance village composition
  -> locomotion / post-movement NPC Consume
  -> authoritative household stock + derived shortage
  -> village resource projection
  -> GDExtension translation
  -> localized RU/EN shortage feedback
  -> bounded real-client scenario + M1 regressions
```

This is the first M2 **landing boundary** and the first point at which the new system counts as player-visible product progress.

#### M2.5 — Core carry + Draw/Deposit/Gift (native-first)

Add actor carried grain/capacity and the three shared World laws with conservation, own-household Gift refusal, empty/full/no-op refusal, checked addition and snapshot/determinism tests. No generic inventory/item ontology. Keep this native-first task short; do not implement Work or pledge here.

#### M2.6 — Gift vertical checkpoint

Expose controlled-actor carry/member-household projection plus Draw/Deposit/Gift semantic commands, adapter translation and the smallest Godot affordance needed to draw and gift at stores. Prove authoritative stock/carry change and the expected M1 rest interference when gifting at the short-household store.

#### M2.7 — Core bounded Work (native-first)

Introduce the field place/work assignment only now: durable destination household, positive fixture yield, bounded remaining work completions, occupancy/refusal/overflow rules, snapshot/determinism and NPC parity at World-rule level. Do not add schedules, wages, tenure, tools, skills, crop calendars or refill.

#### M2.8 — Work vertical checkpoint

Expose Work command/result and field presentation cue/feedback. Prove one allowed labor completion increases the configured short-household stock and exhausts/decrements authoritative work availability exactly as modeled.

#### M2.9 — Core standing household transfer (native-first)

Introduce the standing pledge only now: remaining quantity, durable destination, source-household-member authorization, all-or-nothing execution, insufficient-stock/zero-pledge/overflow refusal, snapshot/determinism and actor parity. No trade/exchange naming or second commodity.

#### M2.10 — Transfer/pledge vertical checkpoint

Expose the pledge projection and semantic execution command, adapter translation and read-only pledge/inventory presentation. Prove the one-way surplus→short household transfer changes authoritative stocks and clears the pledge.

#### M2.11 — Whole-M2 closure

Run the cross-path permutation claim, NPC parity through Gift/Work/Transfer at native level, offscreen Consume, snapshot continuation, sanitizer, architecture/localization checks, bounded performance measurement, RU+EN real-client evidence, and unchanged M1 semantic regressions. Update canonical model/verification/roadmap/index/ADR owners only for behavior actually landed. No new mechanic is introduced here.

#### Local verification / CI policy

Local checks are the working loop. Native-first tasks use focused native tests, the smallest broader native gate justified by risk, and sanitizer where new state/restore/memory-sensitive paths warrant it. **Do not use GitHub CI as an inner-loop test runner.** CI is independent evidence for a pushed candidate revision when a landing/PR actually needs it. Godot is required when a task changes the Simulation↔Godot boundary, existing presentation composition, localization/player-visible behavior, or reaches a vertical checkpoint; pure Core tasks do not run Godot merely for ritual.

### Acceptance criteria / критерии приёмки

Whole-Epic acceptance (from ROADMAP):

\> The village can develop a resource problem without the player, and the player can address it through more than one real path while resource transfers remain authoritative Simulation state.

Evidence follows `docs/VERIFICATION.md`:

- native tests: referential composition validation and single-household membership; consume/gift/transfer/work validation; empty/full/exhausted/no-op refusal; checked-overflow refusal; atomic refusal; derived shortage and bounded shortage onset; consume-budget exhaustion; carry-capacity invariant; bounded work exhaustion; source-member pledge authorization; `Gift` refusal into the actor's own household; cross-kind duplicate `EntityId` rejection; store-place/rest-point coincidence; actor parity driving an NPC actor through gift/transfer/work over the same rules; snapshot/restore of every landed authoritative content value with deterministic household/place/pledge ordering; an ADR 0008 determinism check that equal restored state plus equal operations yields equal subsequent snapshots while driving `Consume` and `Work` after restore so missing content cannot pass silently; RestNeed regression;
- protocol tests: projections derived from World; commands cannot set stock or quantities; generic actor collection replaces named living-need fields; a successful locomotion batch may be followed by post-movement Consume at the same tick and a later revision; ordinary autonomous Consume refusal does not invalidate the movement result; a resource transition between two locomotion ticks does not make the next movement batch rejected by the presentation guard; player-submitted gift/transfer/work commands prove revision-without-tick;
- sanitizer on native resource/snapshot paths;
- architecture check still forbids Godot in `src/sim` / `src/protocol`;
- localization check for new UI keys;
- one bounded performance measurement of the locomotion/application call and bridge cost with the acceptance village loaded, compared against the `PERFORMANCE.md` budgets;
- bounded Godot scenarios on the real client: shortage appears without player economic input; gift, household transfer, and work each change authoritative stock and visible HUD; gifting inside the short household store shows the expected M1 rest interference instead of suppressing it; offscreen NPC consume still mutates household stock; RU+EN smoke still pass; existing `rest_interference` and `offscreen` scenarios remain green.

Do not claim M2 accepted from compile-green or from a Godot-only stock label.

### Definition of done / критерии готовности

This Epic is done only when all of the following are true for the same revision:

- FR1–FR15 are implemented in the owning layers (`src/sim` for world rules, `src/protocol` for commands/projections, GDExtension for translation, Godot for intent/UI only).
- The ROADMAP M2 sentence is proven by actually executed native, protocol, sanitizer, architecture, localization, and bounded Godot evidence listed under Acceptance criteria — not by compile-green or unrun playtests.
- Gift, household transfer, and work are each proven as distinct real paths that change authoritative stock and visible localized feedback.
- Milestone 1 regression scenarios are green on that revision.
- Canonical docs listed below match implemented behavior; the mechanic contract lives in `docs/models/household-resource.md`.
- Non-goals were not smuggled in (no currency engine, needs framework, scheduler, or Godot-owned stock).
- Residual risks are explicit. Unverified claims are not marked accepted.

### Documentation to keep current

When behavior lands, update the canonical owners rather than copying status everywhere:

- `docs/models/household-resource.md` — mechanic contract following the model template in `MODELING.md`;
- `docs/ROADMAP.md` — milestone status only; when M2 docs update, change the scope bullet from “trade, gift or work” to **gift, household transfer, and work** so it matches this Epic, restate the “shop/inventory presentation” bullet as inventory/pledge presentation, and record that priced exchange is deliberately deferred to the milestone that introduces a second good or coin;
- `docs/VERIFICATION.md` — new proof obligations/scenarios, including an explicit **Milestone 2 acceptance gate** section beside the existing Milestone 1 gate, because the repository currently defines a milestone gate only for M1;
- `docs/INDEX.md` — route to the new model;
- `docs/MODELING.md` — short stable pointer, not a second spec;
- ADR 0008 — reconcile its snapshot content list with executable state whenever an M2 task actually bumps the schema: fix the stale “schema version **3**” sentence and actor-field enumeration first, then add only the authoritative household/carry/pledge/place/work state that has landed at that revision. Do not pre-document future fields and do not turn the ADR into a status log.

### Accepted decisions / принятые решения

These are product/architecture choices implicit above. Reject or edit them **before approval** if they are wrong:

1. **Single staple is grain-units; no coin in M2.** The third path is a surplus→short household transfer executed from a standing pledge, not grain-for-grain into actor carry and not a currency. Historical markets/money exist in the baseline, but M2 keeps one resource to avoid a fake currency — and therefore does not claim to implement priced exchange.
2. **Controlled actor is a member of the surplus household** so gift/draw have a real source without spawning a third household.
3. **The Milestone 1 RestNeed NPC becomes a member of the short household** rather than adding a fourth actor for consume.
4. **Resource actions never advance `SimulationTick`.** Independent time stays closed unless this spec is revised. Immediate `Work` yield is a fixture exception to `MODELING.md`'s SimulationTick production default; it does not introduce an economy clock.
5. **Occupancy tolerances, not dialogue or UI confirmation, gate physical transfers.** Presentation may still ask the player to submit the intent.
6. **Transfer destination is a durable content `destination_household` `EntityId`** (the short household in the acceptance village). The executing actor is not the grain sink. Execution is all-or-nothing for the whole remaining pledge, with no client-supplied amount. Do not rebind from live shortage.
7. **Acceptance-village `Work` yield goes to a durable field `destination_household` `EntityId`** (the short household). Same non-rebinding rule.
8. **Short-household store and the M1 NPC's rest point coincide**, built from **one composition constant** so `Consume` and RestNeed coexist on the M1 NPC. `RestNeedState` keeps its Milestone 1 shape (raw `rest_x` / `rest_z` / `axis_arrival_tolerance`, no place reference): the store place record and the NPC's rest need are constructed from the same constant, and a native test pins that they coincide. Refactoring `RestNeedState` to reference a place `EntityId` would change M1 authoritative state, `decide_npc_rest_need`, restore validation and `docs/models/living-need.md`, so it is deliberately deferred until a second need or place-driven behavior requires it.
9. **`EntityId` uniqueness across actors, households, and places**; households are not fake-actors; stores/fields are place records.
10. **Gift occupancy is the receiving household store only** — no separate hand-off place.
11. **NPC `Consume` is applied by deterministic policy in the composition/application path after successful locomotion.** Policy evaluates post-movement authoritative state, invokes the same World rule as any actor source, and an ordinary Consume refusal does not roll back or fail the committed locomotion batch.
12. **`Consume` is bounded by a content-owned consume budget** that is snapshot truth, so reusing the per-locomotion-tick decision path cannot become a recurring economy clock.
13. **No `EntityId` allocator.** One shared identity space with composition-supplied ids and World-enforced cross-kind uniqueness.
14. **The M2 third path is a one-way household transfer, not a trade.** `trade`/`buy`/`sell`/`price`/`shop` naming is reserved for the later milestone that implements real exchange.
15. **Occupancy is non-exclusive**, so gifting at the short household store legitimately triggers the M1 rest-interference mechanic instead of being suppressed.
16. **No client-supplied quantities in M2.** Every transition moves a rule-defined amount: household consume amount, carry-capacity `Draw`, whole carried amount for `Deposit` and `Gift`, whole remaining pledge, fixture work yield.
17. **The observed set stays actor-scoped and is derived in `src/protocol` from World queries.** Simulation Core owns which entities exist; it does not own presentation interest. Households and places are not observed entities and reach Godot only through purpose-built resource projections.
18. **M2 NPC economic parity is proven by shared World rules plus native tests**, with in-client NPC autonomy limited to short-household `Consume`. No NPC pledge or work policy runs in the client this milestone.
19. **The store place and the M1 rest point are built from one composition constant** and `RestNeedState` keeps its Milestone 1 shape; the place-reference refactor is deferred.
20. **The player observes a neighbour's shortage.** Decision 2 keeps the controlled actor in the surplus household so gift and draw have a real source, so the shortage HUD reports the short household rather than the player's own.
21. **M1 scenario fixture constants are rebaselined, not weakened.** A legitimately changed composition changes entity sets and revision numbers; the assertions keep their semantics.
22. **Bounded typed discovery views only:** deterministic-order actor ids support observation/application collection and deterministic-order household ids support the village resource discovery read. `snapshot()` is not an enumeration API; places are reached through authoritative references rather than a presentation-facing place listing.
23. **`Gift` refuses a household the actor belongs to**; that case is `Deposit`.
24. **Resource occupancy uses arrival tolerance, not body-radius occupancy**, and admits one self-inclusive “actor inside place” predicate.
25. **Stranding the pledge by repeatedly drawing and gifting is an accepted authoritative consequence**, not a bug to hide with a refill; acceptance only claims each path once in any permutation.
26. **Household membership is single-valued in M2.** Household member lists are persisted truth; an actor belongs to zero or one household, and a reverse lookup is derived only.
27. **Carry capacity is authoritative actor state and snapshot truth.** M2 does not use a hidden global/client carry bound.
28. **Standing pledge execution is authorized to source-household members only.** Occupancy alone does not let another household spend the source stock.
29. **Work is bounded while independent simulation time is closed.** Field assignments carry positive yield plus non-negative remaining completions; each accepted Work consumes one completion, so revision-only Work cannot manufacture unlimited grain.
30. **Accepted resource transitions always mutate and increment revision exactly once; ordinary refusal mutates nothing and increments neither tick nor revision.** Empty/full/exhausted cases are typed refusal, not no-op success.
31. **Autonomous Consume is post-locomotion application work.** Movement commits first; post-movement Consume can advance revision again at the same tick, and resource projections may therefore be newer than the returned movement batch.
32. **Impossible autonomous Consume is not proposed repeatedly.** Zero budget, insufficient stock, or missing occupancy suppresses the policy attempt until authoritative state makes it feasible again.
33. **Historical scope is explicit but bounded.** The acceptance village uses an England-circa-1200 inland agrarian reference envelope; pledge/work quantities remain gameplay fixtures, and tenure/dues/market/jurisdiction are deliberately absent rather than silently generalized.
34. **Native-first tasks are implementation boundaries, not product milestones.** Pure Core work is verified locally and does not by itself justify M2 acceptance or hidden simulation depth on `main`; vertical checkpoints remain the landing/product boundaries.

If any of these should change, revise the specification version rather than implementing a silent fork.