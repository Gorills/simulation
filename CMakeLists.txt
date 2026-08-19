cmake_minimum_required(VERSION 3.25)

project(
    world_simulation
    VERSION 0.1.0
    DESCRIPTION "Authoritative C++ world simulation with a Godot GDExtension client"
    LANGUAGES CXX
)

include(CTest)

option(WORLD_SIM_BUILD_TESTS "Build native world simulation tests" ${BUILD_TESTING})
option(WORLD_SIM_BUILD_GDEXTENSION "Build the Godot GDExtension adapter" ON)

add_library(sim_core STATIC)
target_sources(
    sim_core
    PRIVATE
        src/sim/world.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS src
        FILES
            src/protocol/move.hpp
            src/sim/world.hpp
)
target_compile_features(sim_core PUBLIC cxx_std_23)
target_include_directories(sim_core PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>")
set_target_properties(sim_core PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        sim_core
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wformat=2
            -Wundef
            -Wnon-virtual-dtor
            -Wold-style-cast
    )
elseif(MSVC)
    target_compile_options(sim_core PRIVATE /W4 /permissive-)
endif()

if(WORLD_SIM_BUILD_TESTS)
    find_package(Python3 3.12 COMPONENTS Interpreter REQUIRED)
    include(cmake/Dependencies.cmake)
    world_sim_fetch_googletest()

    add_executable(sim_tests tests/sim/world_test.cpp)
    target_link_libraries(sim_tests PRIVATE sim_core GTest::gtest_main)
    target_compile_features(sim_tests PRIVATE cxx_std_23)
    set_target_properties(sim_tests PROPERTIES CXX_EXTENSIONS OFF)

    include(GoogleTest)
    gtest_discover_tests(sim_tests PROPERTIES LABELS "unit;sim;determinism")

    add_test(
        NAME architecture_no_godot_in_core
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/check_architecture.py"
    )
    set_tests_properties(architecture_no_godot_in_core PROPERTIES LABELS "architecture")
endif()

if(WORLD_SIM_BUILD_GDEXTENSION)
    include(cmake/Dependencies.cmake)
    world_sim_fetch_godot_cpp()

    add_library(world_sim_gdextension SHARED)
    target_sources(
        world_sim_gdextension
        PRIVATE
            src/adapters/gdextension/register_types.cpp
            src/adapters/gdextension/sim_facade.cpp
            src/adapters/gdextension/sim_facade.hpp
    )
    target_compile_features(world_sim_gdextension PRIVATE cxx_std_23)
    target_link_libraries(world_sim_gdextension PRIVATE sim_core godot::cpp)
    set_target_properties(world_sim_gdextension PROPERTIES CXX_EXTENSIONS OFF)

    if(GODOTCPP_TARGET STREQUAL "template_release")
        set(_world_sim_gdextension_variant "template_release")
    else()
        set(_world_sim_gdextension_variant "template_debug")
    endif()

    set_target_properties(
        world_sim_gdextension
        PROPERTIES
            OUTPUT_NAME "world_sim.${_world_sim_gdextension_variant}"
            LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/godot/bin"
            RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/godot/bin"
    )
endif()
