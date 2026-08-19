# World Simulation

Playable systemic RPG with one authoritative C++23 Simulation Core and one Godot 4 client.

## Start here

- Product: [`docs/PRODUCT.md`](docs/PRODUCT.md)
- Runtime architecture: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- Simulation/modeling: [`docs/MODELING.md`](docs/MODELING.md)
- Verification/playtest: [`docs/VERIFICATION.md`](docs/VERIFICATION.md)
- Roadmap: [`docs/ROADMAP.md`](docs/ROADMAP.md)
- Documentation router: [`docs/INDEX.md`](docs/INDEX.md)
- Stack guides: [`docs/engineering/STACK.md`](docs/engineering/STACK.md)
- Godot/GDExtension versions: [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md)
- Agent bootstrap: [`AGENTS.md`](AGENTS.md)

## Architecture

```text
Godot 4
  -> world_sim_gdextension
  -> protocol
  -> sim_core
```

`src/sim` owns world truth. Godot owns presentation/input/UI. The first implemented path is a semantic move command whose returned native projection drives the client position.

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

`native` omits GDExtension but still builds/tests the Godot-free core. `dev` builds the debug GDExtension against the exact immutable godot-cpp revision in [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake). The smoke playtest is bounded, owns only its Godot process, and validates `debug.json` plus `final.png` under `.cache/play/`.

The selected client split is **GDScript + C++ GDExtension**, not C#. Version semantics and verification status live in [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md).
