# Runtime architecture

This document owns **runtime boundaries, dependency direction, ownership and integration seams**. It does not own product goals, modeling policy, verification procedure or tool versions.

Related canonical owners:

- product and playable invariants: [`PRODUCT.md`](PRODUCT.md)
- modeling/determinism: [`MODELING.md`](MODELING.md)
- verification/playtest evidence: [`VERIFICATION.md`](VERIFICATION.md)
- milestones: [`ROADMAP.md`](ROADMAP.md)
- stack implementation guidance: [`engineering/STACK.md`](engineering/STACK.md)
- Simulation ↔ Godot implementation route: [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md)
- Godot/GDExtension versions: [`engineering/VERSIONS.md`](engineering/VERSIONS.md)
- consequential decisions: [`decisions/`](decisions/)

Current source, build configuration, executable tests and lock files are authoritative for actual implemented behavior. A diagram is not proof that a planned path or target already exists.

## Runtime dependency graph

The graph is one-way:

```text
Godot 4 presentation client
    |
    v
GDExtension adapter
    |
    v
Application protocol
    |
    v
C++23 Simulation Core

Native tools/tests ---> Application protocol / Simulation Core
Content data -------> Simulation Core
```

Hard direction:

```text
godot/ -> src/adapters/gdextension -> src/protocol -> src/sim

src/sim      !-> src/protocol / Godot / godot-cpp / GDExtension
src/protocol !-> Godot / godot-cpp / GDExtension
godot/       !-> authoritative world state or systemic outcomes
```

Folder names do not enforce architecture. The CMake target graph is the primary executable expression of the dependency direction.

## Implemented native target graph

```text
sim_core_tests ---------> sim_core
                              ^
                              |
protocol_tests --------> sim_protocol
                              ^
                              |
world_sim_gdextension -------+----> godot::cpp
```

`sim_protocol` privately depends on `sim_core`; `sim_core` has no dependency on protocol or Godot.

The current executable native smoke path is deliberately named as temporary:

```text
BootstrapMoveIntent(dx, dy)
  -> protocol::Simulation validation
  -> protocol bootstrap control binding -> EntityId{1}
  -> sim::World::apply_bootstrap_step(EntityId, CardinalDirection)
  -> BootstrapActorProjection(entity_id, tick, revision, ...)
```

Malformed transport values are rejected before the domain is called. `GridPosition`, `CardinalDirection`, `BootstrapMoveIntent`, `apply_bootstrap_step` and `BootstrapActorProjection` are Milestone 0 transport evidence, **not** the production third-person spatial contract.

## Ownership by layer

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | entity identity/existence, world state, locations when implemented, inventory/economy/social/political/magic/combat laws, deterministic outcomes, simulation time, seeded RNG | protocol DTOs, Godot types, input devices, render frames, UI, camera state |
| `src/protocol` | control/session binding, commands/intents, validation/translation, results, events, purpose-built projections, versioned boundary DTOs | rendering, scene-node state, duplicated domain rules, mutable exported `WorldState` |
| `src/adapters/gdextension` | Godot-facing ↔ protocol translation, registration, diagnostics | direct world ownership, prices, relationships, combat resolution, authoritative spawning or movement decisions |
| `godot/` | input sampling, presentation replicas, interpolation/prediction state, scenes, camera, audio, animation, VFX, UI/design system | authoritative entity existence/location, inventory, economy, relationships, ownership, access rights, trade/damage/politics/magic outcomes |
| native tools/tests | scenarios, diagnostics, verification, developer orchestration | a second simulator or alternate gameplay implementation |

The authoritative world exists once: in the C++ Simulation Core.

Godot may predict or interpolate a representation for responsiveness. Prediction is a disposable presentation hypothesis; authoritative samples/results reconcile it and all systemic outcomes remain Simulation-owned.

See [`decisions/0004-authoritative-world-presentation-boundary.md`](decisions/0004-authoritative-world-presentation-boundary.md).

## Player/NPC parity

The Simulation Core does not have a privileged player species.

A human-controlled person and an NPC are simulated actors with stable `EntityId`s. What differs is the source that produces their intent:

```text
PlayerControls -> protocol/session binding --+
                                           |
NPC decision -------------------------------+-> same authoritative action/rule path
```

