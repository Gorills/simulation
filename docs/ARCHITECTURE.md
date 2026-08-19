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

The Milestone 0 smoke path retains one deliberately temporary transport probe:

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

`ObservedWorldProjection` does not contain bootstrap grid coordinates or production spatial state. `ControlledActorSpatialProjection` is purpose-built for initial/debug exact-spatial reads of the controlled presentation.

The production controlled movement path crosses the full runtime boundary:

```text
PlayerControls camera-relative analog direction + semantic pace
  -> SimFacade.controlled_actor_submit_move_intent(x, z, pace)
  -> ControlledActorMoveIntent
  -> protocol validation + controller/session intent state
  -> SimFacade.advance_locomotion_tick()
  -> Simulation::advance_locomotion_tick()
  -> World::advance_grounded_locomotion_tick(actor-keyed batch)
       -> actor state + requested pace
       -> single World locomotion-capability resolver
       -> resolved move speed / acceleration / braking
       -> shared Godot-free grounded/fall solver
  -> GroundedLocomotionTickResult
       shared post-transition SimulationTick / WorldRevision
       GroundedLocomotionSample[] sorted by EntityId
  -> AuthoritativeMovementSampleBatch
       shared protocol tick / revision / version
       AuthoritativeMovementSample[] sorted by EntityId
  -> GDExtension mm/mm-s -> meter/m-s translation
  -> WorldPresentation ordered sample validation/reconciliation
  -> controlled Godot physics-root transform/velocity
```

Submitting controller intent does not itself advance world time or spatial state. Direction/pace intent never contains a requested meters-per-second value. One successful actor-keyed World batch advances `SimulationTick` and `WorldRevision` once regardless of actor count. The returned batch is post-transition state, not a later polling snapshot. Godot consumes it during the local fixed physics tick and never copies a scene collision result back into Simulation.

## Ownership by layer

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | entity identity/existence, semantic and exact spatial state when causal, actor locomotion capability, inventory/economy/social/political/magic/combat laws, deterministic outcomes, simulation time, seeded RNG | protocol DTOs, Godot types, input devices, render frames, UI, camera state |
| `src/protocol` | control/session binding, commands/intents, boundary validation/translation, results, events, purpose-built projections, ordered presentation-facing sample DTOs, versioned DTOs | rendering, scene-node state, numeric actor-capability policy, duplicated domain rules, mutable exported `WorldState` |
| `src/adapters/gdextension` | Godot-facing ↔ protocol translation, unit conversion, registration, diagnostics | direct world ownership, prices, relationships, collision resolution, speed resolution, authoritative spawning or movement decisions |
| `godot/` | input sampling, semantic pace selection, presentation replicas, sample ordering guards, interpolation/reconciliation state, scenes, camera, audio, animation, VFX, UI/design system | authoritative entity existence/location, actor movement limits, inventory, economy, relationships, ownership, access rights, trade/damage/politics/magic outcomes |
| native tools/tests | scenarios, diagnostics, verification, developer orchestration | a second simulator or alternate gameplay implementation |

The authoritative world exists once: in the C++ Simulation Core.

Godot may interpolate a representation for responsiveness. Local prediction is optional future presentation state only; if introduced, authoritative samples/results must reconcile it and all systemic outcomes remain Simulation-owned.

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

For locomotion, the shared Core seam is concrete: `World::advance_grounded_locomotion_tick()` consumes an actor-keyed batch of direction/magnitude + semantic pace. `ActorLocomotionCapability` belongs to authoritative actor state; a single World resolver turns capability + pace into the numeric solver limits. Tests prove two actors can move in one world tick, different actors can resolve the same pace from different capabilities, different paces resolve from the same capability, invalid batches do not partially mutate actors, and returned samples are canonically ordered by ascending `EntityId` even when input order is reversed.

Core provides `decide_npc_local_move_toward_waypoint()` for local steering and `decide_npc_rest_need()` for the first causal Milestone 1 producer. `RestNeedState` selects an already-assigned rest point; the task chooses `walk` pace and emits the same `ActorGroundedMoveIntent` shape rather than choosing a numeric speed or moving the NPC itself. The application observes both the controlled actor and `EntityId{2}`, collects human and rest-need NPC intents into one World batch, and Godot materializes the NPC only as a presentation replica of authoritative samples.

