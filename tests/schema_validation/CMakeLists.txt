# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Threads REQUIRED)
# Both initial artifacts use the exact reviewed, literal31-only entry source.
# The additional fault controls are test infrastructure, never another owner.
add_library(subathon_schema_fault_v1 SHARED
    "${PROJECT_SOURCE_DIR}/tools/schema_validation/native/transport_stub.cpp"
    native_fault_fixture.cpp)
target_include_directories(subathon_schema_fault_v1 PRIVATE
    "${PROJECT_SOURCE_DIR}/tools/schema_validation/native")
target_link_libraries(subathon_schema_fault_v1 PRIVATE Threads::Threads)
subathon_target_options(subathon_schema_fault_v1)
target_compile_options(subathon_schema_fault_v1 PRIVATE -fno-exceptions -fno-rtti)
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

# Intentionally separate from green facilities until the reviewed native red gate.
add_test(NAME schema-native.transport-acceptance
    COMMAND /usr/bin/env -i PATH=/usr/bin:/bin LC_ALL=C.UTF-8 LANG=C.UTF-8
        "${Python3_EXECUTABLE}" -I -B
        "${CMAKE_CURRENT_SOURCE_DIR}/native_transport_acceptance.py"
        --artifact "$<TARGET_FILE:subathon_schema_transport_v1>")
set_tests_properties(schema-native.transport-acceptance PROPERTIES
    LABELS "schema-native-behavior;acceptance;intentional-red" TIMEOUT 120)
