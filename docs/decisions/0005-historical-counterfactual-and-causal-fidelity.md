# ADR 0005: Historical counterfactual and adaptive causal fidelity

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../PRODUCT.md`](../PRODUCT.md) · [`../MODELING.md`](../MODELING.md) · [`../research/high-medieval-baseline-c1200.md`](../research/high-medieval-baseline-c1200.md) · [`../ROADMAP.md`](../ROADMAP.md)

## Context

The game needs a believable systemic world inspired by medieval Europe around 1200, while magic can invalidate major historical constraints.

Two failure modes are especially dangerous:

1. building a rigid pseudo-medieval simulator from familiar fantasy clichés and later bolting magic onto it;
2. attempting to simulate every detail of every distant person at full resolution, producing complexity that the player can neither observe nor meaningfully affect.

Historical research also does not support a single universal “feudal pyramid” as the social/political model. Lordship, legal status, offices, towns, church institutions, markets, households and local communities varied and overlapped. Social mobility existed alongside inherited advantage and institutional barriers.

The Simulation therefore needs both a sourced non-magical baseline and an explicit rule for choosing modeling resolution.

## Decision

### Historical baseline is circa 1200, but regional assumptions stay explicit

The default research horizon is Latin/Western-Central Europe around **1180–1230**.

This is a comparative baseline, not a universal content template. A concrete scenario/mechanic records the narrower region and evidence when regional differences affect rules.

Do not encode “medieval” as one set of constants or one universal institution tree.

### Social position is compositional, not a caste/class enum

The authoritative simulation does not use a single `Peasant`, `Merchant`, `Noble`, `Priest`, `Bandit`, or `PlayerClass` value as the source of permissions and behavior.

Roles emerge from concrete world state such as:

- property/control and material resources;
- access/territory;
- skills and knowledge;
- household/kin/social ties when relevant;
- rights and legal status;
- debts and obligations;
- patronage and alliances;
- institution/organization membership;
- office and jurisdiction;
- reputation/standing/trust/fear where mechanics require them;
- coercive capability;
- magical capability/access;
- past actions and persistent consequences.

A UI may summarize those facts as “merchant”, “noble”, “bandit” or another role. The label is a projection, not world authority.

### Institutions resist disruption through actual capabilities, not plot armor

Existing elites and institutions do not receive hardcoded immunity from a magically empowered low-status actor.

They may preserve power through real mechanisms they possess: property, office, legitimacy, law, patronage, wealth, armed followers, alliances, information, sanctions, recruitment, licensing, religious authority, countermeasures or violence.

When a new magical capability changes the balance, those actors can adapt. If their mechanisms are insufficient, the political/social order may genuinely change or collapse.

This allows a commoner to rise extraordinarily far without making rulers implausibly passive or mechanically invulnerable.

### Magic is a permanent causal dimension of model review

History defines the **non-magical reference trajectory**. Magic defines explicit counterfactual changes to its constraints.

Every serious model contract must identify its **magic sensitivity surface**: which assumptions/constraints could be altered by currently implemented or future magic, even when that mechanic does not yet contain a spell.

This requirement does **not** mean:

- add a `magic` parameter to every function;
- add one universal `magic_multiplier`;
- create a generic effect bus before concrete effects exist;
- invent all future schools/spells now.

Instead, future magic changes concrete domain facts or mechanisms, and dependent systems respond to those changed facts.

Examples:

```text
healing -> mortality/recovery/work capacity
        -> household labor/care
        -> production/income
        -> prices/tax/military capacity
        -> institutional/political effects

fast transport -> travel/transport cost
              -> market integration/customs/security
              -> location value/power

extraordinary coercive magic -> security balance
                            -> alliances/recruitment/countermeasures
                            -> office/legitimacy conflict
