# Runtime architecture

This document owns **runtime boundaries, dependency direction, ownership and integration seams**. It does not own product goals, modeling policy, verification procedure or tool versions.

Related canonical owners:

- product and playable invariants: [`PRODUCT.md`](PRODUCT.md)
- modeling/determinism: [`MODELING.md`](MODELING.md)
- verification/playtest evidence: [`VERIFICATION.md`](VERIFICATION.md)
- milestones: [`ROADMAP.md`](ROADMAP.md)
- stack implementation guidance: [`engineering/STACK.md`](engineering/STACK.md)
- Simulation ↔ Godot implementation route: [`engineering/simulation-godot-boundary.md`](engineering/simulation-godot-boundary.md)
- authoritative spatial model: [`models/spatial-location.md`](models/spatial-location.md)
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

Folder names do not enforce architecture. The CMake target graph is the primary executable expression of dependency direction.

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

The Milestone 0 smoke path remains deliberately temporary:

```text
BootstrapMoveIntent(dx, dy)
  -> protocol::Simulation validation
  -> bootstrap control binding -> EntityId{1}
  -> sim::World::apply_bootstrap_step(EntityId, CardinalDirection)
  -> BootstrapActorProjection
```

`GridPosition`, `CardinalDirection`, `BootstrapMoveIntent`, `apply_bootstrap_step` and `BootstrapActorProjection` are transport evidence, **not** production third-person spatial state.

Production-shaped reads are separate:

```text
Simulation
  ├── observed_world_projection()
  │     -> ObservedWorldProjection
  │          identity/presence + tick/revision
  │
  └── controlled_actor_spatial_projection()
        -> ControlledActorSpatialProjection
             EntityId
             position (mm)
             velocity (mm/s)
             SpatialEpoch
             SimulationTick
             WorldRevision
        -> GDExtension mm -> Godot meters
        -> WorldPresentation initial placement
```

`ObservedWorldProjection` does not contain bootstrap grid coordinates or production spatial state. `ControlledActorSpatialProjection` is purpose-built for the exact-spatial controlled presentation.

The native application movement path is production-shaped and now emits an explicit ordered transition result:

```text
ControlledActorMoveIntent(x, z)
  -> protocol validation + controller/session intent state
  -> Simulation::advance_locomotion_tick()
  -> World::advance_grounded_locomotion_tick(actor-keyed batch)
  -> shared Godot-free grounded/fall solver
  -> GroundedLocomotionTickResult
       shared post-transition SimulationTick / WorldRevision
       GroundedLocomotionSample[] sorted by EntityId
  -> AuthoritativeMovementSampleBatch
       shared protocol tick / revision / version
       AuthoritativeMovementSample[] sorted by EntityId
```

Submitting controller intent does not itself advance world time or spatial state. One successful actor-keyed World batch advances `SimulationTick` and `WorldRevision` once regardless of actor count. The result is post-transition state, not a later polling snapshot. The GDExtension/Godot continuous movement seam remains pending, but it now has a defined ordered native sample contract to expose.

## Ownership by layer

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | entity identity/existence, semantic and exact spatial state when causal, inventory/economy/social/political/magic/combat laws, deterministic outcomes, simulation time, seeded RNG | protocol DTOs, Godot types, input devices, render frames, UI, camera state |
| `src/protocol` | control/session binding, commands/intents, boundary validation/translation, results, events, purpose-built projections, ordered presentation-facing sample DTOs, versioned DTOs | rendering, scene-node state, duplicated domain rules, mutable exported `WorldState` |
| `src/adapters/gdextension` | Godot-facing ↔ protocol translation, unit conversion, registration, diagnostics | direct world ownership, prices, relationships, collision resolution, authoritative spawning or movement decisions |
| `godot/` | input sampling, presentation replicas, interpolation/prediction state, scenes, camera, audio, animation, VFX, UI/design system | authoritative entity existence/location, inventory, economy, relationships, ownership, access rights, trade/damage/politics/magic outcomes |
| native tools/tests | scenarios, diagnostics, verification, developer orchestration | a second simulator or alternate gameplay implementation |

The authoritative world exists once: in the C++ Simulation Core.

Godot may predict or interpolate a representation for responsiveness. Prediction is a disposable presentation hypothesis; authoritative samples/results reconcile it and all systemic outcomes remain Simulation-owned.

See ADR 0004 and [`decisions/0006-authoritative-spatial-contract.md`](decisions/0006-authoritative-spatial-contract.md).

