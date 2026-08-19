# ADR 0007 — Protocol integer range at the Godot boundary

Status: ACCEPTED

## Context

Simulation Core uses unsigned 64-bit values for `SimulationTick`, `WorldRevision`, `WorldSeed` and `SpatialEpoch`. Godot Variant/GDScript integers are signed 64-bit values.

The initial adapter cast protocol `uint64_t` fields directly to `int64_t`. Values above `INT64_MAX` therefore had no valid numeric representation in the Godot integer domain and could not be treated as a supported application-boundary contract.

This matters even though ordinary gameplay will never approach that limit: persistence, replay/debug tooling and long-lived deterministic state must not depend on silent narrowing or platform-specific conversion behavior.

## Decision

1. Numeric protocol fields that cross into Godot use `protocol::ProtocolInteger`, an alias of signed `std::int64_t`.
2. Their supported non-negative range is `0 .. INT64_MAX`. Positive-only concepts such as `SpatialEpoch` keep their existing semantic validity rule on top of that range.
3. Simulation Core may retain unsigned 64-bit storage. Core-to-protocol conversion is explicit and checked with `to_protocol_integer`; values outside the protocol range are never wrapped or clamped.
4. The application `Simulation` seed is a `ProtocolInteger` and rejects negative input before constructing the Core `WorldSeed`.
5. A protocol mutation must not knowingly move an observable counter outside the protocol integer range. The current bootstrap mutation reports `protocol_integer_exhausted` before incrementing an already-maximal revision.
6. GDExtension assigns protocol integers directly to Godot values. It does not perform unsigned-to-signed casts for tick/revision/seed/epoch. A defensive out-of-range projection failure becomes an explicit bridge error rather than crossing the extension boundary as a C++ exception or corrupted integer.
7. `protocol_version` remains unchanged for this correction because the Godot-visible dictionary schema and all previously representable values are unchanged. The unsupported unsigned tail is being removed from the claimed C++ contract, not introduced as a new runtime representation.

## Why not strings or high/low integer pairs

Decimal strings or split high/low words could preserve the full unsigned range, but they would complicate ordering, validation, debugging and future high-frequency spatial samples for no demonstrated product requirement. The signed 64-bit range already exceeds any plausible runtime tick/revision/epoch horizon.

If a future persisted/networked format genuinely requires the full unsigned range at the client boundary, that is a new representation decision and must supersede this ADR rather than adding ad hoc per-field encodings.

## Consequences

- Protocol DTOs and Godot use the same integer range for observable counters and seed.
- Native tests cover `INT64_MAX` and reject `INT64_MAX + 1` conversion.
- Core unsigned types remain independent from the presentation technology.
- Future protocol operations that increment/export unsigned Core counters must use the same checked conversion/admission rule.
- No silent wrap, saturation or precision-loss fallback is permitted.

## Primary external contract

- Godot 4.7 Variant integer binary representation: 64-bit signed integer when encoded as 64-bit.
- Godot 4.7 Variant/GDScript documentation: GDScript integers use the 64-bit integer Variant domain.

See the official Godot 4.7 documentation for `Variant` and binary serialization.
