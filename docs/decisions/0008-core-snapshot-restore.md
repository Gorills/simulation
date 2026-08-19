# ADR 0008 — Core snapshot/restore determinism contract

Status: ACCEPTED

## Context

Full save/load remains a later product milestone, but the Simulation Core already owns durable identity, seed, simulation tick, world revision, actor state, optional exact spatial state, fixed-step locomotion continuation that affects subsequent authoritative movement, and the first Milestone 1 actor need state. As more causal systems are added, waiting until the final persistence milestone to prove that authoritative state is fully capturable would make hidden runtime state and non-restorable caches increasingly expensive to discover.

The current `World` also maintains an `EntityId -> actor index` lookup for performance. That index is derived runtime structure: persisting it would couple saves to one storage implementation and would incorrectly elevate cache/index state into world truth.

## Decision

1. Simulation Core exposes a versioned in-memory `WorldSnapshot` value contract.
2. Snapshot schema version **3** contains the current authoritative `World` state: seed, `SimulationTick`, `WorldRevision`, and actor states in deterministic insertion order.
3. Actor snapshot state includes every field currently owned by `ActorState`: the Milestone 0 bootstrap position, optional `SpatialState`, optional `RestNeedState`, and `GroundedLocomotionContinuation`.
4. `GroundedLocomotionContinuation` is snapshot truth rather than a cache because its fractional integration remainders and fixed tick-rate provenance change the result of the next authoritative locomotion tick. An actor without exact `SpatialState` must have pristine locomotion continuation.
5. `RestNeedState` is snapshot truth because its assigned local rest point and arrival tolerance change future NPC decisions. Need satisfaction itself is derived from restored `SpatialState`; no separate completion Boolean is persisted.
6. Derived runtime structures such as `actor_index_by_id_` are never snapshot truth. Restore rebuilds them from stable actor identities and ordered actor state.
7. Restore validates schema version, positive entity identity, spatial-state validity, rest-need validity, locomotion-continuation validity and duplicate identities before replacing the current world.
8. Restore is atomic with respect to validation: malformed snapshots leave the target `World` unchanged.
9. Successful restore preserves seed/tick/revision exactly and does not advance simulation time or revision merely because loading occurred.
10. Equal restored state plus equal subsequent authoritative operations must produce equal subsequent snapshots. Native tests prove this for existing world operations, fractional fixed-step locomotion continuation and the first rest-need state.
11. Unsupported schema versions are rejected. Schema v1 cannot reconstruct authoritative locomotion remainder; schema v2 does not contain the newly authoritative `RestNeedState`. This bounded proof still does not introduce migration policy.

## Not decided here

`WorldSnapshot` is not a disk or network format. Full persistence still needs a versioned outer envelope, serialization/IO, content version, protocol/schema compatibility policy, migration strategy, corruption handling, atomic file replacement and product-facing save-slot behavior when Milestone 7 is implemented.

No JSON, binary codec, database, event sourcing, replay-log format or generic serialization framework is selected by this decision.

## Consequences

- Adding new authoritative `World` state creates an explicit review obligation: it must be captured/restored by the snapshot contract or be demonstrably derived from captured state.
- Fractional fixed-step state cannot disappear across restore and subtly change later movement.
- Actor need state that changes future decisions cannot be recreated from presentation or session defaults after restore.
- Runtime caches/indexes can change implementation without changing persisted world truth.
- Deterministic continuation is tested before economy/social/political systems make persistence gaps harder to isolate.
- The eventual save format can serialize a stable Core value model rather than reverse-engineering live object internals.
- Milestone 7 remains responsible for real cross-session persistence; this ADR is only the minimum Core feasibility proof that makes that later work safer.