## Player/NPC parity

The Simulation Core does not have a privileged player species.

A human-controlled person and an NPC are simulated actors with stable `EntityId`s. What differs is the source that produces intent:

```text
PlayerControls -> protocol/session binding --+
                                           |
NPC decision -------------------------------+-> same authoritative action/rule path
```

Human control is a relationship outside the actor's world identity. It must not grant alternate economy, inventory, relationship, institution, ownership, law, combat or movement rules.

For locomotion, the shared Core seam is concrete: `World::advance_grounded_locomotion_tick()` consumes an actor-keyed batch. Tests prove two actors can move in one world tick, invalid batches do not partially mutate actors, and returned samples are canonically ordered by ascending `EntityId` even when input intent order is reversed. The protocol binds only the human-controlled actor to that generic operation today. A real NPC decision source feeding the same batch remains a later integration proof, not a separate movement law.

If an NPC can open a shop, buy an item, attack a traveler, join an institution, acquire property or move through a place, a human-controlled actor uses the same world capability when the same prerequisites hold.

## Domain API quality bar

Simulation code exposes semantic domain operations and types rather than transport-shaped primitives.

Durable distinctions now include:

- `EntityId` — stable simulated identity;
- `SimulationTick` — world-time progression;
- `WorldRevision` — authoritative mutation ordering;
- `WorldSeed` — deterministic random-state provenance;
- `SpatialPosition` — signed 64-bit millimeters in Simulation X/Y/Z space;
- `SpatialVelocity` — signed 64-bit millimeters per second;
- `SpatialEpoch` — continuity identity for interpolation/snap semantics;
- `SpatialState` — optional exact spatial state when current causal fidelity requires it;
- `PlanarMoveIntent` — bounded semantic X/Z movement intent, never displacement or final state;
- `GroundedLocomotionContinuation` — hidden per-actor fixed-step remainder/tick-rate state that affects the next authoritative movement result and therefore belongs to snapshot truth;
- `GroundedLocomotionTickResult` — one post-transition temporal batch with actor samples canonically ordered by `EntityId`;
- bootstrap grid/cardinal types — explicitly temporary transport probes.

A command changing an actor does not automatically mean time advanced. A world revision changing does not automatically mean spatial position changed. A spatial epoch changing means presentation must treat the relocation as discontinuous. Controller intent submission is specifically non-mutating; the fixed locomotion tick is the world-time mutation boundary.

Do not introduce a strong type merely to wrap every scalar. Add one when it prevents mixing different meanings, removes invalid states or makes an authoritative contract materially clearer.

## Authoritative spatial contract

The exact coordinate representation and the first bounded grounded locomotion/collision solver are selected. The **production content-location geometry/query representation** is not.

Simulation exact 3D coordinates use:

```text
X/Y/Z signed int64 millimeters
velocity signed int64 millimeters/second
Y is up
right-handed X/Y/Z orientation aligned with Godot
```

Godot 4.7 uses one 3D unit = one meter, so GDExtension performs the unit conversion. Godot `Vector3` never becomes a Simulation type.

Exact state is selective:

```text
entity exists != entity has SpatialState != Godot node exists
```

The controlled actor in the active 3D session has exact spatial state. A distant or aggregate-resolved actor may remain authoritative without exact pose when current mechanics do not require one.

`SpatialEpoch` changes on teleport/respawn/discontinuous transfer. Samples across different epochs are not interpolated as ordinary motion.

There is deliberately no general `SetPosition`/`SetVelocity` world API. Implemented continuous movement writes are semantic actor intents whose resulting position/velocity are decided by the Simulation solver. The current neutral `GroundPatch`/`VerticalBarrier` vectors and flat protocol acceptance context prove the transition; they are not yet a production large-world collision/index format or a declaration that Godot scene colliders are authoritative.

Detailed causal contract: [`models/spatial-location.md`](models/spatial-location.md) and [`models/grounded-locomotion.md`](models/grounded-locomotion.md).

## Protocol boundary: writes vs reads

Clients express **intent**, never desired systemic state:

```text
Input/UI Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents / ordered samples
  -> read-only Projections
```

Commands mutate through world rules. Projections answer presentation needs without granting mutation access. Ordered movement samples are transition results, not mutable world objects and not polling aliases for projections.

`protocol::Simulation` may validate/translate boundary data, bind an external controller to an actor and orchestrate calls into `sim`; it must not become a second home for world rules.

