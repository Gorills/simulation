include_guard(GLOBAL)
include(FetchContent)

# Initial acquisition is performed explicitly by tools/bootstrap.py / configure.
# Subsequent ordinary builds/tests do not update dependency checkouts.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Do not update populated dependencies" FORCE)

set(WORLD_SIM_GOOGLETEST_REVISION "52eb8108c5bdec04579160ae17225d66034bd723")
set(WORLD_SIM_GODOT_CPP_REVISION "9c8aeff0f58ad030f3d1030e8262de1322cd0ccd")

function(world_sim_fetch_googletest)
    if(TARGET GTest::gtest_main)
        return()
    endif()

    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG "${WORLD_SIM_GOOGLETEST_REVISION}"
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(googletest)
endfunction()

function(world_sim_fetch_godot_cpp)
    if(TARGET godot::cpp)
        return()
    endif()

    set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Godot API targeted by godot-cpp" FORCE)
    if(NOT DEFINED GODOTCPP_TARGET)
        set(GODOTCPP_TARGET "template_debug" CACHE STRING "godot-cpp build target")
    endif()
    set(GODOTCPP_PRECISION "single" CACHE STRING "Godot floating-point precision" FORCE)
    set(GODOTCPP_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(GODOTCPP_SYSTEM_HEADERS ON CACHE BOOL "" FORCE)
    set(GODOTCPP_WARNING_AS_ERROR OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        godot_cpp
        GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
        GIT_TAG "${WORLD_SIM_GODOT_CPP_REVISION}"
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(godot_cpp)
endfunction()
