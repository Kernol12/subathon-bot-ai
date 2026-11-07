# SPDX-License-Identifier: GPL-3.0-or-later
include_guard(GLOBAL)

# Test-only paths stay on an interface target, never on production libraries.
add_library(subathon_test_fixtures INTERFACE)
target_compile_definitions(subathon_test_fixtures INTERFACE
    SUBATHON_REFERENCE_CAPTURE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_reference_capture_vectors.json"
    SUBATHON_SERVICE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/application_service_vectors.json"
    SUBATHON_HOLD_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_holds_vectors.json"
    SUBATHON_ORIGINAL_CLAIM_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_original_claims_vectors.json"
    SUBATHON_BUNDLE_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_bundle_vectors.json"
    SUBATHON_PLAN_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_application_plans_vectors.json"
    SUBATHON_RETAINED_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/retained_context_vectors.json"
    SUBATHON_RECEIPT_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_receipts_vectors.json"
    SUBATHON_CONTRIBUTION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_contributions_vectors.json"
    SUBATHON_MIGRATION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/migration_catalog_vectors.json"
    SUBATHON_ACCOUNT_SESSION_VECTOR_PATH="${PROJECT_SOURCE_DIR}/tests/fixtures/sqlite_account_sessions_vectors.json"
    SUBATHON_FIXTURE_DIRECTORY="${PROJECT_SOURCE_DIR}/tests/fixtures")

# Qt's own runner discovers every slot/data row. Keep one process per executable;
# source assertions, init/cleanup hooks and the actual test object are unchanged.
function(subathon_add_qt_suite target)
    cmake_parse_arguments(ARG "" "NAME;TIMEOUT" "SOURCES;LIBRARIES;LABELS" ${ARGN})
    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 120)
    endif()
    add_executable(${target} ${ARG_SOURCES})
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_link_libraries(${target} PRIVATE ${ARG_LIBRARIES} Qt6::Test subathon_test_fixtures)
    subathon_target_options(${target})
    add_test(NAME "${ARG_NAME}.suite" COMMAND ${target})
    set_tests_properties("${ARG_NAME}.suite" PROPERTIES
        LABELS "${ARG_LABELS}" TIMEOUT "${ARG_TIMEOUT}")
endfunction()

# GoogleTest keeps standard per-case processes, including sodium initialization
# failure cases. Apply list labels after discovery for older supported CMake too.
function(subathon_discover_gtests target)
    cmake_parse_arguments(ARG "NO_PRETTY_VALUES" "PREFIX;TIMEOUT" "LABELS" ${ARGN})
    set(discovery TEST_LIST "${target}_cases" DISCOVERY_MODE PRE_TEST)
    if(ARG_NO_PRETTY_VALUES)
        list(APPEND discovery NO_PRETTY_VALUES)
    endif()
    if(ARG_PREFIX)
        list(APPEND discovery TEST_PREFIX "${ARG_PREFIX}")
    endif()
    if(ARG_TIMEOUT)
        list(APPEND discovery PROPERTIES TIMEOUT "${ARG_TIMEOUT}")
    endif()
    gtest_discover_tests(${target} ${discovery})
    set(SBT_CASE_LIST "${target}_cases")
    set(SBT_CASE_LABELS "${ARG_LABELS}")
    set(labels_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_labels.cmake")
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/DiscoveredLabels.cmake.in"
        "${labels_file}" @ONLY)
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${labels_file}")
endfunction()
