# Development roadmap

This document owns **current milestone status and sequencing**. It does not duplicate mechanic specifications, protocol schemas, test inventories, fixture numbers or verification results.

For detail, route to [`INDEX.md`](INDEX.md). Current source/tests remain authoritative for implemented behavior, and [`VERIFICATION.md`](VERIFICATION.md) owns what evidence is required to accept a claim.

## Current state

The playable/native spine, authoritative world/presentation boundary, historical/causal-fidelity foundation, exact-spatial contract and first grounded locomotion bridge are accepted.

**Milestone 1 — Living Need is in progress.** One authoritative NPC need now produces a causal local task, enters the same actor-generic movement rules as human control, and is presented in Godot only from authoritative observation/transition data. The locomotion capability correction is also integrated: semantic pace is intent while numeric capability remains authoritative actor state.

The remaining Milestone 1 sequence is intentionally short:

1. add the minimum shared authoritative condition/action through which the controlled actor can help or obstruct the NPC need outcome;
2. prove the NPC continues the causal need/task when its Godot representation is absent;
3. run one bounded real playtest of that interference/help loop and fix defects exposed by the playtest.

Do not insert a generic AI/planner, production navigation framework, universal scheduler/event bus, ECS, stat/effect framework or speculative spatial engine ahead of those steps.

## Milestone rule

Every milestone must end in a playable or otherwise directly observable systemic capability.

Simulation-only depth may not accumulate several milestones ahead of player experience. Systemic milestones preserve actor parity: a human-controlled actor enters the same world-rule path as an equivalent NPC.

A later architectural idea is not a prerequisite unless the current capability is actually blocked without it.

## Accepted foundations

These are accepted prerequisites, not active roadmap work:

### Toolchain & playable spine

The repository has a separated native Simulation/protocol graph, GDExtension adapter, Godot presentation client, deterministic native verification and bounded Godot playtest path.

Acceptance/evidence: [`VERIFICATION.md`](VERIFICATION.md).

### Historical counterfactual and causal fidelity

The world model uses sourced historical baselines, compositional social state and causality-driven detail rather than a camera-distance-only fidelity policy.

Canonical owners: [`MODELING.md`](MODELING.md), [`decisions/0005-historical-counterfactual-and-causal-fidelity.md`](decisions/0005-historical-counterfactual-and-causal-fidelity.md) and relevant research/model documents.

### Authoritative spatial contract

Simulation owns authoritative location at the fidelity required by causality; exact spatial state is selective and Godot presentation is not world truth.

Canonical owners: [`decisions/0006-authoritative-spatial-contract.md`](decisions/0006-authoritative-spatial-contract.md), [`models/spatial-location.md`](models/spatial-location.md) and [`decisions/0009-simulation-authority-and-decision-sources.md`](decisions/0009-simulation-authority-and-decision-sources.md).

### Grounded locomotion bridge

Human and NPC intent sources can enter the same actor-generic authoritative movement law, with numeric movement capability owned by Simulation rather than Godot/client policy.

Canonical mechanic owner: [`models/grounded-locomotion.md`](models/grounded-locomotion.md). Exact proof obligations/evidence belong to [`VERIFICATION.md`](VERIFICATION.md).

The accepted bridge proves the authority seam; it is not a commitment to build a general physics/navigation engine. Production spatial dependencies are admitted only from a concrete blocked capability under ADR 0009.

## Milestone 1 — Living Need — in progress

Goal: prove that an NPC acts because of authoritative world state and that the player can change the outcome through the same causal world rather than a player-only shortcut.

Implemented slice:

- one identity-resolved NPC has an authoritative rest need;
- Core derives a concrete local task from that need;
- the task produces ordinary semantic movement intent rather than directly mutating position;
- NPC and human-controlled intents enter the same authoritative movement rules;
- Godot observes/materializes the NPC from authoritative identity and movement data;
- need/capability/continuation state that affects future behavior is preserved by current Core snapshot truth.

Still required:

- one minimum shared player help/interference world action or condition;
- offscreen continuation with no Godot node required for the causal behavior;
- bounded real playtest of the complete loop.

Mechanic semantics: [`models/living-need.md`](models/living-need.md).

Acceptance:

> An NPC acts because of authoritative world state, and the player can change the outcome through the same causal system rather than a player-only shortcut.

## Milestone 2 — Household Resource Loop

Minimum scope:

- household stock;
- production and consumption;
- shortage;
- player/NPC trade, gift or work through shared transaction/world rules;
- shop/inventory presentation reads projections rather than owning stock or money.

Acceptance:

> The village can develop a resource problem without the player, and the player can address it through more than one real path while resource transfers remain authoritative Simulation state.

## Milestone 3 — Social Consequence

Minimum scope:

- one useful trust/obligation/reputation dimension;
- interaction changes a future opportunity or cost;
- an NPC remembers a relevant event through authoritative state;
- Godot displays the consequence without owning the relationship.

Acceptance:

> A later action becomes available, unavailable or differently costly because of earlier authoritative social state.

## Milestone 4 — First Institution / Politics

Minimum scope:

- one office or authority;
- one real permission/obligation;
- one way any qualified actor can gain, lose or influence it;
- one visible distributional consequence.

Acceptance:

> The player can participate in power through the same world state/rules available to simulated actors rather than a quest flag or Godot-owned state.

## Milestone 5 — First Magic Counterfactual

Minimum scope:

- one magic capability;
- explicit access and cost;
- a non-magical alternative/baseline;
- one load-bearing mundane constraint changed by magic;
- downstream economic/social/institutional effects through existing causal state.

Acceptance:

> Magic changes the authoritative system trajectory and institutions/actors react through actual capabilities; it is not only presentation or a subsystem multiplier.

## Milestone 6 — Emergent Role

Enough compositional state exists that any suitably situated actor — including the human-controlled one — can occupy materially different roles such as worker, trader, bandit, apprentice or office-holder through world paths.

Acceptance:

> Roles emerge from state/rules without an authoritative hardcoded player/social class.

## Milestone 7 — Persistence & Repeated Play

Minimum scope:

- save/load of authoritative world state;
- stable entity identities preserved;
- replay/determinism evidence appropriate to implemented mechanics;
- a substantial repeated session;
- gameplay fixes based on actual play.

Acceptance:

> The authoritative world preserves understandable consequences across sessions and the game supports continued play.

## Admission gates for future infrastructure

These gates prevent roadmap drift without pre-selecting implementations:

- **Navigation/production spatial dependency:** admit only when a real gameplay route/reachability requirement is blocked by the current bounded spatial capability.
- **Offscreen scheduler/time architecture:** admit only the minimum mechanism required by the first real offscreen continuation, then re-admit broader scheduling when another capability needs it.
- **Independent time advancement:** before a non-locomotion production system advances time independently, re-evaluate movement-stream ordering under ADR 0009.
- **Dynamic scenario composition:** replace the current bounded acceptance composition only when a real scenario requires dynamic population/content ownership.
- **External/LLM policy:** implement the deterministic shared world action/goal and validation rules first; language interpretation or an external policy may be added later above that contract.

## Roadmap discipline

A new milestone or major subsystem must identify:

1. the player-visible/systemically observable capability;
2. the minimum authoritative causal state/rule required;
3. the protocol/projection/presentation change needed to expose it;
4. the bounded evidence that proves the complete loop.

Do not move implementation details, protocol version histories, exact test outputs or numeric mechanic fixtures into this roadmap. Link to their canonical owner instead.