Future wounds, carried load, progression or concrete magical effects may alter resolved locomotion capability only when those authoritative mechanics exist. They attach at the World resolver seam; they do not justify a player-only/NPC-only speed multiplier, generic effect bus or second movement solver.

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
- `PlanarMoveIntent` — bounded semantic X/Z direction/magnitude, never displacement or final state;
- `LocomotionPace` — semantic `walk` / `run` / `sprint` intent, never numeric speed;
- `ActorLocomotionCapability` — authoritative per-actor walk/run/sprint ceilings plus acceleration/braking baseline;
- `NpcLocalWaypoint` — already-selected local planar waypoint plus caller-owned arrival tolerance and semantic pace used only to produce NPC movement intent;
- `RestNeedState` — first authoritative NPC need state: assigned local rest point plus arrival tolerance, with satisfaction derived from exact spatial state;
- `GroundedLocomotionContinuation` — hidden per-actor position/planar-velocity/vertical remainders plus tick-rate provenance that affect the next authoritative movement result and therefore belong to snapshot truth;
- `GroundedLocomotionTickResult` — one post-transition temporal batch with actor samples canonically ordered by `EntityId`;
- bootstrap grid/cardinal types — explicitly temporary transport probes.

A command changing an actor does not automatically mean time advanced. A world revision changing does not automatically mean spatial position changed. A spatial epoch changing means presentation must treat the relocation as discontinuous. Controller intent submission is specifically non-mutating; the fixed locomotion tick is the world-time mutation boundary. NPC local steering and RestNeed evaluation are also non-mutating decision output: the shared World tick remains the authoritative movement boundary.

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

The controlled actor and the first identity-resolved living-need NPC in the active 3D scenario have exact spatial state because current locomotion causality needs it. A distant or aggregate-resolved actor may remain authoritative without exact pose when current mechanics do not require one.

`SpatialEpoch` changes on teleport/respawn/discontinuous transfer. Samples across different epochs are not interpolated as ordinary motion.

There is deliberately no general `SetPosition`/`SetVelocity` world API. Implemented continuous movement writes are semantic actor intents whose resulting position/velocity are decided by Simulation after actor-capability resolution and shared collision/movement rules. The current neutral `GroundPatch`/`VerticalBarrier` vectors and flat protocol acceptance context prove the transition; they are not yet a production large-world collision/index format or a declaration that Godot scene colliders are authoritative.

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

The shared application protocol version lives in `src/protocol/version.hpp`. Breaking client-facing boundary changes update that version and affected native/client evidence together. Protocol **v5** began when semantic movement intent plus `AuthoritativeMovementSampleBatch` became Godot-facing. Protocol **v6** adds semantic controlled locomotion pace; it still exposes no numeric movement limit or client-authored velocity.

Movement uses `ControlledActorMoveIntent{x,z,pace}` at the application boundary. `submit_controlled_actor_move_intent()` validates/replaces controller state but does not mutate `World`; `advance_locomotion_tick()` applies the stored intent through the shared actor-generic World batch and returns `AuthoritativeMovementSampleBatch`. `SetNpcPosition`, `SetPlayerTransform`, client-authored velocity/speed and final-displacement setters remain invalid world APIs.

`AuthoritativeMovementSampleBatch` has one post-transition tick/revision envelope and samples sorted by ascending `EntityId`. Each sample carries identity, position, velocity and `SpatialEpoch`. Across movement batches, locomotion tick supplies temporal order; world revision locates the batch relative to other authoritative mutations. No extra sequence counter is introduced without demonstrated need.

The GDExtension exposes the protocol's semantic submission and batch result directly. It maps pace enums and units; it does not manufacture displacement, speed limits, grounded state, collision outcomes or a separate frame-stream sequence.

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

`WorldPresentation` validates monotonic world revisions, tracks observed IDs and owns the `EntityId -> EntityBinding -> presentation root` map. It binds the pre-authored controlled `Player`, materializes observed non-controlled actors through the generic NPC presentation shell, performs controlled initial placement from `ControlledActorSpatialProjection`, and then applies continuous movement for all bound observed actors through ordered `AuthoritativeMovementSampleBatch` results.

The initial-placement API remains separate from continuous movement. A newly materialized NPC shell starts hidden and has no assumed authoritative transform; its first valid movement sample supplies position/velocity/`SpatialEpoch` and makes it visible. `WorldPresentation` rejects non-consecutive locomotion ticks, stale/duplicate revisions, protocol mismatches, invalid/unordered samples, missing bindings and samples outside the observed set before moving any presentation root. Same-epoch movement uses enabled Godot physics interpolation; a changed `SpatialEpoch` applies the authoritative relocation and resets interpolation.

Removing or hiding a Godot representation must never delete the simulated entity. Additional presentation kinds should be added only when real entity capabilities require them rather than through a universal scene factory.

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
4. translate results/projections/events/samples back to Godot-facing values;
5. expose diagnostics without embedding world rules.

The current `SimFacade` exposes:

```text
observed_world_projection()                         # identity/presence read
controlled_actor_spatial_projection()              # initial/debug exact-spatial read
controlled_actor_submit_move_intent(x, z, pace)    # semantic direction + pace
advance_locomotion_tick()                           # authoritative ordered movement batch
bootstrap_submit_move(dx, dy)                       # Milestone 0 probe only
bootstrap_debug_projection()                        # Milestone 0 diagnostics only
```

Spatial unit conversion belongs here: integer millimeters/mm-per-second in protocol become meter/meter-per-second Godot `Vector3` values. Pace enum translation belongs here. Collision, capability or movement-limit decisions do not.

