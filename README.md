# World Simulation

Playable systemic third-person RPG with one authoritative C++23 Simulation Core and one Godot 4 presentation client.

## Start here

- Product: [`docs/PRODUCT.md`](docs/PRODUCT.md)
- Runtime architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- Simulation/modeling: [`docs/MODELING.md`](docs/MODELING.md)
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

`src/sim` is the only authoritative world. Stable entity existence/identity and, as mechanics are implemented, location, inventory, ownership, economy, relationships, politics, combat consequences and magic effects are Simulation state.

Godot samples human input and renders a bounded presentation of that world. It may interpolate or predict visual movement, but a `CharacterBody3D` transform is not authoritative location and cannot decide trade range, inventory transfer, damage, relationships or another systemic result.

The player-controlled person is an ordinary simulated actor. Human input and NPC decisions are different intent sources feeding the same world rules; there is no privileged player domain species.

The native cardinal `BootstrapMoveIntent` path remains only a Milestone 0 protocol/GDExtension transport probe. Production third-person locomotion must migrate to an authoritative actor-location/movement contract and revisioned presentation samples rather than extending the grid probe.

See [`docs/decisions/0004-authoritative-world-presentation-boundary.md`](docs/decisions/0004-authoritative-world-presentation-boundary.md).

## Controls

The reference client is keyboard/mouse and gamepad from the start:

- WASD / left stick — movement intent;
- mouse / right stick — orbit camera;
- Shift / L3 — sprint intent;
- Escape — release captured mouse; click to recapture.

Control code is split into small responsibilities. Input/device translation lives in `PlayerControls`, camera behavior in `ThirdPersonCameraRig`, the current presentation/prediction movement shell in `ThirdPersonPlayer`, and tuneable presentation feel values in `ControlProfile` / `LocomotionProfile` resources.

The current playable motor remains useful for feel while authoritative spatial movement is migrated into Simulation. Do not treat its Godot transform as world truth.

See [`docs/engineering/godot.md`](docs/engineering/godot.md), [`docs/engineering/simulation-godot-boundary.md`](docs/engineering/simulation-godot-boundary.md), and [`docs/decisions/0002-third-person-controls.md`](docs/decisions/0002-third-person-controls.md).

## Local development

Requirements: Python 3.12+ and the exact Godot baseline recorded in [`tools/toolchain.lock.json`](tools/toolchain.lock.json). `GODOT_BIN` may point to the editor binary when it is not on `PATH`.

Bootstrap the pinned CMake/Ninja environment and acquire pinned native dependencies:

```bash
python3 tools/bootstrap.py
```

Canonical front door after bootstrap:

```bash
.venv/bin/python tools/dev.py check --preset native
.venv/bin/python tools/dev.py build --preset dev
.venv/bin/python tools/dev.py test --preset dev
.venv/bin/python tools/dev.py play --scenario smoke
```

On Windows use `.venv\\Scripts\\python.exe` instead.

`native` omits GDExtension but still builds/tests the Godot-free native graph. `dev` builds the debug GDExtension against the exact immutable godot-cpp revision in [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake). The smoke playtest is bounded, owns only its Godot process, and validates `debug.json` plus `final.png` under `.cache/play/`.

The selected client split is **GDScript + C++ GDExtension**, not C#. Version semantics and verification status live in [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md).
