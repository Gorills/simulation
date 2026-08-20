# Model: Remembered Material Aid

Status: DRAFT

## Gameplay purpose

Milestone 3 needs one earlier material choice to alter a later social opportunity. This first social state is deliberately narrower than friendship, trust, reputation or a general relationship graph.

The implemented Core subset gives a household one bounded outstanding memory:

```text
this household remembers that actor X materially helped while we were short
```

The current state is only the remembered cause. The reciprocal-aid action that later consumes/changes this state is a subsequent M3 vertical slice.

Canonical whole-milestone outcome: [`../milestones/m3-playable-social-consequence.md`](../milestones/m3-playable-social-consequence.md).

## Authoritative state

`HouseholdState::remembered_material_aid_actor` is one optional actor reference encoded as an `EntityId`:

- `EntityId{0}` — no outstanding remembered personal aid;
- positive actor id — this household remembers one qualifying material Gift from that actor;
- negative values are invalid state;
- a positive reference must resolve to an existing actor when the household is composed or a snapshot is restored;
- the remembered actor may not be a member of the remembering household, because the authoritative Gift law rejects gifting to one's own household and such a state cannot represent the qualifying cause this field claims.

The slot is intentionally singular. M3 currently needs one remembered favour in one short vignette, not simultaneous debt accounting for every actor. If a later playable capability requires concurrent household obligations, extend the causal model then rather than prebuilding a generic relationship store now.

## Qualifying Gift rule

The existing actor-generic `World::gift_household_grain(actor, receiving_household)` remains the material law.

A successful Gift records the acting actor in `remembered_material_aid_actor` only when all of these are true immediately before commit:

1. every existing Gift validation succeeds;
2. the receiving household is short (`grain_stock_units < shortage_threshold_units`);
3. the household has no outstanding remembered-aid actor.

The qualifying condition uses **pre-transition shortage**. A Gift can therefore be remembered even when that same Gift relieves the shortage; the social cause is that the actor helped while the household was in the shortage state.

A Gift to a household that was not short remains a real material Gift but creates no M3 social credit.

If the household already has an outstanding remembered-aid actor, another valid Gift remains a normal material Gift and does not overwrite the existing actor. The bounded slot is not a last-writer-wins reputation value.

## Atomic causality

Material Gift and newly created social memory share the same authoritative Core transition boundary.

For a qualifying Gift, one successful `World::gift_household_grain` transition performs all of the following before advancing `WorldRevision` exactly once:

```text
actor carried grain decreases to zero
receiving household grain increases by the same amount
remembered_material_aid_actor changes from none to the acting actor
```

There is no second application/protocol/Godot call that "adds reputation" after the Gift has already committed.

All Gift validation, including checked destination-stock addition, completes before any of those fields mutate. A refused Gift leaves material state, remembered aid, `SimulationTick` and `WorldRevision` unchanged.

This preserves the M3 contract requirement that the world never exposes "gift happened, social memory failed" or "social credit exists without its qualifying material cause".

## Attribution

The remembered actor is the actor whose accepted `Gift` transition moved their authoritative carried grain into the short household.

This is actor-generic: the Core has no controlled-player branch. Any actor with equivalent state can become the remembered actor.

The existing standing household transfer does **not** create this personal aid state. Its grain and pledge are source-household state, so executing that household obligation is not automatically personal credit for the executor.

Work is also not wired into this state in the current subset. The whole M3 contract allows a future attributable Work path, but adding it now would expand one Core slice without a second player-facing need.

## Snapshot and validation

The remembered actor changes future authoritative opportunity and is therefore snapshot truth under ADR 0008.

Snapshot schema version 10 includes it as part of each `HouseholdState`.

Restore/composition reject:

- negative remembered actor ids as invalid household social state;
- positive remembered actor ids that do not resolve to an existing actor;
- a remembered actor who is also a member of that household, because this bounded state specifically represents a qualifying inter-household Gift that Core would reject for an own-household member.

`EntityId{0}` remains the canonical empty state.

Snapshot restore preserves the exact remembered actor without advancing tick/revision. Derived UI/protocol views are not snapshot truth.

## Determinism and ordering

No randomness or wall-clock state participates in the rule.

For equal initial World state and equal Gift command sequence, the remembered-aid result is deterministic.

The transition remains revision-only: Gift does not advance `SimulationTick`. The material and social consequence share the same post-transition revision.

## Explicit non-goals

This subset does not implement:

- reciprocal aid or repayment;
- multiple simultaneous household creditors;
- numeric reputation/trust/affection;
- friendship, kinship, hostility or faction standing;
- event logs or natural-language memory;
- a generic relationship graph/index;
- dialogue, quests or Godot-owned social state;
- household-transfer social credit;
- Work social credit;
- protocol/GDExtension/Godot presentation of the memory.

The next M3 slice should consume this exact authoritative cause to create one later opportunity, rather than broadening the social model horizontally.

## Current acceptance boundary

Native proof for this subset must demonstrate:

- short-household Gift changes grain/carry and creates remembered aid in one revision;
- non-short Gift changes material state without creating remembered aid;
- refused Gift is fully non-mutating;
- any equivalent actor can become the remembered actor;
- an existing remembered actor is not overwritten by later Gifts;
- snapshot/restore preserves the state and rejects malformed, dangling, and own-household-member remembered-aid references.

This Core subset alone does **not** satisfy whole Milestone 3 acceptance because there is not yet a reciprocal opportunity or ordinary-play presentation.