If a systemic gameplay rule is implemented inside a `GDCLASS`, GDScript node, UI script or serialization helper, the boundary is violated.

## Godot client architecture

Godot is the interactive presentation client, not the authoritative simulator.

Current graph:

```text
InputMap
  -> PlayerControls + ControlProfile
       -> camera-relative semantic move direction
       -> run / sprint semantic pace selection
       -> ThirdPersonCameraRig
            -> SpringArm3D -> Camera3D

SimFacade.observed_world_projection()
  -> WorldPresentation
       -> EntityBinding -> Player presentation
       -> EntityBinding -> materialized NPC presentation

SimFacade.controlled_actor_spatial_projection()
  -> WorldPresentation.initialize_controlled_spatial_presentation()
       -> initial Player meter-space position
       -> reset_physics_interpolation()

SimFacade.controlled_actor_submit_move_intent(x,z,pace)
  -> SimFacade.advance_locomotion_tick()
       -> AuthoritativeMovementSampleBatch
       -> WorldPresentation.apply_authoritative_movement_sample_batch()
            -> authoritative CharacterBody3D physics-root position/velocity
            -> same-epoch Godot physics interpolation
            -> epoch-change reset
```

`WorldPresentation` is the Godot owner of authoritative presentation identity/presence and continuous sample reconciliation. Feature scripts must not create parallel `EntityId -> Node` registries or assign authoritative IDs themselves.

`ThirdPersonPlayer` does not call `move_and_slide()` and does not apply Godot gravity, acceleration/deceleration or sprint to its physics-root position. `LocomotionProfile` now retains presentation-only turn response. The player's visual child may turn toward authoritative velocity; facing remains presentation-only until an authoritative orientation contract exists.

Because the current Simulation and local Godot client both use the project-owned 60 Hz fixed baseline, each Godot physics tick advances one authoritative locomotion tick and applies its result. Godot's enabled physics interpolation smooths same-epoch rendering. If future transport timing becomes decoupled from local physics ticks, custom jitter buffering/interpolation must be re-admitted against that real timing requirement rather than assumed now.

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
- `ActorLocomotionCapability` is per-actor snapshot truth; `LocomotionPace` is semantic intent; one World resolver produces numeric solver limits;
- actor-generic `World::advance_grounded_locomotion_tick()` applies multiple actor intents atomically and advances time/revision once per batch;
- native tests prove grounded acceleration/braking/reversal, multiple paces, different actor capabilities, and malformed capability/pace rejection;
- Core `decide_npc_local_move_toward_waypoint()` is read-only/deterministic and produces actor-keyed direction/pace intent from authoritative state rather than mutating position;
- native parity tests batch NPC-produced and equivalent human intents through the same World transition and require equivalent movement/velocity outcomes;
- `GroundedLocomotionTickResult` returns post-transition samples canonically sorted by `EntityId`, independent of intent collection order;
- fixed-step locomotion continuation, `ActorLocomotionCapability` and `RestNeedState` that affect subsequent decisions are captured by current `WorldSnapshot` schema **v4** and deterministic restore tests;
- Core living-need tests prove rest-task intent is derived from authoritative state, selects walk pace, becomes satisfied inside assigned tolerance, and survives snapshot/restore;
- protocol movement tests prove semantic direction/pace submission does not mutate world state, invalid pace cannot replace accepted intent, fixed locomotion ticks produce `AuthoritativeMovementSampleBatch`, and repeated batches increase tick/revision monotonically;
- protocol **v6** is the shared Godot-facing contract after semantic locomotion pace is added;
- `ObservedWorldProjection` and `ControlledActorSpatialProjection` remain separate purpose-built reads rather than the continuous movement stream;
- native tests prove exact-spatial and non-exact actors can coexist and invalid spatial epoch cannot mutate the world;
- protocol tests prove bootstrap grid movement changes revision but not production spatial position;
- Godot has one `WorldPresentation` identity/presence/sample owner; it materializes observed non-controlled actors hidden until their first authoritative sample, rejects duplicate/non-consecutive batches, and applies authoritative sample position/velocity instead of local collision results;
- `ThirdPersonPlayer` has no `move_and_slide()`/Godot-gravity position path and Godot locomotion profile has no speed/acceleration law;
- bootstrap movement names discourage treating the grid probe as production locomotion;
- only `world_sim_gdextension` links godot-cpp;
- `tools/check_architecture.py`/CTest rejects direct Godot include markers in `src/sim` and `src/protocol`;
- smoke evidence proves bootstrap separation, observed identity, authoritative spatial state, semantic movement direction/pace round-trip, duplicate-batch rejection, presentation reconciliation and localization independently.

As the graph grows, prefer real target/API boundaries over prose-only rules. Add a narrow mechanical check only when a real dependency edge cannot already be expressed by code/build ownership.
