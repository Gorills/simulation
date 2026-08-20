# Milestone 3 — Playable Social Consequence

**Specification status:** target contract for the active Milestone 3 capability.

Milestone 3 is not a generic relationship system and not a presentation-polish pause. It is the first short playable vignette in which an earlier material choice toward an identifiable person/household changes a later authoritative opportunity.

The milestone must reuse the accepted M1/M2 world instead of replacing it with a new scenario framework.

## Product outcome

The accepted M2 shortage becomes a situation about a specific neighbour rather than a resource test.

A short household can become short without player intervention. The player may materially help through an existing M2 path or may decline to help. The relevant household/NPC remembers the contribution in authoritative Simulation state. Later, a concrete household opportunity is available or refused because of that remembered social state.

The minimum playable loop is:

```text
shortage appears without player input
  -> player recognizes the affected neighbour/household
  -> player chooses whether/how to help
  -> authoritative material consequence occurs
  -> authoritative social memory changes
  -> time/action advances to a later opportunity
  -> the same neighbour/household responds differently because of the earlier choice
```

This is the first milestone where acceptance is about one coherent game situation, not merely proving isolated command paths.

## Vignette: a favour remembered

The bounded M3 vignette is built on the M2 acceptance village.

1. The short household autonomously develops the existing grain shortage.
2. The player can see which household is affected and can distinguish the relevant neighbour from the other villagers without reading entity IDs or opening technical diagnostics.
3. The player may help through at least one existing M2 material path. Gift, field work, and the standing household transfer remain valid M2 paths; M3 must not create a fake social-only "help" button that bypasses them.
4. A real accepted material contribution records a bounded social consequence for the contributing actor. A refused or impossible action does not create social credit.
5. A control path also exists in which the actor reaches the later social check without earning the relevant remembered aid. Verification must keep the later material prerequisites comparable enough that the contrasting outcome cannot be explained by stock, occupancy, or another non-social failure.
6. Later, the player can ask the previously affected household for one bounded reciprocal aid opportunity. The request succeeds only when the authoritative social condition is present and all material prerequisites are still valid; otherwise it is refused without mutation.
7. The successful reciprocal aid must change existing authoritative world state through a shared actor-generic rule. The client may request the action and present the result, but it may not own the relationship or manufacture the reward.

The exact quantity/bounds of reciprocal aid are implementation fixtures, not historical claims and not owned by this document. The detailed mechanic semantics belong in the M3 model document once implementation starts.

## Social dimension

The first useful social dimension is **bounded obligation / remembered material aid**.

It represents that a household has a concrete reason to reciprocate toward an actor who materially helped it during the shortage. It is intentionally narrower than generic friendship, affection, faction reputation, morality, fame, relationship graphs, or natural-language memory.

Required semantics:

- the remembered consequence identifies the relevant actor and social counterparty; it is not a global player reputation flag;
- only an accepted material contribution can create/increase the M3 obligation;
- refusal/no-op cannot create social state;
- the later reciprocal opportunity checks current authoritative social state and current material feasibility;
- consuming the bounded reciprocal opportunity changes or clears the corresponding obligation according to the mechanic contract;
- the same rule is available to any actor with equivalent state/prerequisites. NPC policy does not need to exercise every path in the Godot client during M3, but native evidence must prove no player-only domain branch exists;
- the M3 social state is authoritative World state and therefore must be captured/restored by the versioned `WorldSnapshot` contract under ADR 0008; derived projections/views are not snapshot truth.

Do not prebuild a universal relationship ontology. If a later capability needs trust, reputation, kinship, status, debt, patronage, hostility, legitimacy, or information provenance, add the smallest new dimension when that capability requires it.

## Player-facing identity and world readability

The vignette must communicate through the game world and ordinary interaction presentation, not through technical diagnostics alone.

Minimum presentation requirements:

- the affected household has a stable human-readable identity in ordinary play;
- the relevant NPC is distinguishable from other villagers by a stable ordinary-play cue;
- the household store/work location is legible as a place with a world meaning, not only as an unexplained coloured collision/occupancy rectangle;
- the player can tell that the shortage concerns that household;
- after helping, the player can tell that the neighbour/household recognized the contribution;
- at the later opportunity, acceptance/refusal is understandable as a consequence of the earlier relationship state.

