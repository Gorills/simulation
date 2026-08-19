# Performance contract

Status: ACTIVE

Performance is a gameplay correctness property for this project. A feature that produces correct world state but causes visible hitching, sustained low frame rate, avoidable latency, or unbounded compute/memory growth is not finished.

This document owns cross-stack performance policy. Stack guides may add implementation detail but must not weaken these rules.

## Admission rule

Every non-trivial gameplay or simulation change must answer before implementation:

1. What work does this add to the render frame, physics/simulation tick, background work, loading path, and Simulation ↔ Godot bridge?
2. What is the expected scaling variable (`entities`, `relationships`, `items`, `events`, visible presentations, etc.)?
3. Can the work be event-driven, cached, incrementally maintained, aggregated, or scheduled at a slower causal cadence instead of scanning everything every frame/tick?
4. What measurement will show whether the implementation is inside budget?

Do not reject a simple implementation merely because a theoretically faster one exists. Do reject an implementation whose scaling or critical-path cost is already unjustified by the mechanic.

## Diagnose the right problem

Never use “lag” as one undifferentiated category. Identify at least one of:

- **throughput / low FPS** — ordinary frames consistently exceed the frame budget;
- **hitch / stall** — an intermittent frame or main-thread task is abnormally long;
- **jitter / frame pacing** — motion looks uneven even when average FPS is high;
- **input latency** — input-to-presentation delay is excessive despite acceptable throughput;
- **loading/startup stall** — expensive work happens outside steady gameplay;
- **simulation backlog** — authoritative work cannot keep up with its intended cadence.

Optimization work without this classification and a before/after measurement is incomplete.

## Initial budgets

These are engineering budgets, not claims that the current build already meets them. Measure on a representative release/performance build and record the hardware/environment with evidence.

The baseline interactive target is **60 rendered frames/second**, so one frame deadline is **16.67 ms**.

Steady gameplay starts with these budgets:

- frame interval: no routine frame should miss two 60 Hz deadlines; any repeated or unexplained frame above **33.3 ms** is a blocker to investigate;
- Godot main/render-side CPU work: target **<= 8 ms p95** in representative gameplay, leaving headroom for driver/GPU/OS variance;
- real-time authoritative Simulation work associated with a 60 Hz locomotion tick: target **<= 4 ms p99** on the recorded baseline machine;
- Simulation ↔ Godot bridge/application of authoritative samples: target **<= 1 ms p99** for the observed/materialized set;
- any single synchronous task expected to consume **> 4 ms** on the gameplay/main thread requires explicit review and usually splitting, precomputation, amortization, or moving non-Godot work off the critical thread.

Budgets may be redistributed when measurements justify it, but changing the 60 Hz target or accepting routine hitches is a consequential product/performance decision, not an accidental side effect of a feature.

Long calculations may take seconds **only when they do not freeze interactive presentation**. Loading screens, bounded background jobs, offline generation, or explicitly asynchronous strategic work may have longer wall-clock duration; they must not synchronously block the render/main thread for that duration.

## Godot hot-path rules

In `_process`, `_physics_process`, camera follow, animation update, and other frame/tick callbacks:

- do not load resources, perform disk I/O, instantiate/free large object graphs, serialize JSON, rebuild large UI strings, scan the scene tree, or perform unbounded container growth;
- cache node/resource references and immutable configuration instead of repeatedly resolving them;
- update debug/UI text at a human-readable cadence or when data changes, not every rendered frame unless the text itself is the gameplay animation;
- keep physics/world-law decisions out of Godot when Simulation owns them; avoid paying twice for equivalent movement/economy/social computation;
- keep fixed-step logic and rendered presentation separate; use interpolation correctly rather than raising tick rates to hide jitter;
- prewarm/materialize expensive visual resources when first-use shader/resource creation is proven to hitch;
- use `MultiMesh`, RenderingServer/PhysicsServer, pooling, separate threads, or other lower-level paths only when measurement and object counts justify the added complexity.

Do **not** globally enable Godot separate rendering/physics thread modes as a superstition. Godot APIs have thread-safety limits and some separate-thread modes have caveats; introduce them only for a measured bottleneck with a safe ownership design.

## Simulation Core rules

The authoritative Simulation must scale by **causal work**, not by “number of things that could possibly exist in the world”.

- Do not run every system at 60 Hz. Schedule mechanics at the slowest cadence that preserves their observable causal behavior; locomotion may be fixed-step while household economy, politics, demographics, production, or relationship maintenance can be event-driven or lower-frequency.
- Do not scan the entire world each tick when dirty sets, queues, spatial/semantic locality, ownership indices, dependency tracking, or cached aggregates can bound the work.
- Preserve adaptive causal fidelity: distant or currently non-individual causality may remain aggregate; do not materialize millions of full agents merely to claim realism.
- Avoid accidental `O(N^2)`/`O(N*M)` work. If such complexity is genuinely required, document the real bounds and benchmark the representative maximum.
- Prefer compact contiguous data and predictable iteration on hot paths. Avoid pointer-heavy structures and repeated indirection without a demonstrated semantic need.
- Reuse/reserve buffers when a hot path would otherwise allocate repeatedly. Avoid allocation/deallocation on latency-critical branches when practical.
- Cache derived results only with explicit ownership/invalidation rules; a stale fast answer is still wrong.
- Parallelism is not the first optimization. When introduced, workers operate on clearly owned/immutable data and publish deterministic results at defined synchronization points. Do not place lock contention or blocking joins on the real-time critical path.
- A background job that can run for seconds must be chunkable/cancellable or otherwise unable to starve the interactive loop.

