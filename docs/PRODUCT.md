# Product contract

## Goal

Build a continuously playable systemic RPG in which world simulation, RPG progression, economy, social institutions, politics, trade and magic grow through small causal vertical slices.

The world exists independently of the player. NPCs, households, organizations and institutions act under the information and constraints available to them. The player is an ordinary participant in the same authoritative rules and can acquire roles, rights, obligations, property, knowledge, relationships and power through world state rather than hardcoded classes.

The core product loop is:

```text
world cause
  -> authoritative simulation consequence
  -> player/NPC opportunity
  -> visible game feedback
  -> player choice
  -> persistent world change
```

## Non-negotiable product invariants

### Playable main

Once the playable spine exists, `main` must remain runnable and playable. A change must not intentionally leave the repository in a state such as “core now, client later” or “UI broken until the refactor finishes”.

A temporary break is acceptable only inside the same bounded change that restores the playable path before the change is accepted.

### Vertical capability

The normal unit of gameplay development is one capability that crosses the layers it actually needs:

```text
RULE        authoritative causal rule
CONTRACT    command/result/projection/events
EXPERIENCE  what the player can do or see
PROOF       targeted test + bounded playtest
```

A capability is not done because only `RULE` is implemented.

### No simulation waiting room

Do not build a large simulation subsystem and postpone player exposure until later. New systems start as the smallest genuine causal model that is already visible or actionable.

Example:

```text
not:
complete grain economy -> logistics -> markets -> tax -> later player trade

instead:
minimal harvest -> one shortage -> one player transaction -> visible consequence
  -> then deepen production / market / institutions
```

### Fidelity ladder

Subsystems grow only when the previous level is playable and understandable:

```text
F0 absent
F1 minimal causal & playable
F2 richer constraints and consequences
F3 institutional/social feedback
F4 long-horizon feedback
F5 optimization/scale only if measured
```

Simulation fidelity should not run materially ahead of player-facing exposure.

### One authoritative world

The authoritative implementation of world laws and outcomes exists once, in the C++ Simulation Core.

Forbidden parallel truths include client-side money/inventory/relationships, a simplified Godot economy, C# gameplay rules, or fake client simulation that slowly becomes real.

Presentation prediction/interpolation is allowed only when it cannot create authoritative outcomes.

### Scope follows the capability

Do not start unrelated refactors, speculative frameworks or neighboring subsystems because they are convenient to touch. Build only what the current capability requires.

## Player role is compositional

There is no authoritative `PlayerClass` enum that grants the player the role of farmer, merchant, mage, noble, criminal, priest or politician.

Roles emerge from combinations of:

- skills and knowledge;
- property, capital, stock and tools;
- rights, licenses and legal status;
- household ties;
- organization membership and office;
- reputation, relationships, debts and obligations;
- territory/access;
- magic access;
- actions and history.

A UI role label is a projection, not the source of permissions.

### Player/NPC symmetry

If the player can buy land, own a workshop, hire someone, join an organization, become a magic apprentice, hold office, bribe, trade, create debt or violate a law, the capability must follow from world rules rather than `actor.is_player`.

NPC decision algorithms may differ from player input, but constraints and consequences are shared.

## Opportunity-driven RPG

Available actions arise from world and actor state:

```text
world state + actor state + institution rules
  -> feasible opportunities
```

Do not use a global abstract skill tree to unlock the world independently of its institutions and material constraints.

## Early scope guard

Do not start with a 3D open world, MMO networking, LLM NPCs, universal GOAP/ECS frameworks, full genetics/metabolism, a global political simulator, full religion simulator, universal magic ontology, custom scripting language, microservices, database clusters, or a revived TypeScript/WASM client.

Any of these may be considered later only after a concrete player-facing need, measured constraint and (when consequential) an ADR.

## First playable experience

The first serious slice is a small village around one real shortage and one magic counterfactual.

The world contains a few households/NPCs, homes, one production place, one exchange/social place, one short resource chain, one local authority and the player.

The player can walk, talk, observe the local problem, carry/transfer items, work or help, complete a simple trade, use one magic capability and see at least one persistent social or institutional consequence.

NPCs have a real need/obligation, choose a feasible task, move, work, transfer/consume/produce a resource, respond to shortage and reject impossible actions.

After roughly one short play session the player should understand who lives here, what happens without them, what the local problem is, what choices are available, why the choices differ and what changed because of the choice.

Detailed sequencing lives in [`ROADMAP.md`](ROADMAP.md).

## Definition of product success

The project is moving in the right direction when successive small capabilities let the player increasingly live, work, trade, own, learn, cast magic, negotiate, deceive, help, conflict, join organizations, gain/lose status, influence institutions and alter economic flows — all as consequences of one common authoritative world state rather than bespoke game exceptions.

> Simulation does not precede the game. The game is the continuous way the simulation is built, tested and deepened.
