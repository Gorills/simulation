# Current Foundation Architecture

**Status:** ACTIVE

This document describes what is implemented **now**. The repository is a development/graphics foundation; gameplay has not started.

## Implemented dependency shape

```text
tools/dev.py
  -> CMake / Ninja / CTest
  -> foundation_tests
  -> platform_graphics_smoke
       -> vendored Fenster (diagnostic-only)
       -> OS window/input backend

Linux verification only:
tools/graphics_check.py
  -> Xvfb
  -> platform_graphics_smoke
  -> xwininfo
  -> platform_x11_key_sender
  -> platform_x11_capture
  -> ImageMagick PNG conversion
```

There is currently no `sim_core`, gameplay protocol, game client, persistence layer or gameplay playtest runner.

There is also **no selected production game window/input abstraction yet**. The current Fenster path is retained only as the already-proven native graphical diagnostic. The required production capability/selection gate is canonical in [`engineering/PLATFORM_CAPABILITIES.md`](engineering/PLATFORM_CAPABILITIES.md).

## Repository layout

```text
CMakeLists.txt                 build graph and platform link selection
CMakePresets.json              canonical debug/release presets
src/platform/graphics_smoke.cpp
                               graphical stack fixture; not gameplay
tests/foundation/              C++23/toolchain foundation tests
tools/dev.py                   canonical development front door
tools/graphics_check.py        Linux headless graphical verification supervisor
tools/platform/                Linux-only X11 test helpers
third_party/fenster/           vendored diagnostic window/framebuffer dependency
third_party/manifest.json      vendored dependency integrity metadata
docs/engineering/AGENT_RUNBOOK.md
                               exact operating procedure for agents
docs/engineering/PLATFORM_CAPABILITIES.md
                               production native window/input capability gate
```

## Native graphical diagnostic boundary

The currently implemented graphical diagnostic is a software RGB framebuffer presented through the vendored Fenster platform layer.

Fenster is intentionally thin. In the current repository it proves:

- one native window;
- framebuffer presentation;
- basic keyboard state;
- basic mouse position/button state;
- basic timing helpers.

It is **not** a game engine, ECS, scene graph, state manager, simulation framework or gameplay authority.

It is now also explicitly **not the selected production game window/input API**. The capability audit found that its current public contract is insufficient for the future production requirements around resize/focus lifecycle, wheel input, separate Unicode text input and DPI/scale. Do not build player-facing UI/gameplay directly on Fenster internals.

CMake currently selects the diagnostic OS boundary:

- Linux: X11;
- Windows: Win32 (`gdi32`, `user32`);
- macOS: Cocoa framework.

The Linux diagnostic path is verified in the current agent environment. Windows/macOS branches are present because the pinned upstream library implements them, but they are **not verified here** and must not be reported as locally proven.

## Production platform selection gate

The future production platform layer must satisfy [`engineering/PLATFORM_CAPABILITIES.md`](engineering/PLATFORM_CAPABILITIES.md) and be locally proven before gameplay or game-UI implementation begins.

Required concepts include:

```text
window lifecycle / resize / focus
framebuffer presentation + explicit viewport/scaling
control-key press/release/modifiers
separate Unicode text input
pointer move/buttons/wheel
scale/DPI query contract
bounded polling + owner-controlled teardown
```

Controller/gamepad is intentionally deferred until a concrete gameplay task requires it.

A one-off Linux X11 proof in the agent environment demonstrated resize, focus, control-key input, XIM text input and wheel events using only the already-required X11 stack. That proves the host can support the richer contract; it does not select a production library.

Candidate evaluation and acceptance criteria live only in `PLATFORM_CAPABILITIES.md` so they do not drift across documents.

## Vendored dependency model

The required build path performs no dependency download.

`third_party/fenster/` currently contains the repository-owned **diagnostic** build input plus its MIT license and upstream metadata. `third_party/manifest.json` records local content hashes. `python tools/dev.py vendor-check` validates those hashes before normal foundation verification.

Normal configure/build/test commands never contact the network.

Dependency upgrades/replacements are separate bounded stack tasks; ordinary gameplay/foundation edits must not modify vendored source casually. A production platform replacement must be vendored/pinned, licensed/provenanced and locally verified before it becomes a required build dependency.

## Foundation tests

`foundation_tests` validates that the compiler is actually building in C++23 mode with required standard-library capability.

`graphics_framebuffer_self_test` exercises framebuffer drawing without opening a window. These tests are CTest-registered and dependency-free.

They prove deterministic foundation behavior, not a running graphical OS path and not the future production platform capability contract.

## Linux graphical verification lifecycle

The current canonical diagnostic command is:

```bash
python tools/dev.py graphics-check
```

`tools/graphics_check.py` owns the whole fixture lifecycle:

1. takes a non-blocking repository-local lock;
2. selects a free display in the reserved `:90..:119` range;
3. starts exactly one Xvfb instance in its own process group;
4. starts exactly one `platform_graphics_smoke` process in its own process group;
5. locates the native window by its fixed diagnostic title using `xwininfo`;
6. sends a real X11 `D` key press/release using `platform_x11_key_sender`;
7. requires `state.txt` evidence that the application received the input;
8. captures the actual window with `XGetImage` through `platform_x11_capture` into PPM;
9. verifies window dimensions and a known rendered pixel in the captured image;
10. converts PPM to `final.png` with the locally installed ImageMagick CLI;
11. confirms the graphical process is still alive after input/capture;
12. terminates only the process groups created by the harness;
13. writes bounded run metadata and logs.

Teardown deliberately does **not** send another UI key. Input was already proven by `D`; fixture lifetime belongs to the supervisor, so cleanup is process-owner controlled and cannot depend on a stale window id.

This diagnostic scenario must continue to work until its replacement provides equivalent or stronger evidence. Do not confuse passing `graphics-check` with satisfying the production platform selection gate.

## Graphical verification artifacts

Each run writes under:

```text
.cache/graphics-check/<run-id>/
  run.json
  state.txt
  stdout.log
  stderr.log
  xvfb.stderr.log
  final.ppm
  final.png
```

A successful current fixture has:

- `state.txt`: `d_received=1`;
- captured size: `320x200`;
- rendered probe at `(228,100)`: `#35A853`;
- `run.json` status: `success`;
- visually, the right diagnostic rectangle is green after input.

These artifacts are local evidence and are ignored by Git.

## Canonical build acceptance

```text
python tools/dev.py check
  = doctor
  + debug configure/build
  + CTest
  + Python compile check
  + documentation hygiene

python tools/dev.py verify
  = check
  + real Linux headless graphical diagnostic
  + release configure/build
```

`verify` is the full **current foundation** acceptance command on the verified Linux agent host. It does not by itself certify a future production platform candidate; candidate acceptance must extend/replace the relevant graphical scenario as required by `PLATFORM_CAPABILITIES.md`.

## Planned gameplay architecture — not implemented

When the production platform selection blocker has been resolved and the user explicitly starts gameplay in a later bounded task, the intended direction remains:

```text
graphical presentation/input
        -> typed protocol
        -> authoritative C++23 Simulation Core
```

The platform/framebuffer layer must remain presentation only. The future graphical game executable must not store a second authoritative copy of world state.

No gameplay file or document should be created merely to pre-stage that future architecture.