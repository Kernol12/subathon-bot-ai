# SPDX-License-Identifier: GPL-3.0-or-later
# Standalone compile-only harness. The integrated test target is coordinator-owned.
cmake_minimum_required(VERSION 3.25)
project(Rates012Compile LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql Test)
add_subdirectory(../../.. protected EXCLUDE_FROM_ALL)
function(import_fixture_paths)
    get_filename_component(PROJECT_SOURCE_DIR "../../.." ABSOLUTE)
    include("${PROJECT_SOURCE_DIR}/cmake/AcceptanceRegistration.cmake")
endfunction()
import_fixture_paths()

add_executable(rates_012_stage1 tests.cpp missing.cpp ../../support/credential_native_controls.cpp)
set_target_properties(rates_012_stage1 PROPERTIES AUTOMOC ON)
target_include_directories(rates_012_stage1 PRIVATE ../../../server/src)
target_link_libraries(rates_012_stage1 PRIVATE
    Qt6::Test subathon_test_fixtures subathon_credential_storage subathon_sqlite_credential_catalog
    subathon_open_exchange_rates subathon_exchange_rate_api Sodium::Sodium)
foreach(symbol IN ITEMS sodium_init randombytes_buf sodium_malloc sodium_mlock sodium_free)
    target_link_options(rates_012_stage1 PRIVATE "LINKER:--wrap=${symbol}")
endforeach()
target_link_options(rates_012_stage1 PRIVATE
    "LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/rates_012_stage1.map")
