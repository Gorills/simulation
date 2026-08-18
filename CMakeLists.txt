cmake_minimum_required(VERSION 3.31)

project(simulation_foundation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIM_BUILD_TESTS "Build foundation tests" ON)

function(sim_apply_project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
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
    endif()
endfunction()

add_executable(platform_graphics_smoke
    src/platform/graphics_smoke.cpp
)
target_include_directories(platform_graphics_smoke SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fenster
)
sim_apply_project_warnings(platform_graphics_smoke)

if(WIN32)
    target_link_libraries(platform_graphics_smoke PRIVATE gdi32 user32)
elseif(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    target_link_libraries(platform_graphics_smoke PRIVATE ${COCOA_FRAMEWORK})
else()
    find_package(X11 REQUIRED)
    target_link_libraries(platform_graphics_smoke PRIVATE X11::X11)

    add_executable(platform_x11_key_sender
        tools/platform/x11_key_sender.cpp
    )
    target_link_libraries(platform_x11_key_sender PRIVATE X11::X11)
    sim_apply_project_warnings(platform_x11_key_sender)

    add_executable(platform_x11_capture
        tools/platform/x11_capture.cpp
    )
    target_link_libraries(platform_x11_capture PRIVATE X11::X11)
    sim_apply_project_warnings(platform_x11_capture)
endif()

if(SIM_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(foundation_tests
            tests/foundation/foundation_test.cpp
        )
        sim_apply_project_warnings(foundation_tests)
        add_test(NAME foundation_tests COMMAND foundation_tests)
        set_tests_properties(foundation_tests PROPERTIES LABELS "foundation;toolchain")

        add_test(
            NAME graphics_framebuffer_self_test
            COMMAND platform_graphics_smoke --self-test
        )
        set_tests_properties(graphics_framebuffer_self_test PROPERTIES LABELS "foundation;graphics")
    endif()
endif()