```

A system that assumes a load-bearing mundane constraint is immutable without recording that assumption is incomplete.

### Simulation fidelity follows causal relevance, not camera distance

The full world does not run every process at the same microscopic resolution.

Choose the cheapest representation that preserves the causal facts needed by current gameplay and already-observed history.

Conceptual levels:

1. **Identity-resolved** — an individual actor/item/institution has persistent state because its identity, choices, relationships or history matter.
2. **Aggregate-resolved** — stocks/populations/distributions/flows are represented when collective effects matter but individual identity does not.
3. **Deferred/derived** — detail is not stored or updated until a real mechanic makes it causal.

These are modeling levels, not mandatory C++ base classes or an immediate dynamic-LOD framework.

### Offscreen is not the same as irrelevant

Camera distance or Godot materialization never decides Simulation fidelity by itself.

An offscreen actor/process remains identity-resolved when its identity or causal chain matters: a ruler making a decision, a merchant owing money, a named enemy, a caravan carrying owned goods, a plague reaching the player's region, or an institution enforcing an obligation.

Conversely, a visible crowd member does not require a lifetime of microscopic history if no mechanic depends on it.

### Promotion/demotion must preserve causal continuity

If future mechanics change representation resolution, they must preserve relevant invariants:

- conserved quantities/stocks;
- named or historically consequential identities;
- ownership, rights, debts and obligations that remain causal;
- previously observed facts/events;
- distributions needed for plausible later disaggregation;
- deterministic/replay guarantees.

Promotion must not invent a convenient past that contradicts the aggregate state. Demotion must not erase a fact the player can later hold the world accountable for.

### Model realism is evaluated against observable patterns and consequences

“More variables” is not the realism target.

For each serious model, define what player-observable/system-level patterns make it fit for purpose. This adapts the model-design principle behind ODD: purpose, entities/state/scales, processes/scheduling and evaluation criteria should be explicit.

A mechanism is worth modeling when omitting it produces the wrong opportunity, constraint, trajectory or explanation for gameplay.

## Consequences

### Positive

- weak and strong models get one historical/research direction instead of fantasy clichés;
- social/economic/political systems remain open to extreme magical disruption;
- player and NPC roles can emerge from the same compositional state;
- offscreen simulation can remain meaningful without requiring microscopic detail everywhere;
- later optimization can aggregate only where causal semantics allow it;
- historical claims and deliberate fantasy deviations become distinguishable.

### Costs

- feature work must do focused historical research when a broad baseline is insufficient;
- aggregation/disaggregation, when eventually needed, must define conservation/history rules rather than simply deleting agents;
- magic can force revisiting downstream models instead of remaining isolated in one subsystem;
- world trajectories may diverge strongly from real history and require long-horizon validation.

## Deliberately not introduced

This decision does **not** introduce:

- a universal `SocialClass` hierarchy;
- a complete occupation taxonomy;
- a universal magic ontology;
- spell schools or magic balance constants;
- `MagicSystem`/global effect bus abstractions with no implemented capability;
- dynamic agent aggregation runtime code;
- ECS/sharding/distributed simulation;
- complete demography/religion/politics/economy models;
- authoritative spatial movement.

Those belong to later capabilities when there is an actual player-facing mechanism and evidence need.

## Model author checklist

Before implementing a serious Simulation mechanic, answer:

1. What player-visible/actionable causal question does this model answer?
2. What is the circa-1200 non-magical baseline, and which region/evidence supports it?
3. Which entities must be identity-resolved, which can be aggregate-resolved, and what is deliberately absent?
4. Which stocks/rights/relationships/history must survive a resolution change?
5. What opportunities/actions are available to both NPC and human-controlled actors through the same rule path?
6. Which mundane assumptions form this model's magic sensitivity surface?
7. If a future magical capability breaks one of those assumptions, which downstream mechanisms receive the changed state?
8. Which institutions/actors can respond, and through what actual capabilities?
9. What observable patterns/outcomes make the model realistic enough for gameplay?
10. What long-horizon or counterfactual test would falsify the current simplification?

If these answers require invented framework types rather than a real current mechanic, stop and simplify.
