# ADR 0008 — Core snapshot/restore determinism contract

Status: ACCEPTED

## Context

Full save/load remains a later product milestone, but the Simulation Core already owns durable identity, seed, simulation tick, world revision, actor state, optional exact spatial state, fixed-step locomotion continuation that affects subsequent authoritative movement, the first Milestone 1 actor need state, the landed Milestone 2 household/place/resource, actor grain-carry and bounded field-work assignment state, and the first Milestone 3 remembered-material-aid social state. As more causal systems are added, waiting until the final persistence milestone to prove that authoritative state is fully capturable would make hidden runtime state and non-restorable caches increasingly expensive to discover.

The current `World` also maintains derived lookup/index and deterministic id-view structures for actors, households and places. Those structures are runtime conveniences: persisting them would couple snapshots to one storage implementation and would incorrectly elevate caches/indexes into world truth.

## Decision

1. Simulation Core exposes a versioned in-memory `WorldSnapshot` value contract.
2. Snapshot schema version **10** contains the current authoritative `World` state: seed, `SimulationTick`, `WorldRevision`, actor states, place records, household records in deterministic insertion order, optional bounded `FieldWorkAssignmentState`, each household's `StandingTransferPledge`, and each household's bounded remembered-material-aid actor reference.
3. Actor snapshot state includes every field currently owned by `ActorState`: the Milestone 0 bootstrap position, optional `SpatialState`, `ActorLocomotionCapability`, optional `RestNeedState`, `ActorGrainCarryState`, and `GroundedLocomotionContinuation`.
4. `GroundedLocomotionContinuation` is snapshot truth rather than a cache because its fractional integration remainders and fixed tick-rate provenance change the result of the next authoritative locomotion tick. An actor without exact `SpatialState` must have pristine locomotion continuation.
5. `RestNeedState` is snapshot truth because its assigned local rest point and arrival tolerance change future NPC decisions. Need satisfaction itself is derived from restored `SpatialState`; no separate completion Boolean is persisted.
6. `ActorGrainCarryState` is snapshot truth because carried grain and carry capacity change the result of later Draw, Deposit and Gift transitions. Both quantities are non-negative and carried grain may not exceed capacity.
7. Place snapshot state includes stable `EntityId`, assigned local X/Z and per-axis occupancy tolerance. Household snapshot state includes stable `EntityId`, member actor IDs, the referenced store-place `EntityId`, non-negative grain stock, non-negative shortage threshold, positive per-consume grain amount, remaining bounded consume budget, the standing transfer pledge (durable destination-household `EntityId` plus non-negative remaining pledged grain), and one bounded remembered-material-aid actor reference (`EntityId{0}` for none, otherwise an existing actor).
8. `FieldWorkAssignmentState` is snapshot truth because its referenced work-place `EntityId`, durable destination-household `EntityId`, positive fixture grain yield and remaining bounded work completions determine whether and how later Work succeeds. It is bounded content state, not an entity kind, scheduler or derived cache.
9. Household shortage is derived from restored authoritative quantities (`stock < threshold`) and is never snapshot truth as a separate Boolean.
10. Actor, household and place IDs share one identity space. Restore validates cross-kind uniqueness, valid household member references, valid store-place references, duplicate members, the invariant that one actor belongs to at most one household, actor carry bounds, the landed household resource invariants, remembered-material-aid state/reference validity, field-work assignment validity with resolvable work-place and destination-household references, and standing-pledge destination validity (existing other household, non-negative remaining quantity) before replacing the world.
11. Derived runtime structures such as actor/place/household lookup maps and deterministic id-only views are never snapshot truth. Restore rebuilds them from the ordered authoritative records.
12. Restore validates schema version, positive entity identity, spatial-state validity, locomotion-capability validity, rest-need validity, grain-carry validity, locomotion-continuation validity and composition/resource/social/work references before replacing the current world.
13. Restore is atomic with respect to validation: malformed snapshots leave the target `World` unchanged.
14. Successful restore preserves seed/tick/revision exactly and does not advance simulation time or revision merely because loading occurred.
15. Equal restored state plus equal subsequent authoritative operations must produce equal subsequent snapshots. Native tests prove this for existing world operations, fractional fixed-step locomotion continuation, the first rest-need state, landed composition state, bounded household Consume continuation, Draw/Gift continuation over actor carry state, bounded Work continuation over field assignment state, standing household-transfer continuation over pledge remaining quantity, the Milestone 2 cross-path permutation claim (`AcceptanceVillagePermutation.GiftWorkAndTransferEachSucceedOnceInAnyOrder`), and the Milestone 3 remembered-aid state created atomically by a qualifying Gift.
16. Unsupported schema versions are rejected. Earlier versions cannot represent all current authoritative state; this bounded proof still does not introduce migration policy.

## Not decided here

`WorldSnapshot` is not a disk or network format. Full persistence still needs a versioned outer envelope, serialization/IO, content version, protocol/schema compatibility policy, migration strategy, corruption handling, atomic file replacement and product-facing save-slot behavior when Milestone 7 is implemented.

No JSON, binary codec, database, event sourcing, replay-log format or generic serialization framework is selected by this decision.

## Consequences

- Adding new authoritative `World` state creates an explicit review obligation: it must be captured/restored by the snapshot contract or be demonstrably derived from captured state.
- Fractional fixed-step state cannot disappear across restore and subtly change later movement.
- Actor need state that changes future decisions cannot be recreated from presentation or session defaults after restore.
- Actor carried grain/capacity, household membership, store-place identity, grain stock, shortage threshold, bounded consumption content and standing-transfer remaining quantity/destination cannot be reconstructed from protocol/Godot fixtures after restore.
- The remembered-material-aid actor cannot be reconstructed from a UI message, scenario flag or later inference; it is persistent causal social state and must survive restore exactly.
- Field work-place identity, destination household, positive yield and remaining work completions cannot be reconstructed from client/scenario defaults after restore.
- Derived shortage remains recomputable rather than becoming a second mutable truth.
- Runtime caches/indexes/id views can change implementation without changing persisted world truth.
- Deterministic continuation and referential/resource/social integrity are tested before economy/social/political systems make persistence gaps harder to isolate.
- The eventual save format can serialize a stable Core value model rather than reverse-engineering live object internals.
- Milestone 7 remains responsible for real cross-session persistence; this ADR is only the minimum Core feasibility proof that makes that later work safer.
