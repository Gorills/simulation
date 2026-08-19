# Primary sources for engineering guidance

This file records the upstream evidence behind `docs/engineering/`. It is not a generic reading list: sources are kept when they justify a project rule or compatibility fact.

Prefer versioned official documentation, project upstream repositories, language/library references and established guidelines. Do not promote random tutorials, generated summaries or old Godot 3 material into architectural authority.

## Godot client

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| scene independence, parent-mediated sibling wiring, persistent composition roots | Godot [Scene organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/scene_organization.html) | [`godot.md`](godot.md) |
| Autoload trade-offs | Godot [Autoloads versus regular nodes](https://docs.godotengine.org/en/stable/tutorials/best_practices/autoloads_versus_regular_nodes.html) | `godot.md` |
| Node vs Object/RefCounted/Resource | Godot [Node alternatives](https://docs.godotengine.org/en/stable/tutorials/best_practices/node_alternatives.html) | `godot.md` |
| Resource sharing/identity | Godot [Resources](https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html) | `godot.md` |
| project/file conventions | Godot [Project organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/project_organization.html) | `godot.md` |
| runtime node setup preferences | Godot [Logic preferences](https://docs.godotengine.org/en/stable/tutorials/best_practices/logic_preferences.html) | `godot.md` |
| `_process` vs `_physics_process` | Godot [Idle and physics processing](https://docs.godotengine.org/en/stable/tutorials/scripting/idle_and_physics_processing.html) | `godot.md` |
| semantic input actions | Godot [Using InputEvent / InputMap](https://docs.godotengine.org/en/stable/tutorials/inputs/inputevent.html) | `godot.md` |
| controller mappings, circular deadzone and `Input.get_vector()` | Godot 4.7 [Controllers, gamepads, and joysticks](https://docs.godotengine.org/en/4.7/tutorials/inputs/controllers_gamepads_joysticks.html) | `godot.md`, ADR 0002 |
| captured mouse mouselook and `screen_relative` | Godot 4.7 [`InputEventMouseMotion`](https://docs.godotengine.org/en/4.7/classes/class_inputeventmousemotion.html) and [Mouse and input coordinates](https://docs.godotengine.org/en/4.7/tutorials/inputs/mouse_and_input_coordinates.html) | `godot.md`, ADR 0002 |
| third-person camera collision | Godot 4.7 [Third-person camera with spring arm](https://docs.godotengine.org/en/4.7/tutorials/3d/spring_arm.html) | `godot.md`, ADR 0002 |
| fixed-step smooth rendering and manual camera follow | Godot 4.7 [Physics interpolation quick start](https://docs.godotengine.org/en/4.7/tutorials/physics/interpolation/physics_interpolation_quick_start_guide.html) and [Advanced physics interpolation](https://docs.godotengine.org/en/4.7/tutorials/physics/interpolation/advanced_physics_interpolation.html) | `godot.md`, ADR 0002 |
| custom interpolation can better fit externally timed authoritative samples | Godot 4.7 [Physics interpolation introduction](https://docs.godotengine.org/en/4.7/tutorials/physics/interpolation/physics_interpolation_introduction.html) | [`simulation-godot-boundary.md`](simulation-godot-boundary.md), ADR 0004 |
| official third-person input/camera/movement reference | [`godotengine/tps-demo`](https://github.com/godotengine/tps-demo) — `player/player_input.gd`, `player/player.gd` | `godot.md`, ADR 0002 |
| Godot 4.7 `CharacterBody3D`, camera-relative movement, acceleration/deceleration and interpolation reference | [`godot-demo-projects` 4.7 `3d/platformer`](https://github.com/godotengine/godot-demo-projects/tree/4.7-6ad6167/3d/platformer) | `godot.md`, ADR 0002 |
| commercial precedent that movement response needs explicit tuning | CD PROJEKT RED [The Witcher 3 patch 1.07 changelog](https://www.thewitcher.com/us/en/news/1081/patch-1-07-changelog) | ADR 0002 |
| typed GDScript | Godot [Static typing](https://docs.godotengine.org/en/stable/tutorials/scripting/gdscript/static_typing.html) | `godot.md` |
| project-wide Theme and theme lookup/cascade | Godot 4.7 [Introduction to GUI skinning](https://docs.godotengine.org/en/4.7/tutorials/ui/gui_skinning.html) and [`Theme`](https://docs.godotengine.org/en/4.7/classes/class_theme.html) | [`ui-design-system.md`](ui-design-system.md), ADR 0003 |
| semantic Theme type variations instead of repeated local overrides | Godot 4.7 [Theme type variations](https://docs.godotengine.org/en/4.7/tutorials/ui/gui_theme_type_variations.html) | `ui-design-system.md`, ADR 0003 |
| responsive complex/RPG layout through nested Containers | Godot 4.7 [Using Containers](https://docs.godotengine.org/en/4.7/tutorials/ui/gui_containers.html) | `ui-design-system.md`, ADR 0003 |
| keyboard/controller focus and separation of built-in UI actions from gameplay | Godot 4.7 [Keyboard/Controller Navigation and Focus](https://docs.godotengine.org/en/4.7/tutorials/ui/gui_navigation.html) | `ui-design-system.md`, ADR 0003 |
| multi-resolution `canvas_items` / `expand` strategy | Godot 4.7 [Multiple resolutions](https://docs.godotengine.org/en/4.7/tutorials/rendering/multiple_resolutions.html) and [4.7 changed defaults](https://docs.godotengine.org/en/4.7/tutorials/migrating/upgrading_to_godot_4.7.html#changed-defaults) | `ui-design-system.md`, ADR 0003 |

The Witcher changelog is used only as product-design evidence that movement response is a deliberate player-facing tuning concern. No proprietary Witcher implementation or numeric tuning value is treated as a project API.

## Simulation / presentation boundary

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| separate task-oriented writes from purpose-built read models | Microsoft Azure Architecture Center [CQRS pattern](https://learn.microsoft.com/en-us/azure/architecture/patterns/cqrs) | [`simulation-godot-boundary.md`](simulation-godot-boundary.md), ADR 0004 |
| isolate/translate between subsystems with different semantics instead of leaking one model into another | Microsoft Azure Architecture Center [Anti-Corruption Layer pattern](https://learn.microsoft.com/en-us/azure/architecture/patterns/anti-corruption-layer) | `simulation-godot-boundary.md`, ADR 0004 |
| filter a large virtual environment to a relevant presentation subset | Liu & Theodoropoulos, ACM Computing Surveys / IBM Research [Interest management for distributed virtual environments: A survey](https://research.ibm.com/publications/interest-management-for-distributed-virtual-environments-a-survey) | `simulation-godot-boundary.md`, ADR 0004 |
| heterogeneous autonomous actors and emergent outcomes from individual interactions | Eric Bonabeau, PNAS [Agent-based modeling: Methods and techniques for simulating human systems](https://pmc.ncbi.nlm.nih.gov/articles/PMC128598/) | [`../MODELING.md`](../MODELING.md), ADR 0004 |

These are conceptual precedents, not dependency choices. This project does **not** import Azure services, a distributed CQRS stack, networking middleware or full event sourcing. Commands/projections, translation boundaries and bounded presentation sets are adapted to one in-process Simulation + Godot client because those concepts protect authority and presentation ownership here.

## UI accessibility / interaction quality

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| minimum default text size and future text scaling | Microsoft Xbox Accessibility Guideline 101 [Text display](https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/101) | `ui-design-system.md`, ADR 0003 |
| contrast as a text/non-text legibility requirement | Microsoft Xbox Accessibility Guideline 102 [Contrast](https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/102) | `ui-design-system.md`, ADR 0003 |
| highly visible, persistent UI focus indicator | Microsoft Xbox Accessibility Guideline 113 [UI focus handling](https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/113) | `ui-design-system.md`, ADR 0003 |

These guidelines establish quality/accessibility floors, not the game's art direction. The project can exceed them while preserving its own visual identity.

## GDExtension / godot-cpp

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| GDExtension/godot-cpp compatibility and engine-module distinction | Godot [About godot-cpp](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/about_godot_cpp.html) | [`gdextension.md`](gdextension.md), [`VERSIONS.md`](VERSIONS.md) |
| registration and extension example | Godot [GDExtension C++ example](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/gdextension_cpp_example.html) | `gdextension.md` |
| manifest compatibility fields | Godot 4.7 [`.gdextension` file](https://docs.godotengine.org/en/4.7/engine_details/engine_api/gdextension/gdextension_file.html) | `VERSIONS.md`, `gdextension.md` |
| GDExtension compatibility model | Godot 4.7 [What is GDExtension?](https://docs.godotengine.org/en/4.7/engine_details/engine_api/gdextension/what_is_gdextension.html) | `VERSIONS.md` |
| current binding/version/build behavior | [`godotengine/godot-cpp`](https://github.com/godotengine/godot-cpp) | `VERSIONS.md`, `gdextension.md` |
| template conventions | [`godot-cpp-template`](https://github.com/godotengine/godot-cpp-template) | `gdextension.md` (adapt conventions, not its SCons graph) |
| Godot-free native library plus thin binding as a structural precedent | [`godot-jolt`](https://github.com/godot-jolt/godot-jolt) | `cpp.md`, `gdextension.md` |

## C++

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| RAII/ownership/interface/header guidance | [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) | [`cpp.md`](cpp.md) |
| C++23 `std::expected` semantics | [cppreference `std::expected`](https://en.cppreference.com/w/cpp/utility/expected) | `cpp.md` |

The Core Guidelines are guidance, not a license to import every recommended abstraction. This project applies them together with the current architecture and avoids speculative interface layers.

## CMake / GoogleTest / Python

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| source GLOB warning | CMake [`file()`](https://cmake.org/cmake/help/latest/command/file.html) | [`cmake-python.md`](cmake-python.md) |
| dependency/FetchContent guidance | CMake [Using dependencies](https://cmake.org/cmake/help/latest/guide/using-dependencies.html) | `cmake-python.md` |
| independent tests and assertion guidance | [GoogleTest Primer](https://google.github.io/googletest/primer.html) | `cpp.md`, `cmake-python.md` |
| subprocess shell/security behavior | Python [`subprocess` security considerations](https://docs.python.org/3/library/subprocess.html#security-considerations) | `cmake-python.md`, [`../VERIFICATION.md`](../VERIFICATION.md) |

## Source-use rules

- Stable/versioned official docs beat old tutorials when APIs or compatibility differ.
- A large production repository may provide a useful structural precedent without making every one of its build choices appropriate here.
- Commercial games may motivate a UX requirement without their internal implementation becoming a project dependency or guessed contract.
- Unofficial mnemonic phrasing may help an explanation, but the rule must be grounded in an upstream/canonical source.
- When upstream behavior changes, update the project guide and source record together; do not preserve stale compatibility claims for consistency with old prose.
- Version-specific facts belong in [`VERSIONS.md`](VERSIONS.md) and machine-readable build/lock files, not scattered across guides.