The protocol is a small application contract, not an exported `WorldState`. Internal Simulation types do not automatically become public/client types.

The shared application protocol version lives in `src/protocol/version.hpp`. Breaking boundary changes update that version and affected native/client evidence together. The ordered movement sample batch is currently additive at the native protocol layer and is not yet exposed through GDExtension, so the existing runtime protocol version remains 4 in this slice; the bridge version is reviewed when the new method becomes client-facing.

Movement uses `ControlledActorMoveIntent{x,z}` at the native application boundary. `submit_controlled_actor_move_intent()` validates/replaces controller state but does not mutate `World`; `advance_locomotion_tick()` applies the stored intent through the shared actor-generic World batch and returns `AuthoritativeMovementSampleBatch`. `SetNpcPosition`, `SetPlayerTransform`, client-authored velocity and final-displacement setters remain invalid world APIs.

`AuthoritativeMovementSampleBatch` has one post-transition tick/revision envelope and samples sorted by ascending `EntityId`. Each sample carries identity, position, velocity and `SpatialEpoch`. Across movement batches, locomotion tick supplies temporal order; world revision locates the batch relative to other authoritative mutations. No extra sequence counter is introduced without demonstrated need.

The current movement API is not yet exposed by GDExtension. The next bridge slice should translate this batch directly rather than polling `ControlledActorSpatialProjection` as an underspecified frame stream.

## Purpose-built projections

Godot reads projections shaped for a presentation task. Implemented durable read models now include:

```text
ObservedWorldProjection
ControlledActorSpatialProjection
```

Likely additional families as real mechanics arrive include:

```text
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

Events are not automatically the persistence model. This project does not adopt full event sourcing without a demonstrated need.

## Observed/materialized world

World existence and Godot scene-node lifetime are different concepts.

```text
full authoritative Simulation world
  -> observation policy
      -> bounded ObservedWorldProjection
          -> WorldPresentation keyed by EntityId
              -> presentation nodes / bindings
```

`WorldPresentation` validates monotonic world revisions, tracks observed IDs and binds the controlled `Player` representation through `EntityBinding`. It also performs the **initial** controlled presentation placement from `ControlledActorSpatialProjection`.

That initialization API is intentionally not a continuous movement updater. Repeated authoritative movement now has a separate native sample-batch contract and must later go through a GDExtension translation + Godot sample buffer/interpolator/reconciliation path.

Future materialization may instantiate/update/dematerialize typed presentation scenes when real NPC/item capabilities exist. Removing a Godot representation must never delete the simulated entity.

Keep distinct:

- authoritative existence;
- semantic location;
- exact `SpatialState`;
- actor knowledge/visibility;
- presentation materialization;
- camera frustum/occlusion.

## Offscreen simulation and causal fidelity

A settlement continues in Simulation when it is not represented in Godot.

Economy, relationships, politics, violence, magic and other implemented causal systems continue according to Simulation rules/time. Exact spatial resolution is retained only where current causality needs it; this is a model decision under ADR 0005, not a camera-distance performance hack.

Presentation materialization, causal model resolution and runtime performance architecture remain separate concerns.

## GDExtension seam

Godot crosses into native gameplay through exactly one runtime seam. The adapter should be deliberately boring:

1. receive a semantic client request;
2. translate Godot-facing values into protocol values;
3. invoke protocol/application behavior;
4. translate results/projections/events back to Godot-facing values;
5. expose diagnostics without embedding world rules.

The current `SimFacade` exposes:

```text
observed_world_projection()             # identity/presence read
controlled_actor_spatial_projection()  # authoritative exact-spatial read
bootstrap_submit_move(dx, dy)           # Milestone 0 probe only
bootstrap_debug_projection()            # Milestone 0 diagnostics only
```

Spatial unit conversion belongs here: integer millimeters in protocol become meter-space Godot `Vector3` values. Collision or movement decisions do not.

The native semantic locomotion API and ordered `AuthoritativeMovementSampleBatch` intentionally stop at `src/protocol` in the current Stage C2 slice. The next bridge task can expose that existing batch through GDExtension and establish continuous Godot consumption without inventing another movement stream contract.

If a systemic gameplay rule is implemented inside a `GDCLASS`, GDScript node, UI script or serialization helper, the boundary is violated.

## Godot client architecture

Godot is the interactive presentation client, not the authoritative simulator.

Current graph:

```text
InputMap
  -> PlayerControls + ControlProfile
       -> ThirdPersonPlayer + LocomotionProfile   # current local presentation shell
       -> ThirdPersonCameraRig
            -> SpringArm3D -> Camera3D