Human control is a relationship outside the actor's world identity. It must not grant alternate economy, inventory, relationship, institution, ownership, law or combat APIs.

If an NPC can open a shop, buy an item, attack a traveler, join an institution or acquire property, a human-controlled actor should use the same domain capability when the same prerequisites hold.

## Domain API quality bar

Simulation code exposes semantic domain operations and types rather than transport-shaped primitives.

The implemented foundation establishes durable distinctions:

- `EntityId` is stable simulated identity and does not mean “player id”;
- `SimulationTick` is world time progression;
- `WorldRevision` is authoritative state ordering and may change without time advancing;
- `WorldSeed` is deterministic random-state provenance, not time or identity;
- `World` stores actors by identity instead of owning a `player_position_` field;
- bootstrap grid/cardinal types and operations are explicitly named as temporary probes.

A command changing an actor does not automatically mean time advanced. Time changes only through explicit simulation advancement.

Do not introduce a strong type merely to wrap every scalar. Add one when it prevents mixing different domain meanings, removes invalid states, or makes an authoritative contract materially clearer.

## Protocol boundary: writes vs reads

Clients express **intent**, never desired systemic state:

```text
Input/UI Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents
  -> read-only Projections
```

Commands mutate through world rules. Projections answer presentation needs without granting mutation access.

`protocol::Simulation` may validate/translate boundary data, bind an external controller to an actor and orchestrate calls into `sim`; it must not become a second home for world rules.

The protocol is a small application contract, not an exported `WorldState`. Internal Simulation types do not automatically become public/client types.

Breaking protocol changes update the explicit protocol version and affected native/client verification together.

