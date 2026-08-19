# ADR 0008 — Core snapshot/restore determinism contract

Status: ACCEPTED

## Context

Full save/load remains a later product milestone, but the Simulation Core already owns durable identity, seed, simulation tick, world revision, actor state and optional exact spatial state. As more causal systems are added, waiting until the final persistence milestone to prove that authoritative state is fully capturable would make hidden runtime state and non-restorable caches increasingly expensive to discover.

The current `World` also maintains an `EntityId -> actor index` lookup for performance. That index is derived runtime structure: persisting it would couple saves to one storage implementation and would incorrectly elevate cache/index state into world truth.

## Decision

1. Simulation Core exposes a versioned in-memory `WorldSnapshot` value contract.
2. Snapshot schema version 1 contains the current authoritative `World` state: seed, `SimulationTick`, `WorldRevision`, and actor states in deterministic insertion order.
3. Actor snapshot state includes every field currently owned by `ActorState`, including the Milestone 0 bootstrap position and optional `SpatialState`. The bootstrap field is temporary gameplay architecture, but while it remains authoritative Core state it must round-trip exactly.
4. Derived runtime structures such as `actor_index_by_id_` are never snapshot truth. Restore rebuilds them from stable actor identities and ordered actor state.
5. Restore validates schema version, positive entity identity, spatial-state validity and duplicate identities before replacing the current world.
6. Restore is atomic with respect to validation: malformed snapshots leave the target `World` unchanged.
7. Successful restore preserves seed/tick/revision exactly and does not advance simulation time or revision merely because loading occurred.
8. Equal restored state plus equal subsequent authoritative operations must produce equal subsequent snapshots. Native tests must prove this continuation property.
9. Unsupported schema versions are rejected. This bounded proof does not introduce migration policy yet.

## Not decided here

`WorldSnapshot` is not a disk or network format. Full persistence still needs a versioned outer envelope, serialization/IO, content version, protocol/schema compatibility policy, migration strategy, corruption handling, atomic file replacement and product-facing save-slot behavior when Milestone 7 is implemented.

No JSON, binary codec, database, event sourcing, replay-log format or generic serialization framework is selected by this decision.

## Consequences

- Adding new authoritative `World` state now creates an explicit review obligation: it must be captured/restored by the snapshot contract or be demonstrably derived from captured state.
- Runtime caches/indexes can change implementation without changing persisted world truth.
- Deterministic continuation is tested before economy/social/political systems make persistence gaps harder to isolate.
- The eventual save format can serialize a stable Core value model rather than reverse-engineering live object internals.
- Milestone 7 remains responsible for real cross-session persistence; this ADR is only the minimum Core feasibility proof that makes that later work safer.
