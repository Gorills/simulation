# Runtime architecture

This document owns **runtime boundaries, dependency direction, ownership and integration seams**. It does not own product goals, modeling policy, verification procedure or tool versions.

Related canonical owners:

- product and playable invariants: [`PRODUCT.md`](PRODUCT.md)
- modeling/determinism: [`MODELING.md`](MODELING.md)
- verification/playtest evidence: [`VERIFICATION.md`](VERIFICATION.md)
- milestones: [`ROADMAP.md`](ROADMAP.md)
- stack implementation guidance: [`engineering/STACK.md`](engineering/STACK.md)
- Godot/GDExtension versions: [`engineering/VERSIONS.md`](engineering/VERSIONS.md)
- consequential decisions: [`decisions/`](decisions/)

Current source, build configuration, executable tests and lock files are authoritative for actual implemented behavior. A diagram is not proof that a planned path or target already exists.

## Runtime dependency graph

The graph is one-way:

```text
Godot 4 client
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
godot/       !-> authoritative systemic world outcomes
```

Folder names do not enforce architecture. The CMake target graph is the primary executable expression of the dependency direction.

## Implemented native target graph

The native graph is physically split by responsibility:

```text
sim_core_tests ---------> sim_core
                              ^
                              |
protocol_tests --------> sim_protocol
                              ^
                              |
world_sim_gdextension -------+----> godot::cpp
```

`sim_protocol` has a private link dependency on `sim_core`; `sim_core` has no dependency on protocol or Godot.

- `sim_core` owns domain types and authoritative world transitions.
- `sim_protocol` owns client-facing intents/results/projections and translates validated protocol requests into domain operations.
- `sim_core_tests` test the domain directly with no protocol/Godot dependency.
- `protocol_tests` test validation plus command-to-domain-to-projection behavior.
- `world_sim_gdextension` links `sim_protocol` + `godot::cpp`; it does not own the authoritative `sim::World` directly.
- `architecture_no_godot_in_core` runs `tools/check_architecture.py` through CTest and rejects Godot/godot-cpp include markers under `src/sim` and `src/protocol`.

The first executable native smoke path is:

```text
MoveIntent(dx, dy)
  -> protocol::Simulation validation
  -> sim::CardinalDirection
  -> sim::World::move
  -> PlayerProjection
```

Malformed transport values such as diagonal or zero deltas are rejected in the protocol layer before the domain is called. The domain API accepts a valid `CardinalDirection`, so an invalid movement delta is not representable inside `sim::World::move`.

This path is deliberately retained as a Milestone 0 protocol/GDExtension round-trip probe. It is **not** the locomotion contract for the third-person client.

## Ownership by layer

| Layer | Owns | Must not own |
| --- | --- | --- |
| `src/sim` | world state, domain value types, laws, deterministic outcomes, seeded RNG state | protocol DTOs, Godot types, input, frames, UI, wall-clock gameplay truth |
| `src/protocol` | commands/intents, validation/translation, results, events, projections, versioned boundary DTOs | rendering, scene state, duplicated domain rules |
| `src/adapters/gdextension` | translation between Godot-facing values and protocol API; GDExtension registration | direct world ownership, world laws or alternate gameplay truth |
| `godot/` | scenes, input, local `CharacterBody3D` kinematics/collision, camera, audio, animation, UI, presentation state | authoritative inventory, economy, relationships, ownership, access rights, spell/trade/damage outcomes |
| native tools/tests | scenarios, diagnostics, verification, developer orchestration | a second simulator or alternate gameplay implementation |

The authoritative systemic world exists once: in the C++ Simulation Core. Godot may own engine-local kinematic/presentation state when that state cannot create systemic outcomes by itself. When local locomotion needs to affect semantic location, reachability or another world law, add an explicit protocol capability rather than promoting arbitrary scene transforms into world truth.

See [`decisions/0002-third-person-controls.md`](decisions/0002-third-person-controls.md) for the third-person locomotion boundary.