SimFacade.observed_world_projection()
  -> WorldPresentation
       -> EntityBinding -> Player presentation

SimFacade.controlled_actor_spatial_projection()
  -> WorldPresentation.initialize_controlled_spatial_presentation()
       -> initial Player meter-space position
       -> reset_physics_interpolation()
```

`WorldPresentation` is the Godot owner of authoritative presentation identity/presence. Feature scripts must not create parallel `EntityId -> Node` registries or assign authoritative IDs themselves.

The current `ThirdPersonPlayer.move_and_slide()` result is still local presentation/prototype movement. It must not be copied back into Simulation as authoritative location.

The native path now exists through semantic controlled intent -> actor-generic World batch -> deterministic Simulation movement/collision -> ordered authoritative sample batch. The next movement stage is therefore:

```text
AuthoritativeMovementSampleBatch
  -> GDExtension translation
  -> Godot EntityId-keyed sample buffer
  -> monotonic tick/revision validation
  -> SpatialEpoch-aware interpolation / reconciliation
  -> remove duplicate local world-law movement
  -> optional prediction only if measured latency requires it
```

The production collision/navigation representation is intentionally **not selected yet**. It must be chosen from the first real terrain/reachability requirement and tested in Godot-free native code; the current neutral acceptance vectors are not a large-world geometry architecture.

Godot's floating-point world precision is also presentation-side. Simulation's int64 millimeter coordinates do not require enabling Godot large-world coordinates now; future rebasing/double precision can be added if measured world scale requires it.

## UI design-system boundary

The Godot UI has one project-wide visual source of truth:

```text
godot/ui/design_system/world_theme.tres
```

Feature scenes consume semantic Theme variations and compose layout with Godot Containers. Static colors, typography sizes, StyleBoxes, focus treatment and common spacing do not belong in feature scenes as copied local overrides.

The logical desktop baseline is 1920×1080 with `canvas_items` + `expand`.

See ADR 0003 and [`engineering/ui-design-system.md`](engineering/ui-design-system.md).

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

Attackers, victims, hostility, authoritative movement, damage, death, loot, law/social consequences and resulting state are Simulation concerns.

If participants are materialized, Godot renders movement/combat/FX/audio and sends player intervention as intent. If they are offscreen, Simulation resolves the implemented causal event without scene nodes.

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

Current executable structure establishes load-bearing boundaries:

- separate `sim_core` and `sim_protocol` targets encode protocol -> Simulation direction;
- native domain tests link only `sim_core`;
- `World` stores actors by stable `EntityId` rather than a special player field;
- protocol/session owns the current human-control binding;
- `SimulationTick`, `WorldRevision` and `SpatialEpoch` have separate meanings;
- exact `SpatialState` is optional and Godot-free;
- actor-generic `World::advance_grounded_locomotion_tick()` applies multiple actor intents atomically and advances time/revision once per batch;
- `GroundedLocomotionTickResult` returns post-transition samples canonically sorted by `EntityId`, independent of intent collection order;
- fixed-step locomotion continuation that affects subsequent state is captured by `WorldSnapshot` schema v2 and deterministic restore continuation tests;
- protocol movement tests prove semantic intent submission does not mutate world state, fixed locomotion ticks produce `AuthoritativeMovementSampleBatch`, and repeated batches increase tick/revision monotonically;
- `ObservedWorldProjection` and `ControlledActorSpatialProjection` remain separate purpose-built reads rather than the continuous movement stream;
- native tests prove exact-spatial and non-exact actors can coexist and invalid spatial epoch cannot mutate the world;
- protocol tests prove bootstrap grid movement changes revision but not production spatial position;
- Godot has one `WorldPresentation` identity/presence owner and initializes the controlled representation from an authoritative spatial sample;
- bootstrap movement names discourage treating the grid probe as production locomotion;
- only `world_sim_gdextension` links godot-cpp;
- `tools/check_architecture.py`/CTest rejects direct Godot include markers in `src/sim` and `src/protocol`;
- smoke evidence is designed to prove bootstrap, observed identity, authoritative spatial sample and presentation initialization independently.

As the graph grows, prefer real target/API boundaries over prose-only rules. Add a narrow mechanical check only when a real dependency edge cannot already be expressed by code/build ownership.
