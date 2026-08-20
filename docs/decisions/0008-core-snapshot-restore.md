# ADR 0008 — Core snapshot/restore determinism contract

Status: ACCEPTED

## Context

Full save/load remains a later product milestone, but the Simulation Core already owns durable identity, seed, simulation tick, world revision, actor state, optional exact spatial state, fixed-step locomotion continuation that affects subsequent authoritative movement, the first Milestone 1 actor need state, and the first Milestone 2 composition records for households and places. As more causal systems are added, waiting until the final persistence milestone to prove that authoritative state is fully capturable would make hidden runtime state and non-restorable caches increasingly expensive to discover.

The current `World` also maintains derived lookup/index and deterministic id-view structures for actors, households and places. Those structures are runtime conveniences: persisting them would couple snapshots to one storage implementation and would incorrectly elevate caches/indexes into world truth.

## Decision

1. Simulation Core exposes a versioned in-memory `WorldSnapshot` value contract.
2. Snapshot schema version **5** contains the current authoritative `World` state: seed, `SimulationTick`, `WorldRevision`, actor states, place records and household records in deterministic insertion order.
3. Actor snapshot state includes every field currently owned by `ActorState`: the Milestone 0 bootstrap position, optional `SpatialState`, `ActorLocomotionCapability`, optional `RestNeedState`, and `GroundedLocomotionContinuation`.
4. `GroundedLocomotionContinuation` is snapshot truth rather than a cache because its fractional integration remainders and fixed tick-rate provenance change the result of the next authoritative locomotion tick. An actor without exact `SpatialState` must have pristine locomotion continuation.
5. `RestNeedState` is snapshot truth because its assigned local rest point and arrival tolerance change future NPC decisions. Need satisfaction itself is derived from restored `SpatialState`; no separate completion Boolean is persisted.
6. Place snapshot state includes stable `EntityId`, assigned local X/Z and per-axis occupancy tolerance. Household snapshot state includes stable `EntityId`, member actor IDs and the referenced store-place `EntityId`. Resource quantities are not part of schema v5 because they have not landed yet.
7. Actor, household and place IDs share one identity space. Restore validates cross-kind uniqueness, valid household member references, valid store-place references, duplicate members and the invariant that one actor belongs to at most one household.
8. Derived runtime structures such as actor/place/household lookup maps and deterministic id-only views are never snapshot truth. Restore rebuilds them from the ordered authoritative records.
9. Restore validates schema version, positive entity identity, spatial-state validity, locomotion-capability validity, rest-need validity, locomotion-continuation validity and composition references before replacing the current world.
10. Restore is atomic with respect to validation: malformed snapshots leave the target `World` unchanged.
11. Successful restore preserves seed/tick/revision exactly and does not advance simulation time or revision merely because loading occurred.
12. Equal restored state plus equal subsequent authoritative operations must produce equal subsequent snapshots. Native tests prove this for existing world operations, fractional fixed-step locomotion continuation, the first rest-need state and the landed composition state.
13. Unsupported schema versions are rejected. Earlier versions cannot represent all current authoritative state; this bounded proof still does not introduce migration policy.

## Not decided here

`WorldSnapshot` is not a disk or network format. Full persistence still needs a versioned outer envelope, serialization/IO, content version, protocol/schema compatibility policy, migration strategy, corruption handling, atomic file replacement and product-facing save-slot behavior when Milestone 7 is implemented.

No JSON, binary codec, database, event sourcing, replay-log format or generic serialization framework is selected by this decision.

## Consequences

- Adding new authoritative `World` state creates an explicit review obligation: it must be captured/restored by the snapshot contract or be demonstrably derived from captured state.
- Fractional fixed-step state cannot disappear across restore and subtly change later movement.
- Actor need state that changes future decisions cannot be recreated from presentation or session defaults after restore.
- Household membership and store-place identity cannot be reconstructed from protocol/Godot fixtures after restore.
- Runtime caches/indexes/id views can change implementation without changing persisted world truth.
- Deterministic continuation and referential integrity are tested before economy/social/political systems make persistence gaps harder to isolate.
- The eventual save format can serialize a stable Core value model rather than reverse-engineering live object internals.
- Milestone 7 remains responsible for real cross-session persistence; this ADR is only the minimum Core feasibility proof that makes that later work safer.
