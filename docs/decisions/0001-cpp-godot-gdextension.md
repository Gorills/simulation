# ADR 0001: C++ Simulation Core + Godot GDExtension

Status: Accepted  
Date: 2026-08-19  
Contract: [`docs/specs/TZ.md`](../specs/TZ.md) · Map: [`docs/INDEX.md`](../INDEX.md)

## Context

The greenfield TZ originally targeted C++/WASM + TypeScript/Canvas + Playwright/Chromium. Previous local experiments (Fenster, web client) were deleted. The required local stack is C++ and Godot. Godot 4.7.1.stable.mono is installed; g++ 13.3 compiles C++23 `std::expected`; cmake/ninja are not installed yet.

## Decision

1. Authoritative world laws live in a Godot-free C++23 library (`src/sim` + `src/protocol`).
2. Godot 4 is the reference client: 2D scenes, typed GDScript, InputMap, UI.
3. The only runtime seam is a thin **GDExtension** adapter (`src/adapters/gdextension`) that translates protocol commands/projections. It may depend on godot-cpp; Simulation Core must not.
4. Native headless tests (GoogleTest/CTest) do not require Godot.
5. C#, TypeScript, WASM/Emscripten, Playwright, and Fenster are out of the local contract.

## Alternatives rejected

- **Process/IPC client:** stronger isolation, weaker playable loop and extra failure modes for a local single-player spine.
- **Godot module compiled into a custom engine:** heavier distribution and slower iteration than GDExtension.
- **World rules in GDScript/C#:** creates a second authority and breaks headless determinism tests.
- **Keep WASM+TypeScript:** contradicts the chosen local stack.

## Consequences

- Playable path is native core + Godot 2D, not a browser.
- godot-cpp must be pinned to the Godot 4.7.x series.
- Mono editor may be used as the Godot binary; the project language split remains C++ / GDScript.
- Bootstrap must install cmake and ninja before the first C++ build.
