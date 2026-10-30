# SPDX-License-Identifier: GPL-3.0-or-later
# Disposable feasibility probe; intentionally outside the product build.
cmake_minimum_required(VERSION 3.25)
project(SubathonQueueFeasibility LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(SPIKE_REQUIRE_DROGON "Require real Drogon for the dependency probe" OFF)
if(SPIKE_REQUIRE_DROGON)
    find_package(Drogon CONFIG REQUIRED)
endif()
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql)
find_package(Threads REQUIRED)
add_executable(spike_001_queue queue_probe.cpp)
target_link_libraries(spike_001_queue PRIVATE Qt6::Core Qt6::Sql Threads::Threads)
target_compile_options(spike_001_queue PRIVATE -Wall -Wextra -Wpedantic)
enable_testing()
add_test(NAME spike_001.synthetic_queue_writer_drain COMMAND spike_001_queue)
set_tests_properties(spike_001.synthetic_queue_writer_drain PROPERTIES TIMEOUT 15)


# Explicit opt-in: the normal project configure never downloads Drogon.
option(SPIKE_BUILD_REAL_BRIDGE "Build the isolated real Drogon/Qt probe" OFF)
if(SPIKE_BUILD_REAL_BRIDGE)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "The disposable real bridge experiment currently targets Linux only")
    endif()
    find_package(Drogon 1.9.14 EXACT CONFIG REQUIRED)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_executable(spike_001_bridge bridge_probe.cpp)
    target_link_libraries(spike_001_bridge PRIVATE Drogon::Drogon Qt6::Core Qt6::Sql Threads::Threads)
    target_compile_options(spike_001_bridge PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME spike_001.real_http_websocket_queue_shutdown
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_bridge.py"
                --probe "$<TARGET_FILE:spike_001_bridge>" -v)
    set_tests_properties(spike_001.real_http_websocket_queue_shutdown
        PROPERTIES TIMEOUT 120 LABELS "feasibility;loopback")
endif()