Greybox presentation is sufficient. Final art, character models, animation, voice, dialogue trees, and environment production are not required.

Coloured occupancy footprints, IDs, revisions, raw stock thresholds, and other systemic overlays remain valuable developer tools, but they are not the primary M3 player communication path.

## Debug and acceptance boundary

M3 keeps diagnostics, but acceptance must prove the vignette with technical diagnostics hidden.

The ordinary player-facing evidence must therefore run with the debug overlay off and must not require knowledge of exact fixture coordinates, entity IDs, scenario names, revisions, or coloured-pad conventions.

A separate diagnostic artifact may show overlays/IDs/state to prove authority and aid debugging. That diagnostic evidence supplements the player-facing artifact; it does not replace it.

Automated playtests may drive the real gameplay scene and semantic commands, but production gameplay behavior must not depend on a test scenario having pre-scripted the desired social outcome.

## Authority and layer ownership

M3 preserves existing project invariants:

- Simulation Core owns the authoritative social state and validates the reciprocal opportunity;
- protocol exposes only the semantic command/result/projection needed by the vignette;
- GDExtension translates the protocol contract;
- Godot presents identity, opportunity, remembered consequence, and refusal/success feedback;
- Godot does not calculate whether the household owes the actor anything;
- no `is_player` branch grants social privilege;
- M1 RestNeed and accepted M2 household-resource behavior remain valid unless the M3 capability exposes a real causal defect that must be fixed in the same bounded slice.

## Development rule for M3

The unit of implementation is one playable beat, not one horizontal subsystem layer.

A bounded M3 implementation slice should normally cross only the layers needed to make the next piece of this vignette real:

```text
material event
  -> authoritative social memory
  -> semantic projection/command
  -> ordinary-play feedback/opportunity
  -> targeted proof
```

Do not land several independent "relationship foundation" tasks that leave `main` with no new understandable player consequence. A short native-first enabler is acceptable only when its immediate consumer is part of the same bounded implementation task or the next explicitly blocked vertical slice.

## Acceptance

Milestone 3 is accepted only when bounded ordinary-play evidence demonstrates both branches of the social consequence, normally as two clean short runs from controlled initial state.

### Helped branch

- the shortage appears without a player economic command;
- the player can identify the affected neighbour/household;
- the player performs a real accepted M2 material contribution;
- ordinary play communicates that the contribution was recognized;
- a later reciprocal-aid opportunity is available;
- taking that opportunity changes authoritative world state;
- the outcome is caused by authoritative M3 social state rather than a quest flag, scenario branch, or Godot-owned variable.

### Control branch

- the actor reaches the same later opportunity without the qualifying remembered aid;
- the reciprocal request is unavailable/refused or otherwise materially different for the social reason defined by the mechanic contract;
- verification proves the material prerequisites are otherwise satisfiable, so the differing outcome is not merely caused by empty stock, wrong occupancy, exhausted content, or another unrelated refusal;
- the refusal leaves authoritative state unchanged.

### Experience gate

With technical diagnostics hidden, a human playthrough should be able to answer, from ordinary game feedback:

1. Who has the problem?
2. What can I do about it?
3. Did my chosen action materially help?
4. Did that person/household remember it?
5. What later opportunity changed because of what I did?

If those answers require entity IDs, raw projections, fixture-coordinate knowledge, or reading the automated scenario implementation, M3 is not accepted even if native/protocol tests are green.

Exact commands, test inventory, artifact filenames, fixture quantities, and collected evidence belong in [`../VERIFICATION.md`](../VERIFICATION.md) as implementation lands.

## Explicit non-goals

M3 does **not** require:

- a generic relationship graph;
- multiple reputation axes;
- dialogue trees or an LLM conversation layer;
- procedural quests;
- a universal event-memory framework;
- friendship/romance/morale systems;
- faction reputation;
- offices, law, permissions, or politics from M4;
- trade/coin/market pricing;
- final village art or production character assets;
- replacing the current authoritative locomotion/resource architecture;
- generalizing M2 grain/carry into a universal inventory.

The milestone exists to make one earlier choice matter later in a way the player can understand and care about. Broader social simulation grows from that proven loop rather than preceding it.
