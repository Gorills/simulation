# Native Platform Capability Contract

**Status:** ACTIVE

This document is the canonical capability gate for the future production native window/input boundary. It exists because the current graphical diagnostic proves that native framebuffer presentation and basic keyboard input work, but the currently vendored Fenster API is intentionally too small to be treated as the final game window/input contract.

This is still foundation work. It does **not** create gameplay or a game UI.

## 1. Stage gate

Until a production platform layer satisfies the required capability set below and is locally proven by the canonical agent verification path:

- `third_party/fenster` remains a **diagnostic-only** dependency;
- `platform_graphics_smoke` remains a **diagnostic-only** executable;
- do not build gameplay or player-facing UI on direct Fenster state;
- do not let future gameplay/UI code depend on X11/Win32/Cocoa handles directly;
- do not claim the production native platform boundary is selected.

The next bounded platform task must replace or wrap the diagnostic layer with a project-owned API whose required Linux path is buildable and runnable in the agent environment without downloading dependencies.

## 2. Required production capabilities

The future production platform boundary must expose these concepts without leaking OS-specific handles into gameplay/UI code.

### Window lifecycle

Required:

- create/destroy one game window;
- close request as an observable event/state;
- focus gained/lost;
- current drawable/window size;
- resize notification;
- deterministic, owner-controlled teardown;
- fixed bounded event polling/update behavior with no hidden unbounded wait.

Windowed/fullscreen transition is desirable before public distribution, but it is not required for the first gameplay slice if ordinary windowed play is reliable.

### Framebuffer presentation

Required:

- present a project-owned 32-bit framebuffer;
- explicit pixel-format contract;
- stable logical-buffer dimensions independent of OS window size;
- explicit viewport/scaling policy rather than implicit gameplay coordinates tied to physical window pixels;
- no GPU API required by the canonical agent path unless a later bounded stack task proves a concrete need.

Alpha composition, clipping, images and text belong to the separate presentation layer above this boundary; the OS layer only needs to present the finished framebuffer.

### Keyboard: controls vs text

These are separate channels and must remain separate.

**Control input** requires:

- stable logical key identity for gameplay/navigation bindings;
- press and release;
- modifier state;
- defined repeat policy;
- no assumption that printable characters equal physical/logical control keys.

**Text input** requires:

- Unicode code points or UTF-8 text events suitable for names/search/chat/text fields when those features exist;
- layout-aware input through the OS text/input-method path where available;
- text entry must not be reconstructed from gameplay key codes.

A platform layer that only exposes an ASCII-like key array is not sufficient as the long-term production contract.

### Pointer input

Required:

- pointer position in documented window/drawable coordinates;
- primary/secondary/middle button press/release;
- scroll/wheel delta;
- cursor show/hide when supported by the desktop backend;
- coordinate conversion must remain well-defined when the window is resized or letterboxed.

Raw/locked mouse input is **not** a current requirement. Add it only when a concrete gameplay interaction needs it.

### Scale / DPI

Required contract:

- distinguish logical layout scale from framebuffer/window pixels;
- provide a safe scale value (at least `1.0`) and document backend limitations;
- do not assume runtime DPI-change notifications exist on every backend;
- UI accessibility scaling must be a project-level setting and must not be replaced by OS DPI alone.

### Controller/gamepad

**Deferred, not missing-by-accident.** The first planned playable spine requires keyboard input only. Do not introduce a controller library or abstraction before a real bounded task requires controller support.

When controller support becomes real scope, select/prove it separately and add:

- connect/disconnect;
- stable button mapping;
- axes/deadzones;
- device identity where needed;
- remapping/accessibility policy;
- agent-verifiable input evidence if the environment permits it.

## 3. Current diagnostic layer assessment: Fenster

Current vendored Fenster remains useful because it is tiny, network-free and already proves native window/framebuffer execution in the agent environment.

However, its current public state is essentially:

- fixed `width` / `height`;
- `uint32_t*` framebuffer;
- `keys[256]` described upstream as mostly ASCII;
- modifier mask;
- mouse x/y;
- one mouse button/state field;
- open/loop/close/time helpers.

For the production contract above it does **not** currently provide a sufficient explicit API for resize, focus lifecycle, wheel, separate Unicode text input, DPI/scale, or richer pointer-button semantics.

**Decision:** keep Fenster only as the currently proven diagnostic fixture until a replacement production platform layer passes this document's gate. Do not grow gameplay/UI directly around it.

