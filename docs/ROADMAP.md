# Development roadmap

This document owns **current milestone status and sequencing**. It does not duplicate mechanic specifications, protocol schemas, test inventories, fixture numbers or verification results.

For detail, route to [`INDEX.md`](INDEX.md). Current source/tests remain authoritative for implemented behavior, and [`VERIFICATION.md`](VERIFICATION.md) owns what evidence is required to accept a claim.

## Current state

The playable/native spine, authoritative world/presentation boundary, historical/causal-fidelity foundation, exact-spatial contract and first grounded locomotion bridge are accepted.

**Milestone 1 — Living Need is accepted.** The vertical loop contains authoritative need state, Core-owned decision output, actor-generic movement, shared player interference, offscreen continuation, a purpose-built read-only client outcome and a bounded real-client `traveling -> blocked -> satisfied` scenario.

**Milestone 2 — Household Resource Loop is accepted.** The village can develop a grain shortage without player intervention; the player can address it through gift, household transfer, and work while every stock change remains authoritative Simulation state. Bounded native, sanitizer, performance, localization, and RU+EN Godot evidence for shortage, gift, work, transfer, offscreen continuation, and unchanged M1 rest interference is recorded in [`VERIFICATION.md`](VERIFICATION.md).

Active development now moves to **Milestone 3 — Playable Social Consequence**. M3 turns the accepted shortage into one coherent vignette in which a material choice toward an identifiable neighbour is remembered and changes a later opportunity. The whole-milestone contract is [`milestones/m3-playable-social-consequence.md`](milestones/m3-playable-social-consequence.md). Do not expand RestNeed or the M2 household-resource loop horizontally unless this vignette exposes a concrete defect or missing causal requirement.

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

## Milestone 1 — Living Need — accepted

Goal:

> An NPC acts because of authoritative world state, and the player can change the outcome through the same causal system rather than a player-only shortcut.

Accepted vertical capability:

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

Milestone 1 is complete; active work must move to M2 instead of growing a generic need/planner framework.

## Milestone 2 — Household Resource Loop — accepted

Minimum scope:

- household stock;
- production and consumption;
- shortage;
- player/NPC gift, household transfer, and work through shared transaction/world rules;
- inventory/pledge presentation reads projections rather than owning stock or money.

Priced exchange (`trade` / `buy` / `sell` / `shop`) is deferred until a later milestone that introduces a second good or coin. The M2 third path is a one-way standing household transfer, not a market.

Acceptance evidence is defined in [`VERIFICATION.md`](VERIFICATION.md). Mechanic semantics are in [`models/household-resource.md`](models/household-resource.md).

Milestone 2 is complete; active work must move to M3 instead of growing a general inventory/economy framework.

## Milestone 3 — Playable Social Consequence — active

Goal:

> An earlier material choice toward an identifiable neighbour is remembered by authoritative social state and changes a later concrete opportunity in ordinary play.

Minimum scope:

- reuse the accepted M2 autonomous shortage as the initiating world problem;
- one bounded obligation / remembered-material-aid dimension attached to the relevant actor and social counterparty;
- a real M2 contribution can create the social consequence; refused/no-op actions cannot;
- one later reciprocal-aid opportunity is available/refused or materially different because of that social state;
- the rule remains actor-generic and Godot only presents/requests it;
- the affected household/NPC and the later consequence are understandable with technical diagnostics hidden.

Acceptance:

> In one short ordinary-play vignette, the player can identify who has the problem, choose whether/how to help through real world rules, see that the help was remembered, and later encounter a materially different opportunity because of that authoritative social memory. A control path without the qualifying help produces the contrasting later outcome.

Whole-milestone contract: [`milestones/m3-playable-social-consequence.md`](milestones/m3-playable-social-consequence.md). Exact mechanic semantics will live in the relevant model document when implementation introduces the authoritative M3 state/rule; proof obligations/evidence belong in [`VERIFICATION.md`](VERIFICATION.md).

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
- **Dynamic scenario composition:** replace the current bounded acceptance composition when a real scenario requires dynamic population/content ownership; M2 now triggers this gate.
- **External/LLM policy:** implement deterministic shared world actions/goals and validation first; language interpretation or an external policy may be added later above that contract.

## Roadmap discipline

A new milestone or major subsystem must identify:

1. the player-visible/systemically observable capability;
2. the minimum authoritative causal state/rule required;
3. the protocol/projection/presentation change needed to expose it;
4. the bounded evidence that proves the complete loop.

Do not move implementation details, protocol version histories, exact test outputs or numeric mechanic fixtures into this roadmap. Link to their canonical owner instead.