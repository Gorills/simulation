# World Simulation

Playable systemic third-person RPG with one authoritative C++23 Simulation Core and one Godot 4 presentation client.

## Start here

- Product: [`docs/PRODUCT.md`](docs/PRODUCT.md)
- Runtime architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- Simulation/modeling: [`docs/MODELING.md`](docs/MODELING.md)
- Authoritative spatial model: [`docs/models/spatial-location.md`](docs/models/spatial-location.md)
- Grounded locomotion model: [`docs/models/grounded-locomotion.md`](docs/models/grounded-locomotion.md)
- Simulation ↔ Godot boundary: [`docs/engineering/simulation-godot-boundary.md`](docs/engineering/simulation-godot-boundary.md)
- Verification/playtest: [`docs/VERIFICATION.md`](docs/VERIFICATION.md)
- Roadmap: [`docs/ROADMAP.md`](docs/ROADMAP.md)
- Documentation router: [`docs/INDEX.md`](docs/INDEX.md)
- Stack guides: [`docs/engineering/STACK.md`](docs/engineering/STACK.md)
- Godot/GDExtension versions: [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md)
- Agent bootstrap: [`AGENTS.md`](AGENTS.md)

## Architecture

```text
Godot 4 presentation/input/UI
  -> world_sim_gdextension
  -> sim_protocol
  -> sim_core
```

`src/sim` is the only authoritative world. Entity identity/existence and exact spatial state when causally required belong to Simulation, as will inventory, ownership, economy, relationships, politics, combat consequences and magic effects.

Godot samples human input and renders a bounded presentation. It may interpolate or predict visual movement, but a `CharacterBody3D` transform cannot decide authoritative location or another systemic result.

The player-controlled person is an ordinary simulated actor. Human input and NPC decisions are different intent sources feeding the same world rules.

`ObservedWorldProjection` carries authoritative identity/presence. `ControlledActorSpatialProjection` separately carries authoritative position/velocity, `SpatialEpoch`, `SimulationTick`, `WorldRevision` and protocol version. Simulation uses signed 64-bit millimeters; GDExtension converts those values to Godot meter-space `Vector3` values.

Exact `SpatialState` is selective. An entity may exist authoritatively without an exact 3D pose when current gameplay causality only needs semantic or aggregate location.

The first Godot-free grounded locomotion transition now proves deterministic flat-ground integration plus head-on/oblique wall blocking against neutral Simulation-owned geometry. It is not wired into the live controlled-actor protocol path yet; slope, step, fall and presentation sample reconciliation remain later slices. The native `BootstrapMoveIntent` grid path remains Milestone 0 transport evidence only.

See [`docs/decisions/0004-authoritative-world-presentation-boundary.md`](docs/decisions/0004-authoritative-world-presentation-boundary.md), [`docs/decisions/0006-authoritative-spatial-contract.md`](docs/decisions/0006-authoritative-spatial-contract.md), and [`docs/models/grounded-locomotion.md`](docs/models/grounded-locomotion.md).

## Controls

The reference client supports keyboard/mouse and gamepad:

- WASD / left stick — movement intent;
- mouse / right stick — orbit camera;
- Shift / L3 — sprint intent;
- Escape — release captured mouse; click to recapture.

Input/device translation lives in `PlayerControls`, camera behavior in `ThirdPersonCameraRig`, the current local presentation motor in `ThirdPersonPlayer`, presentation identity/spatial initialization in `WorldPresentation` / `EntityBinding`, and tuneable feel values in `ControlProfile` / `LocomotionProfile` resources.

The current playable motor remains useful for feel while authoritative continuous movement is migrated into Simulation. Its Godot transform is not world truth.

## Play locally

Requirements: Python 3.12+, a C++23 compiler, GNU Make, and the exact Godot baseline recorded in [`tools/toolchain.lock.json`](tools/toolchain.lock.json). If Godot is not on `PATH`, set `GODOT_BIN` to its executable.

From the repository root:

```bash
make play
```

On the first run this creates the repository-local `.venv`, installs the pinned CMake/Ninja tooling without populating the pip download cache, configures/fetches the pinned native dependencies, and builds the `dev` Simulation/protocol/tests/GDExtension graph. Later runs reuse that bootstrap and perform an incremental build. Before each game launch the tooling performs a headless incremental Godot import so project-local `.godot` metadata and global `class_name` registrations are valid even on a clean checkout; no editor window is opened. Then Godot launches the configured project main scene directly.

Repository-managed generated state stays under ignored project paths such as `.venv/`, `build/`, `.cache/`, `godot/.godot/` and generated `godot/bin/` libraries. The Godot executable itself is an external prerequisite; this command verifies the pinned version rather than compiling the engine.

Useful companion commands:

```bash
make check   # configure/build/test the dev graph locally
make smoke   # build, then run the bounded artifact-producing Godot smoke playtest
```

The underlying Python front door remains available for focused work:

```bash
python3 tools/bootstrap.py
python3 tools/dev.py check --preset native
python3 tools/dev.py build --preset dev
python3 tools/dev.py test --preset dev
python3 tools/dev.py run
```

`native` omits GDExtension but still builds/tests the Godot-free graph. `dev` builds the debug GDExtension against the immutable godot-cpp revision in [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake). The bounded smoke playtest validates `debug.json` plus `final.png` under `.cache/play/`.

The client split is **GDScript + C++ GDExtension**, not C#. Version semantics live in [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md).
