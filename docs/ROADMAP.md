# Development roadmap

This document owns **current milestone status and sequencing**. It does not duplicate mechanic specifications, protocol schemas, test inventories, fixture numbers or verification results.

For detail, route to [`INDEX.md`](INDEX.md). Current source/tests remain authoritative for implemented behavior, and [`VERIFICATION.md`](VERIFICATION.md) owns what evidence is required to accept a claim.

## Current state

The playable/native spine, authoritative world/presentation boundary, historical/causal-fidelity foundation, exact-spatial contract and first grounded locomotion bridge are accepted.

**Milestone 1 — Living Need is an acceptance candidate.** The vertical loop now contains authoritative need state, Core-owned decision output, actor-generic movement, shared player interference, offscreen continuation, a purpose-built read-only client outcome and a bounded real-client `traveling -> blocked -> satisfied` scenario.

This branch may mark M1 as the next accepted milestone only if the exact revision passes the required native/protocol and Godot evidence. If that scenario exposes a defect, fix the defect rather than weakening the acceptance condition.

No additional RestNeed framework work belongs ahead of that gate.

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

## Milestone 1 — Living Need — acceptance candidate

Goal:

> An NPC acts because of authoritative world state, and the player can change the outcome through the same causal system rather than a player-only shortcut.

Implemented vertical capability:

- one identity-resolved NPC has authoritative RestNeed state;
- Core derives deterministic local-task movement from that state;
- NPC and human-controlled movement enter the same actor-generic authoritative rule;
- another exact-spatial actor inside the assigned tolerance blocks satisfaction as a derived condition;
- the controlled actor can create/remove that condition through ordinary authoritative locomotion;
- the NPC continues the same causal need/travel path while its Godot node is absent;
- observation and materialization remain separate;
- a purpose-built `LivingNeedProjection` exposes only the derived `traveling` / `blocked` / `satisfied` outcome plus ordering/version context;
- Godot renders localized read-only need feedback rather than calculating the need itself;
- a bounded real-client scenario drives the controlled actor through the existing movement command, proves `blocked`, captures visible evidence, moves the actor away and proves later `satisfied`.

Acceptance evidence is defined in [`VERIFICATION.md`](VERIFICATION.md). Mechanic semantics are in [`models/living-need.md`](models/living-need.md).

If the final exact-revision gates pass, M1 is accepted and active development moves to Milestone 2 rather than expanding the first need horizontally.

## Milestone 2 — Household Resource Loop — next after M1 acceptance

Minimum scope:

- household stock;
- production and consumption;
- shortage;
- player/NPC trade, gift or work through shared transaction/world rules;
- shop/inventory presentation reads projections rather than owning stock or money.

Acceptance:

> The village can develop a resource problem without the player, and the player can address it through more than one real path while resource transfers remain authoritative Simulation state.

The first M2 slice must remain vertical and observable. Re-admit its minimum state/rule/read/presentation/evidence after M1 is green; do not prebuild a general inventory/economy framework.

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
- **Offscreen scheduler/time architecture:** admit only the minimum mechanism required by a real offscreen continuation; the first M1 proof did not require a scheduler.
- **Independent time advancement:** before a non-locomotion production system advances time independently, re-evaluate movement-stream ordering under ADR 0009.
- **Dynamic scenario composition:** replace the current bounded acceptance composition only when a real scenario requires dynamic population/content ownership.
- **External/LLM policy:** implement deterministic shared world actions/goals and validation first; language interpretation or an external policy may be added later above that contract.

## Roadmap discipline

A new milestone or major subsystem must identify:

1. the player-visible/systemically observable capability;
2. the minimum authoritative causal state/rule required;
3. the protocol/projection/presentation change needed to expose it;
4. the bounded evidence that proves the complete loop.

Do not move implementation details, protocol version histories, exact test outputs or numeric mechanic fixtures into this roadmap. Link to their canonical owner instead.
