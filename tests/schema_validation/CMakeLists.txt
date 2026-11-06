# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Threads REQUIRED)
# Both artifacts instantiate the SAME entry/common algorithm with compile-time Ops.
# The additional fault controls are test infrastructure, never another owner.
add_library(subathon_schema_fault_v1 SHARED
    "${PROJECT_SOURCE_DIR}/tools/schema_validation/native/transport_entry.cpp"
    native_fault_fixture.cpp)
target_include_directories(subathon_schema_fault_v1 PRIVATE
    "${PROJECT_SOURCE_DIR}/tools/schema_validation/native" "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(subathon_schema_fault_v1 PRIVATE
    SUBATHON_SCHEMA_NATIVE_FAULT_OPS=1
    SUBATHON_NATIVE_FAULT_CHILD_PATH="$<TARGET_FILE:schema_transport_native_fault_child>")
add_dependencies(subathon_schema_fault_v1 schema_transport_native_fault_child)
target_link_libraries(subathon_schema_fault_v1 PRIVATE Threads::Threads)
subathon_target_options(subathon_schema_fault_v1)
target_compile_options(subathon_schema_fault_v1 PRIVATE -fno-exceptions -fno-rtti -fstack-usage)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(subathon_schema_fault_v1 PRIVATE -fcallgraph-info=su)
endif()
set_target_properties(subathon_schema_fault_v1 PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
target_link_options(subathon_schema_fault_v1 PRIVATE
    "LINKER:-z,now" "LINKER:-z,relro"
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/native_fault_v1.map")
set_property(TARGET subathon_schema_fault_v1 APPEND PROPERTY
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/native_fault_v1.map")

add_executable(schema_transport_native_fault_child native_fault_child.cpp)
subathon_target_options(schema_transport_native_fault_child)
target_compile_options(schema_transport_native_fault_child PRIVATE -fno-exceptions -fno-rtti)
target_link_options(schema_transport_native_fault_child PRIVATE "LINKER:-z,now")

add_executable(schema_transport_native_foundation_child native_foundation_child.cpp)
subathon_target_options(schema_transport_native_foundation_child)
target_compile_options(schema_transport_native_foundation_child PRIVATE -fno-exceptions -fno-rtti)
target_link_options(schema_transport_native_foundation_child PRIVATE "LINKER:-z,now")

add_executable(schema_transport_native_foundation
    native_foundation.cpp native_fault_foundation.cpp native_fault_fixture.cpp
    native_fault_child_foundation.cpp)
target_link_libraries(schema_transport_native_foundation PRIVATE
    subathon_schema_transport_v1 GTest::gtest_main Threads::Threads ${CMAKE_DL_LIBS})
target_compile_definitions(schema_transport_native_foundation PRIVATE
    SUBATHON_NATIVE_FOUNDATION_CHILD_PATH="$<TARGET_FILE:schema_transport_native_foundation_child>"
    SUBATHON_NATIVE_FAULT_CHILD_PATH="$<TARGET_FILE:schema_transport_native_fault_child>")
add_dependencies(schema_transport_native_foundation
    schema_transport_native_foundation_child schema_transport_native_fault_child)
subathon_target_options(schema_transport_native_foundation)
gtest_discover_tests(schema_transport_native_foundation
    DISCOVERY_MODE PRE_TEST
    NO_PRETTY_VALUES
    TEST_PREFIX "schema-native-foundation."
    PROPERTIES LABELS "schema-native-foundation;foundation" TIMEOUT 30)

add_test(NAME schema-native.loader-foundation
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -S -B
        "${CMAKE_CURRENT_SOURCE_DIR}/native_loader_foundation.py"
        --artifact "$<TARGET_FILE:subathon_schema_transport_v1>")
set_tests_properties(schema-native.loader-foundation PROPERTIES
    LABELS "schema-native-foundation;foundation" TIMEOUT 30)

# Reviewed behavior baseline is preserved in evidence; implemented acceptance is unconditional.
add_test(NAME schema-native.transport-acceptance
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -B
        "${CMAKE_CURRENT_SOURCE_DIR}/native_transport_acceptance.py"
        --artifact "$<TARGET_FILE:subathon_schema_transport_v1>")
set_tests_properties(schema-native.transport-acceptance PROPERTIES
    LABELS "schema-native-behavior;acceptance" TIMEOUT 120)

# Every closed fault row gets a fresh ordinary-venv process. No hidden load_tests.
foreach(row IN ITEMS
    a1 a2 a3 o4 o5 o6 o7 o8 o9 o10 s11 s12 e13 e14 e15 e16 e17 e18 e19 e20 e21 e22 d23 p1 p2 p3 p4 p5 p6 p7 p8 w1 w2 w3 w4 w5 w6 w7 w8 w9 w10 w11 w12 w13 w14 w15 w16 w17 w18 c1 c2 c3 i25a i25b r26 l27 f28)
    add_test(NAME schema-native.owner-${row}
        COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
            "${Python3_EXECUTABLE}" -I -B
            "${CMAKE_CURRENT_SOURCE_DIR}/native_owner_acceptance.py"
            --artifact "$<TARGET_FILE:subathon_schema_fault_v1>" --row "${row}")
    set_tests_properties(schema-native.owner-${row} PROPERTIES
        LABELS "schema-native-behavior;acceptance" TIMEOUT 40)
endforeach()

# Public and binding suites require the same reviewed native build and real SDK.
foreach(suite IN ITEMS native_binding_acceptance native_loader_cleanup_regression
        wrapper_acceptance parent_cleanup_regression
        worker_context_regression worker_lexical_regression parent_spawn_regression)
    add_test(NAME schema-native.${suite}
        COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
            "${Python3_EXECUTABLE}" -I -B "${CMAKE_CURRENT_SOURCE_DIR}/${suite}.py")
    set_tests_properties(schema-native.${suite} PROPERTIES
        LABELS "schema-public-behavior;acceptance" TIMEOUT 180)
endforeach()

# Independently reviewed reached-bug regression; now passing with active-work classification.
add_test(NAME schema-native.native_normalization_close_regression
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -B
        "${CMAKE_CURRENT_SOURCE_DIR}/native_normalization_close_regression.py"
        --artifact "$<TARGET_FILE:subathon_schema_fault_v1>")
set_tests_properties(schema-native.native_normalization_close_regression PROPERTIES
    LABELS "schema-native-behavior;acceptance" TIMEOUT 40)
