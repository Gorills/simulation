# Current Architecture

**Status:** ACTIVE

This document describes implemented architecture only.

## Dependency direction

```text
sim_cli presentation
      -> typed protocol
      -> sim_core
```

`sim_core` does not depend on the client or developer tooling.

## Authoritative ownership

`src/sim/` owns authoritative world state and transitions.

Current authoritative state contains:

- explicit seed;
- simulation tick;
- player grid position.

The client receives `PlayerProjection` and cannot set authoritative position directly.

## Protocol

`src/protocol/protocol.hpp` defines protocol version `1` and the current typed contract:

- `MoveIntent`;
- `MoveDirection`;
- `CommandResult`;
- `PlayerProjection`.

Movement follows:

```text
terminal key
-> MoveIntent
-> Simulation::execute
-> CommandResult / PlayerProjection
-> terminal render + debug projection
```

## Reference client

`src/client-terminal/main.cpp` is presentation/input only.

It maps W/A/S/D to `MoveIntent`, renders the current projection as an ASCII frame, and emits a `DEBUG_JSON` line for bounded verification.

The terminal client is the required reference client because it runs completely in the agent environment with no downloadable runtime.

## Toolchain boundary

The required path is native only:

```text
C++23 -> CMake/Ninja -> native executables -> CTest / terminal playtest
```

Emscripten, WASM, browser clients, npm dependencies and Playwright are not part of the required architecture. A future graphical/browser adapter may be added as an optional presentation layer only after local execution viability is proven.

## Playtest lifecycle

`python tools/play.py --scenario smoke`:

1. obtains a non-blocking repository-local playtest lock;
2. starts exactly one `sim_cli` process in its own process group;
3. sends bounded player input through stdin;
4. captures stdout/stderr;
5. validates before/after authoritative projections;
6. writes `final.txt` and `debug.json` evidence;
7. tears down the owned process within a hard timeout.

No network, browser or external service participates in this path.
