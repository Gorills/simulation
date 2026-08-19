# ADR 0001: C++ Simulation Core + Godot GDExtension

Status: Accepted  
Date: 2026-08-19

Related contracts: [`../PRODUCT.md`](../PRODUCT.md) · [`../ARCHITECTURE.md`](../ARCHITECTURE.md) · [`../engineering/VERSIONS.md`](../engineering/VERSIONS.md)

## Context

The repository is being restarted as a greenfield playable systemic RPG. Deleted experiments used Fenster and a TypeScript/Canvas/WASM/Playwright path; those experiments are not the architecture baseline.

The desired local stack is native C++ for authoritative simulation plus Godot as the actual game client. The locally observed preparation environment included a Godot 4.7.1 Mono editor and GCC with C++23 support; build bootstrap/tool availability must still be established by executable project tooling rather than assumed from this ADR.

The architecture needs:

- one authoritative world implementation;
- fast Godot-free deterministic native tests;
- a real player-facing Godot loop;
- a narrow engine/native boundary;
- no custom Godot engine build unless future evidence requires one.

## Decision

1. Authoritative world state, laws and outcomes live in a Godot-free **C++23 Simulation Core**.
2. A small C++ **application protocol** expresses semantic intents/commands, results, events and projections without Godot types.
3. **Godot 4** is the reference client: 2D presentation initially, typed GDScript, InputMap, UI/audio/scene composition.
4. The only Godot ↔ native gameplay seam is a thin **GDExtension adapter** using godot-cpp.
5. Native tests prove Simulation Core/protocol behavior without loading Godot.
6. The project uses one CMake-based native build graph; upstream SCons examples do not create a second project build graph.
7. C#, TypeScript/Canvas, WASM/Emscripten, Playwright/Chromium and Fenster are not part of the initial project stack.

Godot engine, GDExtension API and godot-cpp revisions are separate version dimensions. Exact current policy is owned by [`../engineering/VERSIONS.md`](../engineering/VERSIONS.md); this ADR deliberately does not duplicate an exact binding revision.

## Alternatives rejected

### Process/IPC client boundary

A separate simulation process would create stronger runtime isolation, but it adds process/protocol lifecycle and failure modes before the local single-player playable spine needs them. A stable application protocol keeps that option open without paying its complexity now.

### Godot engine module / custom engine build

A module would integrate native code more deeply but couples development/distribution to a custom Godot build. GDExtension provides the required native seam without rebuilding the engine.

### World rules in GDScript or C#

This creates a second world authority and makes native deterministic tests incomplete. Godot scripts remain presentation/input/client orchestration.

### Restore WASM + TypeScript client

This contradicts the selected local Godot client and would create another presentation/runtime stack before the playable native-to-Godot spine exists.

## Consequences

Positive:

- native simulation tests stay fast and engine-independent;
- Godot remains a real game client rather than the domain model;
- the engine seam is explicit and replaceable;
- product work can proceed as vertical capabilities through one protocol.

Costs:

- protocol/projection design must be explicit;
- the GDExtension adapter needs build/load compatibility verification;
- UI may require projection DTOs rather than directly browsing internal world objects.

## Follow-up enforcement

Once runtime targets exist, architecture checks/build dependencies should mechanically prove:

- `src/sim`/`src/protocol` have no Godot dependency;
- only the GDExtension adapter links godot-cpp;
- the selected binding revision builds and loads in the pinned Godot environment;
- player-facing capabilities reach Godot through the application protocol rather than client-side truth.