## Domain API quality bar

Simulation code should expose semantic domain operations and types rather than transport-shaped primitives.

The initial native smoke examples are intentionally small:

- `SimulationTick` is distinct from an arbitrary integer counter;
- `WorldSeed` is distinct from simulation time;
- `GridPosition` is an explicit value used by the current bootstrap probe;
- `CardinalDirection` represents a valid bootstrap movement choice;
- `World::move(CardinalDirection)` cannot receive a malformed `dx/dy` pair.

Do not infer from those bootstrap names that the final third-person world uses a tile/grid locomotion model. The production client uses a local 3D locomotion shell; future semantic-location modeling must be introduced from an actual gameplay requirement.

Do not introduce a strong type merely to wrap every scalar. Add one when it prevents mixing different domain meanings, removes invalid states, or makes an authoritative contract materially clearer.

## Protocol boundary

Clients express **intent**, never desired systemic state:

```text
Input Intent
  -> validated Command
  -> authoritative transition
  -> CommandResult + DomainEvents + Projections
```

`protocol::Simulation` is the current thin application surface. It may validate/translate boundary data and orchestrate calls into `sim`; it must not become a second home for world rules.

The protocol is a small application contract, not an exported `WorldState`. Internal simulation types do not automatically become public/client types.

Breaking protocol changes update the explicit protocol version and affected native/client verification together.

