# World Simulation

Playable systemic third-person RPG with one authoritative C++23 Simulation Core and one Godot 4 presentation client.

## Start here

- Product: [`docs/PRODUCT.md`](docs/PRODUCT.md)
- Runtime architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- Simulation/modeling: [`docs/MODELING.md`](docs/MODELING.md)
- Authoritative spatial model: [`docs/models/spatial-location.md`](docs/models/spatial-location.md)
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

The native `BootstrapMoveIntent` grid path remains Milestone 0 transport evidence and does not change production spatial state. Continuous third-person locomotion still needs the next Godot-free deterministic movement/collision stage; do not extend the grid probe or copy Godot motor results back into Simulation.

See [`docs/decisions/0004-authoritative-world-presentation-boundary.md`](docs/decisions/0004-authoritative-world-presentation-boundary.md) and [`docs/decisions/0006-authoritative-spatial-contract.md`](docs/decisions/0006-authoritative-spatial-contract.md).

## Controls

The reference client supports keyboard/mouse and gamepad:

- WASD / left stick — movement intent;
- mouse / right stick — orbit camera;
- Shift / L3 — sprint intent;
- Escape — release captured mouse; click to recapture.

Input/device translation lives in `PlayerControls`, camera behavior in `ThirdPersonCameraRig`, the current local presentation motor in `ThirdPersonPlayer`, presentation identity/spatial initialization in `WorldPresentation` / `EntityBinding`, and tuneable feel values in `ControlProfile` / `LocomotionProfile` resources.

The current playable motor remains useful for feel while authoritative continuous movement is migrated into Simulation. Its Godot transform is not world truth.

## Local development

Requirements: Python 3.12+ and the exact Godot baseline recorded in [`tools/toolchain.lock.json`](tools/toolchain.lock.json). `GODOT_BIN` may point to the editor binary when it is not on `PATH`.

```bash
python3 tools/bootstrap.py
.venv/bin/python tools/dev.py check --preset native
.venv/bin/python tools/dev.py build --preset dev
.venv/bin/python tools/dev.py test --preset dev
.venv/bin/python tools/dev.py play --scenario smoke
```

On Windows use `.venv\\Scripts\\python.exe` instead.

`native` omits GDExtension but still builds/tests the Godot-free graph. `dev` builds the debug GDExtension against the immutable godot-cpp revision in [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake). The bounded smoke playtest validates `debug.json` plus `final.png` under `.cache/play/`.

The client split is **GDScript + C++ GDExtension**, not C#. Version semantics live in [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md).
