# ADR 0009: Simulation authority and decision-source boundaries

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`0004-authoritative-world-presentation-boundary.md`](0004-authoritative-world-presentation-boundary.md) · [`0005-historical-counterfactual-and-causal-fidelity.md`](0005-historical-counterfactual-and-causal-fidelity.md) · [`0006-authoritative-spatial-contract.md`](0006-authoritative-spatial-contract.md) · [`../engineering/simulation-godot-boundary.md`](../engineering/simulation-godot-boundary.md)

## Context

The project deliberately owns one authoritative C++ Simulation world while Godot presents and controls that world through protocol/GDExtension boundaries.

Two future implementation mistakes would preserve the words of that architecture while violating its intent:

1. treating "Simulation owns authoritative spatial outcomes" as a requirement to hand-write every collision, navigation and character-physics primitive forever;
2. treating a future language model as an authoritative NPC "brain" that owns world facts, mutates actors directly, receives omniscient world state or is queried inside the fixed simulation step.

A third scaling risk appears as the world becomes less local: authoritative existence does not require every distant entity to retain an exact 3D pose or to be advanced at locomotion frequency. The project needs persistent causal state, not universal microscopic simulation.

This ADR clarifies those boundaries before production-scale navigation, offscreen simulation or external AI is introduced. It does not choose a physics/navigation library and does not implement an LLM integration.

## Decision

### Simulation authority is ownership of world truth, not ownership of every algorithm implementation

Simulation remains the sole authority for consequences that become world facts: entity existence, location at the required causal fidelity, ownership, resources, relationships, institutions, damage, needs, task outcomes and other systemic state.

Godot remains presentation/input/UI/audio/animation/VFX infrastructure. It may smooth or predict presentation, but it does not create authoritative outcomes.

This authority boundary does **not** require every low-level spatial algorithm to be project-authored code.

A future exact-spatial subsystem may use:

- project-owned deterministic code;
- a specialized Godot-free C++ physics/collision library;
- a specialized Godot-free navigation/pathfinding library;
- a bounded combination of those approaches.

A third-party library is an implementation dependency inside the authoritative Simulation path, not a second world authority, when all of these remain true:

1. Simulation owns the authoritative inputs and resulting world state;
2. Godot scene nodes/colliders/navmeshes are not silently used as world truth;
3. the dependency is behind a narrow Simulation-owned contract rather than leaking engine types through the domain;
4. the required authoritative behavior can be exercised headlessly in native verification;
5. determinism/replay requirements are evaluated explicitly for the chosen dependency and use case;
6. the dependency is admitted because a concrete playable requirement needs it, not because a general engine might be useful later.

Therefore the current grounded solver proves the authority seam; it is not a commitment to grow into a general rigid-body, navigation or world-geometry engine.

### Exact spatial resolution is selective

Preserve the existing distinction:

```text
entity exists != entity has exact SpatialState != Godot node exists
```

Exact 3D state is required only when current causality depends on exact geometry, movement, reachability or interaction.

Offscreen or otherwise lower-resolution entities may continue through authoritative semantic/causal state without being advanced as 60 Hz physics bodies. Examples of future semantic state include being at a place, performing an activity, travelling between places, owing an obligation, participating in a transaction or waiting for a scheduled causal event.

When an entity later requires exact spatial materialization, Simulation must derive or restore a valid authoritative pose from the causal state and the location contract then in force. Godot may render that result; it must not invent the world fact simply because a scene was loaded.

No generic semantic-place, scheduler, event queue or regional LOD framework is introduced by this ADR. The first offscreen Milestone 1 requirement should admit only the minimum state/transition needed to prove continuation without a Godot representation.

### Actor decision source is separate from actor world identity

A human-controlled actor and an NPC are ordinary simulated actors. Control/decision policy is an input relationship, not a different set of world laws.

The durable shape is:

```text
human input --------------------+
                                |
deterministic NPC policy -------+-> structured proposed intent
                                |            |
future external policy/LLM -----+            v
                                           Simulation validation/rules
                                                    |
                                                    v
                                           authoritative result
```

Actor state must not gain provider-specific identity such as `ai_enabled`, `llm_provider`, `prompt`, `brain_model` or another field whose purpose is to bind a world entity to one decision technology.

World state contains facts about the actor. A decision source observes an allowed context and proposes an action. Simulation decides whether that action is valid and what consequence occurs.

### Future external/LLM decisions use high-level structured proposals

If a language model or another nondeterministic external policy is later introduced, it operates above deterministic world rules.

It may eventually help with tasks such as:

```text
player speech
  -> language interpretation / policy decision
  -> RequestFollow { requester, requested_actor }
  -> Simulation checks hearing/knowledge/relationship/authority/need/risk/etc.
  -> accept or refuse
  -> accepted FollowGoal { target = EntityId }
  -> navigation/local steering
  -> ActorGroundedMoveIntent
  -> shared World locomotion
```

The external policy must not output final positions, velocities, money balances, relationship values or other authoritative state mutations.

It should propose typed high-level intents/goals from an explicit allowlist. Simulation validates identity, prerequisites, permissions, current state and consequences exactly as it would for another intent source.

Low-level locomotion remains deterministic execution. An external model is not queried every 60 Hz tick to decide `PlanarMoveIntent`.

### External policy context is bounded by NPC knowledge

