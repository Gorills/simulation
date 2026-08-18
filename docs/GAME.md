# Current Game

**Status:** ACTIVE

This document describes only what is actually playable now.

## Current milestone

Milestone 0 — Toolchain & Playable Spine.

## How to play

Build:

```bash
python tools/dev.py configure
python tools/dev.py build
```

Run the reference client:

```bash
./build/native-debug/sim_cli
```

Controls in the current line-oriented terminal client:

- `W` + Enter — north;
- `A` + Enter — west;
- `S` + Enter — south;
- `D` + Enter — east;
- `Q` + Enter — quit.

## What is playable

- a seeded authoritative C++ world state;
- a player position on a small terminal view;
- movement through `MoveIntent`;
- authoritative tick advancement after accepted movement;
- rendered terminal feedback after each move.

The `+` marker is the origin after the player leaves it. `@` is the current player projection.

## Current scenario

`smoke` uses seed `20260818` and proves that an east movement changes authoritative position from `(0,0)` to `(1,0)` and advances tick from `0` to `1`.

Run it with:

```bash
python tools/play.py --scenario smoke
```

## Known player-facing gaps

There are no NPCs, interaction, inventory, economy, dialogue, persistence, institutions or magic yet. Those are later vertical capabilities and must not be implemented ahead of their milestone task.
