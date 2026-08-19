# Primary sources for engineering guidance

This file records the upstream evidence behind `docs/engineering/`. It is not a generic reading list: sources are kept when they justify a project rule or compatibility fact.

Prefer versioned official documentation, project upstream repositories, language/library references and established guidelines. Do not promote random tutorials, generated summaries or old Godot 3 material into architectural authority.

## Godot client

| Topic | Primary source | Encoded in |
| --- | --- | --- |
| scene independence, parent-mediated sibling wiring, persistent Main/World/GUI structure | Godot [Scene organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/scene_organization.html) | [`godot.md`](godot.md) |
| Autoload trade-offs | Godot [Autoloads versus regular nodes](https://docs.godotengine.org/en/stable/tutorials/best_practices/autoloads_versus_regular_nodes.html) | `godot.md` |
| Node vs Object/RefCounted/Resource | Godot [Node alternatives](https://docs.godotengine.org/en/stable/tutorials/best_practices/node_alternatives.html) | `godot.md` |
| Resource sharing/identity | Godot [Resources](https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html) | `godot.md` |
| project/file conventions | Godot [Project organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/project_organization.html) | `godot.md` |
| runtime node setup preferences | Godot [Logic preferences](https://docs.godotengine.org/en/stable/tutorials/best_practices/logic_preferences.html) | `godot.md` |
| `_process` vs `_physics_process` | Godot [Idle and physics processing](https://docs.godotengine.org/en/stable/tutorials/scripting/idle_and_physics_processing.html) | `godot.md` |
| semantic input actions | Godot [Using InputEvent / InputMap](https://docs.godotengine.org/en/stable/tutorials/inputs/inputevent.html) | `godot.md` |
| typed GDScript | Godot [Static typing](https://docs.godotengine.org/en/stable/tutorials/scripting/gdscript/static_typing.html) | `godot.md` |

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
- Unofficial mnemonic phrasing may help an explanation, but the rule must be grounded in an upstream/canonical source.
- When upstream behavior changes, update the project guide and source record together; do not preserve stale compatibility claims for consistency with old prose.
- Version-specific facts belong in [`VERSIONS.md`](VERSIONS.md) and eventually machine-readable build/lock files, not scattered across guides.
