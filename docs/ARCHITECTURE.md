# Current Foundation Architecture

**Status:** ACTIVE

This document describes what is implemented **now**. The repository is a development/graphics foundation; gameplay has not started.

## Implemented dependency shape

```text
tools/dev.py
  -> CMake / Ninja / CTest
  -> foundation_tests
  -> platform_graphics_smoke
       -> vendored Fenster
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
third_party/fenster/           vendored window/framebuffer/input dependency
third_party/manifest.json      vendored dependency integrity metadata
docs/engineering/AGENT_RUNBOOK.md
                               exact operating procedure for agents
```

## Native graphical boundary

The chosen presentation foundation is a software RGB framebuffer presented through the vendored Fenster platform layer.

Fenster is intentionally thin. It provides:

- one native window;
- framebuffer presentation;
- keyboard state;
- mouse position/button state;
- basic timing helpers.

It is **not** a game engine, ECS, scene graph, state manager, simulation framework or gameplay authority.

CMake selects the OS boundary:

- Linux: X11;
- Windows: Win32 (`gdi32`, `user32`);
- macOS: Cocoa framework.

The Linux path is verified in the current agent environment. Windows/macOS branches are present because the pinned upstream library implements them, but they are **not verified here** and must not be reported as locally proven.

## Vendored dependency model

The required build path performs no dependency download.

`third_party/fenster/` contains the repository-owned build input plus its MIT license and upstream metadata. `third_party/manifest.json` records local content hashes. `python tools/dev.py vendor-check` validates those hashes before normal foundation verification.

Normal configure/build/test commands never contact the network.

Dependency upgrades are separate bounded stack tasks; ordinary gameplay/foundation edits must not modify vendored source casually.

## Foundation tests

`foundation_tests` validates that the compiler is actually building in C++23 mode with required standard-library capability.

`graphics_framebuffer_self_test` exercises framebuffer drawing without opening a window. These tests are CTest-registered and dependency-free.

They prove deterministic foundation behavior, not a running graphical OS path.

## Linux graphical verification lifecycle

The canonical command is:

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
  + real Linux headless graphical verification
  + release configure/build
```

`verify` is the full foundation acceptance command on the verified Linux agent host.

## Planned gameplay architecture — not implemented

When the user explicitly starts gameplay in a later bounded task, the intended direction remains:

```text
graphical presentation/input
        -> typed protocol
        -> authoritative C++23 Simulation Core
```

The platform/framebuffer layer must remain presentation only. The future graphical game executable must not store a second authoritative copy of world state.

No gameplay file or document should be created merely to pre-stage that future architecture.
