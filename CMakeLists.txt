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

function(world_sim_enable_project_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(
            ${target}
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
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    endif()
endfunction()

add_library(sim_core STATIC)
target_sources(
    sim_core
    PRIVATE
        src/sim/grounded_locomotion.cpp
        src/sim/world.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS src
        FILES
            src/sim/grounded_locomotion.hpp
            src/sim/spatial.hpp
            src/sim/types.hpp
            src/sim/world.hpp
)
target_compile_features(sim_core PUBLIC cxx_std_23)
target_include_directories(sim_core PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>")
set_target_properties(sim_core PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
world_sim_enable_project_warnings(sim_core)

add_library(sim_protocol STATIC)
target_sources(
    sim_protocol
    PRIVATE
        src/protocol/simulation.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS src
        FILES
            src/protocol/version.hpp
            src/protocol/bootstrap_move.hpp
            src/protocol/observed_world.hpp
            src/protocol/spatial.hpp
            src/protocol/simulation.hpp
)
target_compile_features(sim_protocol PUBLIC cxx_std_23)
target_include_directories(sim_protocol PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>")
target_link_libraries(sim_protocol PRIVATE sim_core)
set_target_properties(sim_protocol PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
world_sim_enable_project_warnings(sim_protocol)

if(WORLD_SIM_BUILD_TESTS)
    find_package(Python3 3.12 COMPONENTS Interpreter REQUIRED)
    include(cmake/Dependencies.cmake)
    world_sim_fetch_googletest()

    add_executable(
        sim_core_tests
        tests/sim/grounded_locomotion_test.cpp
        tests/sim/world_test.cpp
    )
    target_link_libraries(sim_core_tests PRIVATE sim_core GTest::gtest_main)
    target_compile_features(sim_core_tests PRIVATE cxx_std_23)
    set_target_properties(sim_core_tests PROPERTIES CXX_EXTENSIONS OFF)
    world_sim_enable_project_warnings(sim_core_tests)

    add_executable(protocol_tests tests/protocol/simulation_test.cpp)
    target_link_libraries(protocol_tests PRIVATE sim_protocol GTest::gtest_main)
    target_compile_features(protocol_tests PRIVATE cxx_std_23)
    set_target_properties(protocol_tests PROPERTIES CXX_EXTENSIONS OFF)
    world_sim_enable_project_warnings(protocol_tests)

    include(GoogleTest)
    gtest_discover_tests(sim_core_tests PROPERTIES LABELS "unit;sim;determinism")
    gtest_discover_tests(protocol_tests PROPERTIES LABELS "unit;protocol;determinism")

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
    target_link_libraries(world_sim_gdextension PRIVATE sim_protocol godot::cpp)
    set_target_properties(world_sim_gdextension PROPERTIES CXX_EXTENSIONS OFF)
    world_sim_enable_project_warnings(world_sim_gdextension)

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
