# Model: Remembered Material Aid

Status: DRAFT

## Gameplay purpose

Milestone 3 needs one earlier material choice to alter a later social opportunity. This social state is deliberately narrower than friendship, trust, reputation or a general relationship graph.

The implemented model gives a household one bounded outstanding memory:

```text
this household remembers that actor X materially helped while we were short
```

That remembered cause now changes one later authoritative opportunity: while the household can safely spare grain, the remembered actor may ask for one reciprocal material favour at that household's store. A successful repayment moves real household grain into the actor's existing carry slot and clears the remembered favour.

Canonical whole-milestone outcome: [`../milestones/m3-playable-social-consequence.md`](../milestones/m3-playable-social-consequence.md).

## Authoritative state

`HouseholdState::remembered_material_aid_actor` is one optional actor reference encoded as an `EntityId`:

- `EntityId{0}` — no outstanding remembered personal aid;
- positive actor id — this household remembers one qualifying material Gift from that actor;
- negative values are invalid state;
- a positive reference must resolve to an existing actor when the household is composed or a snapshot is restored;
- the remembered actor may not be a member of the remembering household, because the authoritative Gift law rejects gifting to one's own household and such a state cannot represent the qualifying cause this field claims.

The slot is intentionally singular. M3 needs one remembered favour in one short vignette, not simultaneous debt accounting for every actor. If a later playable capability requires concurrent household obligations, extend the causal model then rather than prebuilding a generic relationship store now.

## Qualifying Gift rule

The existing actor-generic `World::gift_household_grain(actor, receiving_household)` remains the material law.

A successful Gift records the acting actor in `remembered_material_aid_actor` only when all of these are true immediately before commit:

1. every existing Gift validation succeeds;
2. the receiving household is short (`grain_stock_units < shortage_threshold_units`);
3. the household has no outstanding remembered-aid actor.

The qualifying condition uses **pre-transition shortage**. A Gift can therefore be remembered even when that same Gift relieves the shortage; the social cause is that the actor helped while the household was in the shortage state.

A Gift to a household that was not short remains a real material Gift but creates no M3 social credit.

If the household already has an outstanding remembered-aid actor, another valid Gift remains a normal material Gift and does not overwrite the existing actor. The bounded slot is not a last-writer-wins reputation value.

## Reciprocal aid rule

`World::request_household_reciprocal_aid(actor, household)` is the one bounded future opportunity created by the remembered cause.

The request succeeds only when all of these current authoritative prerequisites hold:

1. actor and selected household exist and their relevant resource/social state is valid;
2. the actor is physically inside that household's authoritative store footprint;
3. the household has an outstanding remembered-aid actor;
4. that remembered actor is the requesting actor;
5. the actor has positive free grain-carry capacity;
6. the household has grain strictly above its shortage threshold.

The caller supplies no amount. Core computes:

```text
free carry = carry capacity - current carry
safe surplus = household stock - shortage threshold
received grain = min(free carry, safe surplus)
```

A successful repayment therefore cannot itself make the household short. It changes existing material truth rather than minting a reward:

```text
household stock decreases by received grain
actor carry increases by the same received grain
remembered_material_aid_actor becomes none
WorldRevision advances exactly once
SimulationTick does not advance
```

The favour is one-shot. After a successful reciprocal transfer, a second request refuses because there is no outstanding remembered aid.

Material inability does **not** erase the obligation. Full carry, no safe household surplus, wrong location, or another ordinary refusal leaves household stock, actor carry, remembered aid, tick and revision unchanged. The actor can try again later if material prerequisites change.

A materially feasible household with no remembered aid refuses for the social reason `no_remembered_aid`. A household remembering another actor refuses `remembered_for_other_actor`. These cases are required to distinguish the M3 consequence from stock/occupancy failure.

## Atomic causality

Material Gift and newly created social memory share one authoritative Core transition boundary.

For a qualifying Gift, one successful `World::gift_household_grain` transition performs all of the following before advancing `WorldRevision` exactly once:

```text
actor carried grain decreases to zero
receiving household grain increases by the same amount
remembered_material_aid_actor changes from none to the acting actor
```

There is no second application/protocol/Godot call that "adds reputation" after the Gift has already committed.

Reciprocal repayment has the same atomicity requirement in reverse: grain transfer into actor carry and clearing the remembered favour either both commit in one Core transition or neither changes.

All validation completes before material/social mutation. A refused Gift or reciprocal request leaves relevant material state, remembered aid, `SimulationTick` and `WorldRevision` unchanged.

