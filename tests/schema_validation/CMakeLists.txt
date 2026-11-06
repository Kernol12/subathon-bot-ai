# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Threads REQUIRED)
add_executable(schema_transport_native_foundation_child native_foundation_child.cpp)
subathon_target_options(schema_transport_native_foundation_child)
target_compile_options(schema_transport_native_foundation_child PRIVATE -fno-exceptions -fno-rtti)
target_link_options(schema_transport_native_foundation_child PRIVATE "LINKER:-z,now")

add_executable(schema_transport_native_foundation native_foundation.cpp)
target_link_libraries(schema_transport_native_foundation PRIVATE
    subathon_schema_transport_v1 GTest::gtest_main Threads::Threads ${CMAKE_DL_LIBS})
target_compile_definitions(schema_transport_native_foundation PRIVATE
    SUBATHON_NATIVE_FOUNDATION_CHILD_PATH="$<TARGET_FILE:schema_transport_native_foundation_child>")
add_dependencies(schema_transport_native_foundation schema_transport_native_foundation_child)
subathon_target_options(schema_transport_native_foundation)
gtest_discover_tests(schema_transport_native_foundation
    DISCOVERY_MODE PRE_TEST
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