See [`MODELING.md`](MODELING.md#protocol-semantics) and [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md).

## Purpose-built projections

Godot reads projections shaped for a presentation task. Likely families as real mechanics arrive include:

```text
ObservedWorldProjection
ControlledActorProjection
ShopProjection
InventoryProjection
RelationshipProjection
InstitutionProjection
JournalProjection
```

Do not introduce one universal projection containing every person, secret relationship, inventory, market, law and internal subsystem field.

A projection exposes only information the client is allowed to know. A shop UI may see offered stock/price without receiving hidden merchant knowledge or unrelated world state.

## Domain events

Domain events describe facts that happened and support feedback, explanation, tests and projection updates.

Examples may include `ItemTransferred`, `TradeCompleted`, `ActorArrived`, `AttackStarted`, `ActorWounded`, `RelationshipChanged`, `OfficeChanged`, `LawViolated` and `MagicEffectApplied` once those mechanics exist.

Events are not automatically the persistence model. This project does not adopt full event sourcing without a separate demonstrated need.

## Observed/materialized world

World existence and Godot scene-node lifetime are different concepts.

```text
full authoritative Simulation world
  -> Simulation/protocol observation + materialization policy
      -> bounded projection for the controlled context
          -> Godot presentation replica keyed by EntityId
```

Godot materializes, updates and dematerializes representations according to projections. Dematerializing an NPC because the player left town does not delete or pause the simulated NPC. Returning later materializes the **current** resulting state after offscreen simulation.

Keep these concepts separate:

- authoritative existence;
- semantic/spatial location;
- actor knowledge/visibility;
- presentation materialization;
- camera frustum/occlusion.

Godot may perform extra visual culling. Visual culling does not decide world existence or reveal hidden information.

## Offscreen simulation

A settlement continues in Simulation when it is not represented in Godot.

Economy, relationships, politics, schedules, violence, magic and other implemented causal systems continue according to Simulation time/rules. If the player returns, presentation shows current projections: changed stock, residents/locations, injuries/deaths, institutional changes, prices or other actual consequences.

Simulation-internal fidelity/LOD may be introduced later only from profiling evidence. It must preserve authoritative semantics and never delegate distant-world outcomes to Godot.

## GDExtension seam

Godot crosses into native gameplay through exactly one runtime seam. The adapter should be deliberately boring:

1. receive a semantic client request;
2. translate Godot-facing values into protocol values;
3. invoke protocol/application behavior;
4. translate results/projections/events back to Godot-facing values;
5. expose diagnostics without embedding world rules.

The current `SimFacade` owns `protocol::Simulation`, exposes bootstrap `submit_move` and read-only `debug_projection`, and converts only protocol projections into Godot dictionaries.

If a systemic gameplay rule is implemented inside a `GDCLASS`, GDScript node, UI script or serialization helper, the boundary is violated.

The adapter may depend on godot-cpp. `src/sim` and `src/protocol` may not.

## Godot client architecture

Godot is the interactive presentation client, not the authoritative simulator.

The implemented control/presentation graph is currently:

```text
InputMap
  -> PlayerControls + ControlProfile
       -> ThirdPersonPlayer + LocomotionProfile   # presentation/prediction shell
       -> ThirdPersonCameraRig
            -> SpringArm3D -> Camera3D
```

ADR 0004 supersedes the earlier interpretation that local `CharacterBody3D` state is authoritative. Production locomotion must migrate toward:

```text
semantic player input
  -> actor intent
  -> authoritative spatial simulation
  -> revisioned movement samples/projection
  -> Godot interpolation / optional prediction + reconciliation
```

The exact authoritative spatial/collision representation is intentionally not selected yet; it must be chosen from actual terrain/navigation/determinism/performance constraints.

Detailed presentation guidance is in [`engineering/godot.md`](engineering/godot.md); cross-boundary guidance is in [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md).

## UI design-system boundary

The Godot UI has one project-wide visual source of truth:

```text
godot/ui/design_system/world_theme.tres
```

Feature scenes consume semantic Theme variations and compose layout with Godot Containers. Static colors, typography sizes, StyleBoxes, focus treatment and common spacing do not belong in feature scenes as copied local overrides.

The logical desktop baseline is 1920×1080 with `canvas_items` + `expand`.

See [`decisions/0003-project-wide-ui-design-system.md`](decisions/0003-project-wide-ui-design-system.md) and [`engineering/ui-design-system.md`](engineering/ui-design-system.md).

## Example: merchant transaction

```text
Simulation: merchant exists, is present, owns stock, offers apples at price P
  -> ShopProjection
  -> Godot renders stock/price
  -> BuyItem intent
  -> Simulation revalidates location/stock/funds/access
  -> authoritative money/item transfer + events + new projections
  -> Godot updates presentation
```

The UI cannot make an apple appear in inventory by changing a GDScript array. NPC customers use the same transaction rule path.

## Example: bandit attack

Attackers, victims, hostility, movement, damage, death, loot, law/social consequences and resulting state are Simulation concerns.

If participants are in the current materialization projection, Godot renders movement/combat/FX/audio and sends player intervention as intent. If they are offscreen, Simulation resolves the event without scene nodes. Returning later materializes the consequences.

## Vertical capability rule

```text
minimal world rule
  -> semantic command/result/events/projection
  -> GDExtension translation
  -> Godot affordance/feedback
  -> focused deterministic/regression proof
  -> bounded real playtest
```

A coherent capability may touch several layers. It must not create a second implementation of the same rule in Godot.

## External AI Layer boundary

`Gorills/ai-layer` is the development control plane, not a runtime/build dependency of the game.

- no repository-local `.ai-layer/` durable state;
- no copied Work/Task/Epic lifecycle or registry/database state;
- no runtime/build dependency on `ai-layer`;
- repository source/tests/docs own game contracts; AI Layer may own external development workflow/context.

See [`AGENT_CONTEXT.md`](AGENT_CONTEXT.md).

## Mechanical architecture verification

Current executable structure establishes the first load-bearing boundaries:

- separate `sim_core` and `sim_protocol` targets encode protocol -> Simulation direction;
- native domain tests link only `sim_core`;
- `World` stores actors by stable `EntityId` rather than a special player field;
- protocol/session owns the bootstrap human-control binding;
- `SimulationTick` and `WorldRevision` are distinct;
- protocol tests prove malformed transport input cannot mutate the projection;
- native tests prove different actor IDs use the same authoritative domain operation;
- bootstrap movement names explicitly discourage treating the grid probe as production locomotion;
- only `world_sim_gdextension` links godot-cpp;
- `tools/check_architecture.py`/CTest rejects direct Godot include markers in `src/sim` and `src/protocol`;
- the smoke playtest validates actor identity plus tick/revision in the native projection once run in the pinned local environment.

As the graph grows, prefer real target/API boundaries over prose-only rules. Add a narrow mechanical check only when a real dependency edge cannot already be expressed by code/build ownership.