See [`MODELING.md`](MODELING.md#protocol-semantics) for semantic rules.

## GDExtension seam

Godot crosses into native gameplay through exactly one runtime seam: the GDExtension adapter.

The adapter should be deliberately boring:

1. receive a semantic client request;
2. translate Godot-facing values into protocol values;
3. invoke the application/protocol surface;
4. translate results/projections/events back to Godot-facing values;
5. expose diagnostics without embedding world rules.

The Milestone 0 `SimFacade` follows that contract: it owns `protocol::Simulation`, exposes `submit_move` and read-only `debug_projection`, and converts only protocol projections into Godot dictionaries.

If a systemic gameplay rule is implemented inside a `GDCLASS`, GDScript node, UI script or serialization helper, the boundary is probably being violated.

The adapter may depend on godot-cpp. `src/sim` and `src/protocol` may not. Version rules live only in [`engineering/VERSIONS.md`](engineering/VERSIONS.md).

## Godot client architecture

Godot is the real reference client, not merely a debug visualizer. It owns the latency-sensitive local third-person control shell while remaining a client of systemic world authority.

The implemented control graph is:

```text
InputMap
  -> PlayerControls + ControlProfile
       -> ThirdPersonPlayer + LocomotionProfile
       -> ThirdPersonCameraRig
            -> SpringArm3D -> Camera3D
```

`Main` is the composition root that wires these components together. `ThirdPersonPlayer` never reads raw keys/joypad indices; `ThirdPersonCameraRig` never moves the player; `PlayerControls` never owns kinematic/world state. Tuneable feel lives in profile Resources rather than branching implementations by device.

The ordinary third-person client and the native smoke probe are intentionally separate concerns. Ordinary WASD/left-stick locomotion drives the Godot kinematic shell. The bounded smoke scenario separately calls `SimFacade.submit_move(1, 0)` and exposes the resulting native projection as debug evidence that the GDExtension/protocol path works.

Detailed ownership and extension points are in [`engineering/godot.md`](engineering/godot.md).

### UI design-system boundary

The Godot UI has one project-wide visual source of truth:

```text
godot/ui/design_system/world_theme.tres
```

`project.godot` installs it through `gui/theme/custom`. Feature scenes consume semantic `theme_type_variation` roles and compose layout with Godot `Container`s. Static colors, typography sizes, StyleBoxes, focus treatment and common spacing do not belong in inventory/journal/HUD/settings scenes as copied local overrides.

The intended dependency direction is:

```text
world_theme.tres
   -> semantic Theme variations / DesignTokens
       -> optional reusable design-system components
           -> feature UI scenes
```

Feature UI may own screen-specific information architecture and behavior. It must not become a second skinning system. If a visual change that should be global requires edits across feature screens, the design-system boundary is missing an abstraction.

The logical desktop baseline is 1920×1080 with `canvas_items` + `expand`. Responsive screen structure is container/anchor-driven rather than a list of resolution-specific coordinate branches.

The supplied dark-fantasy mockups are mood references, not canonical assets. The baseline preserves their dark cinematic surfaces, sparse gold selection and cool informational accent while deliberately reducing nested framing so hierarchy comes from surface elevation, spacing and typography.

See [`decisions/0003-project-wide-ui-design-system.md`](decisions/0003-project-wide-ui-design-system.md) and [`engineering/ui-design-system.md`](engineering/ui-design-system.md) for the visual ownership and extension contract.

Do not let a convenient Autoload, Resource, transform cache or UI model become an authoritative parallel inventory/economy/social state.

## Vertical capability rule

Normal systemic gameplay work follows the product contract:

```text
minimal world rule
  -> protocol command/result/projection
  -> GDExtension translation
  -> Godot affordance/feedback
  -> focused deterministic/regression proof
  -> bounded real playtest
```

Purely engine-local presentation/control work does not need a fake native rule merely to satisfy the diagram. It must still respect the authority boundary and be verified in the real Godot client.

A coherent capability may touch several layers. It must not broaden into unrelated subsystem work.

## External AI Layer boundary

`Gorills/ai-layer` is the development control plane, not a runtime/build dependency of the game.

AI Layer may own durable Work/Task/Epic state, Project Map, Knowledge, project Decisions storage and project skills outside this repository. This repository owns source, tests, build configuration, product/modeling/runtime contracts, committed ADRs and short host bootstrap files.

Therefore:

- do not add repository-local `.ai-layer/` state;
- do not commit AI Layer databases, registry data or copied Work/Task/Epic state;
- do not reimplement AI Layer continuation or managed workflow in repository docs;
- do not link/import `ai-layer` into game/runtime/build targets;
- project skills are materialized by AI Layer into host-native catalogs outside the repository rather than copied here;
- AI Layer Knowledge/Decision records provide durable context but do not silently override current source or committed ADRs.

See [`AGENT_CONTEXT.md`](AGENT_CONTEXT.md) for how agent instructions are packaged without conflicting with this boundary.

## Repository shape

The current paths below exist; `content/`, mechanic models/research and additional adapters remain future-on-demand areas:

```text
src/
  sim/
  protocol/
  adapters/
    gdextension/

godot/
  config/
  scenes/
  scripts/
    controls/
    player/
  ui/
    design_system/

tests/
tools/
cmake/

docs/
  INDEX.md
  PRODUCT.md
  ARCHITECTURE.md
  MODELING.md
  VERIFICATION.md
  ROADMAP.md
  AGENT_CONTEXT.md
  engineering/
  decisions/
  models/       # when serious mechanic contracts exist
  research/     # when load-bearing research artifacts exist
```

Do not create future directories solely to satisfy this picture. Establish physical boundaries when real code or evidence needs them.

## Mechanical architecture verification

Current executable structure establishes the first load-bearing boundaries:

- separate `sim_core` and `sim_protocol` CMake targets encode protocol -> simulation dependency direction;
- native domain tests link only `sim_core`;
- protocol tests link `sim_protocol`, whose implementation depends on `sim_core`;
- only `world_sim_gdextension` links godot-cpp;
- `tools/check_architecture.py`/CTest rejects direct Godot include markers in `src/sim` and `src/protocol`;
- the smoke playtest is designed to prove Godot load plus a real protocol/projection round-trip once run in the pinned local environment.

As the graph grows, prefer real target/API boundaries over prose-only rules. Add a narrow mechanical check only when a real dependency edge cannot already be expressed by the build graph.
