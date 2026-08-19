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
  -> GDExtension adapter
  -> application protocol
  -> C++23 Simulation Core
```

`src/sim` owns world truth. Godot owns presentation/input/UI. See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Local development

Runtime/build bootstrap has not landed yet. Do not infer available tools or successful builds from documentation.

The selected client split is **GDScript + C++ GDExtension**, not C#. Exact Godot/GDExtension policy is in [`docs/engineering/VERSIONS.md`](docs/engineering/VERSIONS.md).

The next implementation target is [`docs/ROADMAP.md`](docs/ROADMAP.md#milestone-0--toolchain--playable-spine).