## 4. Linux host capability proof

A one-off foundation probe was compiled and run in the current agent environment using only the already-required X11 development/runtime stack. It was intentionally kept outside the repository because it is decision evidence, not a new production subsystem.

Under bare Xvfb the probe verified:

```text
resize:  320x200 -> 400x240
focus:   received
key:     D received as control key
text:    D produced through XIM/Xutf8LookupString text path
wheel:   X11 wheel event received
```

The first harness attempt called `XSetInputFocus` before the bare-Xvfb window became `IsViewable` and failed with `BadMatch`. The second attempt waited for the mapped/viewable state and passed all checks.

Conclusion: the verified Linux host is capable of the required resize/focus/key/text/wheel primitives using the existing X11 stack. The production-layer gap is a library/API selection problem, not a fundamental host limitation.

This one-off proof does **not** prove Cyrillic/IME composition end-to-end; the future chosen platform layer must separately prove Unicode text behavior that its API claims to support.

## 5. Candidate review

Candidate review is not adoption. A candidate becomes required only after it is vendored/pinned, builds locally with the documented host prerequisites, runs through a real bounded graphical capability scenario, and replaces the diagnostic dependency coherently.

### MiniFB

Reviewed upstream repository:

- https://github.com/emoon/minifb
- reviewed `master`: `2ca56860a2be5f62414ebe6bfaf385716d75327f` (v0.10.1 line)

Useful upstream API/features include resizable windows, focus/resize/close callbacks, key events, separate Unicode character input, mouse buttons/move/scroll, viewport/best-fit helpers, window/drawable size queries, cursor visibility and monitor scale.

The reviewed X11 implementation uses X11/XKB/XIM facilities for control and text events, including `Xutf8LookupString`, and supports software XImage framebuffer presentation when OpenGL is disabled.

**Current blocker:** upstream documents `libxkbcommon-dev` as an X11 build prerequisite and the reviewed X11 source includes `<xkbcommon/xkbcommon.h>`. That development header is not installed in the verified agent environment. The required project loop must not gain a dependency the agent cannot build locally.

The reviewed X11 source did not show an actual use of xkbcommon symbols outside that include, but we do not silently delete an upstream prerequisite and call the result pristine upstream. Adopting a patched MiniFB snapshot would require a separately recorded source patch plus a real local compile/run proof.

Result: **not adopted in this pass**.

### RGFW

Reviewed upstream repository:

- https://github.com/ColleagueRiley/RGFW
- reviewed `main`: `312e69e32f5334bb58eda37e26d00d715da6e3f4`

RGFW is a single-header native window/input library that advertises X11/WinAPI/Cocoa backends, raw software surface blitting and no non-system dependencies for its core windowing path. Its current project has also deliberately removed gamepad support from core, which is compatible with our decision to defer controller support rather than pre-build it.

Result: **candidate only**. It was source/API reviewed but was not locally compiled and driven in this pass, so it must not be described as verified or selected.

## 6. Selection acceptance test

Before replacing Fenster, the chosen production candidate must pass one bounded Linux agent scenario proving at least:

1. build with repository-vendored source and no network;
2. software framebuffer window opens under Xvfb;
3. application remains alive during the scenario;
4. resize event changes reported window/drawable dimensions;
5. focus event/state is observable;
6. real key press/release is observable through the candidate API;
7. candidate's separate text-input path produces text evidence;
8. primary pointer button semantics are available;
9. wheel/scroll event is observable;
10. scale/DPI query returns a documented valid value;
11. actual window framebuffer can still be captured for visual evidence;
12. harness owns and cleans up only its process groups;
13. full debug tests and release build still pass.

Do not replace the dependency based only on README feature claims.

## 7. Non-goals of this contract

This document does not authorize or pre-design:

- gameplay input bindings;
- UI widget/focus-navigation architecture;
- controller support;
- raw mouse / FPS camera controls;
- audio;
- clipboard/drag-and-drop;
- networking;
- GPU rendering;
- mobile/web targets.

Those become architecture only when a bounded requirement needs them.

## 8. Completion condition

This platform capability task is complete only when the repository records the required production contract and accurately marks the current diagnostic layer as insufficient for production use.

The broader **production platform selection is still blocked** until one candidate passes the selection acceptance test above in the agent environment. Therefore the next foundation continuation must resolve this platform selection blocker before starting the separate native presentation/font/asset task.