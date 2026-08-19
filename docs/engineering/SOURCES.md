# Sources for stack architecture

Facts below were read from primary docs (Godot **4.7** stable set, CMake current `file()` / FetchContent guides, C++ Core Guidelines, GoogleTest primer, Python 3 subprocess). Blog/tutorial slogans are listed only when they match official text.

| Topic | Source | What we encoded |
| --- | --- | --- |
| Scene independence, parent injects deps, sibling mediation, Main/World/GUI, don’t parent player to disposable rooms | [Scene organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/scene_organization.html) | `godot.md` |
| Autoload is not a default manager; scene-local audio; autoload ≠ singleton | [Autoloads vs regular nodes](https://docs.godotengine.org/en/stable/tutorials/best_practices/autoloads_versus_regular_nodes.html) | `godot.md` |
| Node vs Object/RefCounted/Resource | [Node alternatives](https://docs.godotengine.org/en/stable/tutorials/best_practices/node_alternatives.html) | `godot.md` |
| Shared Resource identity; duplicate for per-instance mutation | [Resources](https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html) | `godot.md` |
| `snake_case` files, PascalCase nodes, `.gdignore` | [Project organization](https://docs.godotengine.org/en/stable/tutorials/best_practices/project_organization.html) | `godot.md` |
| Set properties before `add_child`; preload vs load | [Logic preferences](https://docs.godotengine.org/en/stable/tutorials/best_practices/logic_preferences.html) | `godot.md` |
| `_process` vs `_physics_process` | [Idle and physics processing](https://docs.godotengine.org/en/stable/tutorials/scripting/idle_and_physics_processing.html) | `godot.md` |
| InputMap actions | [Using InputEvent](https://docs.godotengine.org/en/stable/tutorials/inputs/inputevent.html) | `godot.md` |
| Typed GDScript | [Static typing](https://docs.godotengine.org/en/stable/tutorials/scripting/gdscript/static_typing.html) | `godot.md` |
| GDExtension vs modules; minor-version compatibility; float precision | [About godot-cpp](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/about_godot_cpp.html) | `gdextension.md` |
| `register_types`, `.gdextension`, ClassDB | [GDExtension C++ example](https://docs.godotengine.org/en/stable/tutorials/scripting/cpp/gdextension_cpp_example.html), [godot-cpp](https://github.com/godotengine/godot-cpp) | `gdextension.md` |
| Template layout (adapt: CMake, not SCons) | [godot-cpp-template](https://github.com/godotengine/godot-cpp-template) | `gdextension.md` |
| Godot-free core + thin binding (do **not** copy GLOB) | [godot-jolt](https://github.com/godot-jolt/godot-jolt) | `cpp.md`, `gdextension.md` |
| RAII, ownership in signatures, header hygiene | [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) R.1, I.11, SF.7, SF.11 | `cpp.md` |
| `std::expected` | [cppreference](https://en.cppreference.com/w/cpp/utility/expected) | `cpp.md` (error *channel*; TZ forbids exceptions as gameplay) |
| Independent tests, `EXPECT_`/`ASSERT_`, no `_` in `TEST` names, `gtest_main` | [GoogleTest primer](https://google.github.io/googletest/primer.html) | `cpp.md`, `cmake-python.md` |
| Do not GLOB sources | [CMake `file()`](https://cmake.org/cmake/help/latest/command/file.html) | `cmake-python.md` |
| Pinned FetchContent | [Using dependencies](https://cmake.org/cmake/help/latest/guide/using-dependencies.html) | `cmake-python.md` |
| `shell=True` | [subprocess security](https://docs.python.org/3/library/subprocess.html#security-considerations) | `cmake-python.md` |

Memorable unofficial phrasing **“call down, signal up”** (e.g. KidsCanCode) matches official sibling-mediation rules; this repo cites Godot docs as authority.

Do not treat DeepWiki, DEV.to, or random Godot 3 autoload articles as versioned authority. Godot 3.x APIs are out of contract.