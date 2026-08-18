cmake_minimum_required(VERSION 3.31)

project(simulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIM_BUILD_TESTS "Build native tests" ON)

set(SIM_WARNING_FLAGS
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

add_library(sim_core STATIC
    src/sim/simulation.cpp
)
target_include_directories(sim_core PUBLIC src)
target_compile_options(sim_core PRIVATE ${SIM_WARNING_FLAGS})

add_executable(sim_cli
    src/client-terminal/main.cpp
)
target_link_libraries(sim_cli PRIVATE sim_core)
target_compile_options(sim_cli PRIVATE ${SIM_WARNING_FLAGS})

if(SIM_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(sim_core_tests
            tests/sim/simulation_test.cpp
        )
        target_link_libraries(sim_core_tests PRIVATE sim_core)
        target_compile_options(sim_core_tests PRIVATE ${SIM_WARNING_FLAGS})
        add_test(NAME sim_core_tests COMMAND sim_core_tests)
        set_tests_properties(sim_core_tests PROPERTIES LABELS "unit;sim;protocol;determinism")
    endif()
endif()