This preserves the M3 contract requirement that the world never exposes social credit without its material cause, a committed qualifying contribution without required memory, or a consumed favour without the material repayment that consumed it.

## Attribution

The remembered actor is the actor whose accepted `Gift` transition moved their authoritative carried grain into the short household.

This is actor-generic: the Core has no controlled-player branch. Any actor with equivalent state can become the remembered actor and use the same reciprocal-aid World law.

The existing standing household transfer does **not** create this personal aid state. Its grain and pledge are source-household state, so executing that household obligation is not automatically personal credit for the executor. This provides the natural M3 control path: the target shortage can be materially relieved without creating personal remembered aid.

Work is not wired into this state in the current model. The whole M3 contract allows a future attributable Work path, but adding it now would broaden the social trigger without a second player-facing need.

## Protocol and presentation

Protocol version 11 adds only the semantic surface needed by this vignette:

- a controlled reciprocal-aid command selecting one household and no quantity;
- a purpose-built projection that reports whether that selected household currently remembers the controlled actor;
- typed reciprocal-aid success/refusal results.

The projection intentionally does **not** expose the identity of another remembered actor and does not claim that repayment will succeed. Material feasibility remains Core-owned and is revalidated by the command.

GDExtension translates those protocol values. Godot uses the existing neighbour-store interaction context: the same transfer input that executes the controlled actor's own-household standing pledge requests reciprocal aid when the actor occupies the tracked neighbour store. Godot never sets/clears the favour and never computes the repayment quantity.

Ordinary interaction feedback can therefore say that the neighbour remembers a qualifying contribution, invite the player to ask for returned aid, report successful repayment, or explain a social/material refusal without displaying raw actor IDs or relationship variables.

## Snapshot and validation

The remembered actor changes future authoritative opportunity and is therefore snapshot truth under ADR 0008.

Snapshot schema version 10 already includes it as part of each `HouseholdState`; reciprocal aid adds no new authoritative field, so this vertical does not require another snapshot schema bump.

Restore/composition reject:

- negative remembered actor ids as invalid household social state;
- positive remembered actor ids that do not resolve to an existing actor;
- a remembered actor who is also a member of that household, because this bounded state specifically represents a qualifying inter-household Gift that Core would reject for an own-household member.

`EntityId{0}` remains the canonical empty state.

Snapshot restore preserves the exact remembered actor without advancing tick/revision. Derived UI/protocol views are not snapshot truth.

## Determinism and ordering

No randomness or wall-clock state participates in either Gift qualification or reciprocal aid.

For equal initial World state and equal command/step sequence, remembered-aid creation and repayment are deterministic.

Both transitions are revision-only and do not advance `SimulationTick`. Each material/social pair shares one post-transition revision.

## Explicit non-goals

This model does not implement:

- multiple simultaneous household creditors;
- numeric reputation/trust/affection;
- friendship, kinship, hostility or faction standing;
- event logs or natural-language memory;
- a generic relationship graph/index;
- dialogue or quest state;
- Godot-owned social state;
- caller-selected reciprocal quantities or reward spawning;
- household-transfer social credit;
- Work social credit;
- timers/cooldowns forcing repayment to occur after an arbitrary delay.

A later playable capability may extend only the concrete dimension it needs.

## Current acceptance boundary

Native and protocol proof for this vertical must demonstrate:

- short-household Gift changes grain/carry and creates remembered aid in one revision;
- non-short Gift changes material state without creating remembered aid;
- refused Gift is fully non-mutating;
- any equivalent actor can become the remembered actor;
- an existing remembered actor is not overwritten by later Gifts;
- snapshot/restore preserves the state and rejects malformed, dangling, and own-household-member remembered-aid references;
- the remembered actor can receive one Core-owned reciprocal grain amount from safe household surplus, and repayment clears the favour in the same revision;
- repayment never reduces household stock below its shortage threshold;
- material refusal preserves the favour;
- another actor cannot consume the favour;
- a materially feasible control path without remembered aid refuses for the social reason and is non-mutating;
- protocol exposes the controlled actor's remembered state without exposing another creditor identity and carries no caller-authored repayment amount;
- GDExtension/Godot present the semantic command and feedback without owning social/material truth.

This vertical makes the remembered cause actionable in ordinary interaction, but whole Milestone 3 acceptance still additionally requires bounded debug-hidden helped/control play evidence and the milestone's human-readable neighbour/household identity gate.