## Simulation ↔ Godot bridge rules

Crossing the GDExtension/Variant boundary is not free.

- Do not make one cross-language call per entity per frame when one bounded projection/batch can carry the same information.
- Never serialize or transfer full-world state for presentation convenience.
- Keep observation/materialization bounded before converting to Godot values.
- Debug/bootstrap dictionaries and JSON are evidence tools, not a template for future high-volume production streams.
- Measure conversion, allocation, copy volume, and application cost before choosing a high-volume representation. Do not prematurely invent a binary protocol, but do not preserve convenient `Dictionary` churn after it becomes measurable hot-path cost.
- Decouple presentation sample frequency from slower world systems; only data whose observable change cadence requires a sample should cross the boundary.

## Performance evidence

Every optimization claim requires before/after evidence on the same scenario and build class.

For Godot use, as appropriate:

- built-in Profiler / Visual Profiler;
- `Performance` monitors such as frame, physics, navigation, object and rendering metrics;
- an external CPU/GPU profiler when built-in measurements cannot identify the bottleneck;
- frame-time distributions, not average FPS alone.

For C++ Simulation use, as appropriate:

- scoped `<chrono>` measurements for an early hypothesis;
- sampling/tracing profilers for real workloads;
- repeatable benchmark executables/scenarios when a mechanic has meaningful scale;
- representative entity/relationship/item counts and algorithmic scaling evidence.

A benchmark with tiny artificial inputs that never exercises the real scaling variable does not justify a performance claim.

## Release versus debug measurements

Debug builds are for correctness and diagnosis. They may be substantially slower than production.

- Do not declare production performance from `Debug` C++/GDExtension measurements.
- Performance acceptance uses an optimized release/performance configuration representative of shipping code.
- Debug builds must still avoid pathological frame-time behavior caused by project code (for example per-frame serialization or unbounded scans), because developers need usable iteration and such smells often survive into release.

Release optimization flags such as IPO/LTO are build-level opportunities, not substitutes for fixing bad algorithms or excessive work. Enable them only through supported CMake configuration and verify the resulting build/profiling workflow.

## Regression policy

A change is not performance-safe merely because it compiles or “still feels okay”.

For a mechanic with a known representative performance scenario:

- compare before/after frame/tick distributions;
- explain a material regression or fix it before merge;
- if the feature deliberately spends more budget, record what product value consumes that budget and what remains for future systems;
- preserve the scenario so future work can reproduce the result locally.

Do not turn empirical performance budgets into CI pass/fail thresholds without a stable representative runner and scenario. Performance verification remains reproducible and evidence-backed on recorded hardware; the repository's minimal native CI is a correctness gate, not performance evidence.

## Current known smell removed by this foundation

The bootstrap HUD previously rebuilt formatted debug text and serialized a projection to JSON on every rendered frame. That work is not gameplay and has no reason to run at render cadence. The performance foundation decimates debug-HUD refresh and caches the JSON string until the underlying projection changes.

## Primary source basis

- Godot 4.7, **General optimization tips**: https://docs.godotengine.org/en/4.7/tutorials/performance/general_optimization.html
- Godot 4.7, **Performance** monitors: https://docs.godotengine.org/en/4.7/classes/class_performance.html
- Godot 4.7, **Debugger / Profiler**: https://docs.godotengine.org/en/4.7/tutorials/scripting/debug/the_profiler.html
- Godot, **Fixing jitter, stutter and input lag**: https://docs.godotengine.org/en/stable/tutorials/rendering/jitter_stutter.html
- Godot 4.7, **Physics interpolation**: https://docs.godotengine.org/en/4.7/tutorials/physics/interpolation/physics_interpolation_introduction.html
- Godot, **Thread-safe APIs**: https://docs.godotengine.org/en/stable/tutorials/performance/thread_safe_apis.html
- Godot, **Using multiple threads**: https://docs.godotengine.org/en/stable/tutorials/performance/using_multiple_threads.html
- C++ Core Guidelines, **Per: Performance**: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-performance
- CMake, **INTERPROCEDURAL_OPTIMIZATION**: https://cmake.org/cmake/help/latest/prop_tgt/INTERPROCEDURAL_OPTIMIZATION.html

These sources justify measurement-first optimization, frame/physics timing, interpolation discipline, careful thread ownership, and C++ hot-path design. Project-specific numeric budgets above are our engineering targets, not upstream defaults.