A future model receives a purpose-built `NpcDecisionContext`-style view, not a complete `World` snapshot.

The context may contain only facts the actor is allowed to perceive/remember/know plus the minimal decision-relevant state and available action vocabulary. This prevents an external policy from becoming a source of telepathic, hidden-system or meta-game knowledge.

The exact knowledge/memory representation is deferred until real social/knowledge mechanics exist. Do not introduce a vector database, generic memory framework or universal knowledge graph in anticipation of LLM use.

### External nondeterminism is recorded at the authoritative input boundary

An external model is nondeterministic input, like human input from outside the deterministic Core.

If replay/persistence requires reproducing a run, record the accepted structured proposal/intent that crossed into Simulation. Replay consumes the recorded structured input and does not re-query the external model.

Provider/model/version/request metadata may be retained for diagnostics when useful, but it is not the authoritative world result.

External output is untrusted input:

- schema/allowlist validation is mandatory;
- referenced entities/actions must be validated against current Simulation state;
- free-form text must never become a direct mutation command;
- provider failure/timeout must not corrupt or stall the world;
- a deterministic fallback or no-op behavior must remain possible.

### Fixed-step simulation never waits for an external model

No network/model call may block the authoritative fixed locomotion step or another time-critical Simulation transition.

External decisions happen at bounded decision points such as a new request/conversation, completion/failure of a goal, or another meaningful state change. The result is delivered later as an external structured input and admitted against the then-current authoritative state.

The precise async transport is deliberately deferred. This ADR only forbids synchronous provider dependency inside deterministic ticking.

### Re-admit tick/stream ordering before non-locomotion time advances independently

The current in-process movement bridge can require consecutive `SimulationTick` values because locomotion is currently the relevant time-advancing stream.

That assumption must not silently become a permanent presentation protocol invariant once slower systems, offscreen continuation, time acceleration or other world transitions advance simulation time independently of locomotion samples.

Before the first such production path is added, explicitly re-admit the ordering contract and choose the smallest correct result:

- retain `SimulationTick` if its semantics still make movement batches consecutive by construction;
- relax the presentation guard if gaps are valid and ordering remains unambiguous;
- or introduce a locomotion/result-stream sequence only if a concrete requirement proves tick + revision insufficient.

Do not add a second sequence counter now merely because it may be needed later.

### Current protocol scenario hardcoding is bounded acceptance scaffolding

The current first-Living-Need application scenario may hardcode a controlled actor, one NPC and its observed set while it proves the vertical capability.

Do not scale that pattern by adding a growing list of named actor fields/IDs and feature-specific branches to `protocol::Simulation`.

Re-admit scenario composition/observation ownership before the first change that needs dynamic population composition, a second independent NPC behavior scenario, or reusable location content. The replacement should be driven by that concrete requirement; this ADR does not authorize a generic entity framework, ECS or scenario DSL in advance.

## Consequences

### Positive

- the project keeps one authoritative world without committing to a hand-written general physics/navigation engine;
- offscreen persistence can scale by causal work rather than universal exact-spatial ticking;
- future LLM use has a compatible seam without contaminating actor state or world rules with provider concerns;
- deterministic replay can treat accepted external decisions as recorded inputs rather than trying to make an external model deterministic;
- prompt injection/provider failure cannot directly mutate authoritative state when the validation boundary is preserved;
- the current simple protocol scenario can remain simple until a real composition requirement appears;
- future time systems have an explicit compatibility gate before they break current movement ordering assumptions.

### Costs

- later spatial-library selection still requires research and native verification;
- offscreen materialization needs a real semantic-location/placement contract when that capability is implemented;
- useful grounded dialogue eventually requires actual knowledge, memory, relationship and world-action state rather than prompt-only simulation;
- external decisions require asynchronous delivery, validation and replay-input recording once implemented;
- some current acceptance scaffolding will intentionally be replaced when the next real scenario outgrows it.

## Deliberately not introduced

This decision does not introduce:

- an LLM provider or API client;
- `AIManager`, `LLMService`, prompt framework or provider abstraction;
- behavior trees, GOAP, a universal utility planner or generic NPC brain interface;
- vector storage, embeddings or generic memory infrastructure;
- a `FollowGoal` implementation or natural-language parser;
- a physics/navigation library choice;
- production collision geometry, navmesh or spatial index;
- a semantic place/travel schema;
- offscreen scheduler/event queue/regional LOD framework;
- an additional movement sequence counter;
- ECS, networking or sharding.

## Admission gates

Use the first real requirement to choose the next bounded implementation:

1. **Milestone 1 offscreen continuation:** add only the minimum authoritative causal state/transition needed for the current NPC need/task to continue without a Godot representation.
2. **First real route-around-obstacle requirement:** evaluate production navigation/geometry needs and research project-owned versus specialized Godot-free library implementation; do not extend the fixture solver speculatively.
3. **First future natural-language command:** implement the underlying deterministic shared action/goal and acceptance/refusal rules first; only then add optional language interpretation above that contract.
4. **First non-locomotion time-advancing production system:** re-admit `SimulationTick` versus movement-stream ordering before changing the presentation protocol.
5. **First scenario that outgrows the two-actor acceptance composition:** replace the hardcoded application fixture with the minimum real composition/observation owner required by that scenario.
