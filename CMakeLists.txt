cmake_minimum_required(VERSION 3.25)
project(SubathonBot VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
option(SUBATHON_BUILD_ACCEPTANCE_TESTS "Build selected component acceptance suites" OFF)
# Retain the reviewed baseline selector for ordinary API-011 acceptance.
option(SUBATHON_BUILD_REFERENCE_INTAKE_BASELINE "Build reviewed API-011 Qt acceptance" OFF)
set(SUBATHON_ACCEPTANCE_SUITES "time-credit" CACHE STRING "Acceptance suite/profile names, or all")
option(SUBATHON_BUILD_SCHEMA_VALIDATION_TESTS "Check repository schemas and examples with the pinned Python validators" OFF)
option(SUBATHON_BUILD_QT_PROBE "Configure Qt 6.12 Core/Test dependency discovery" OFF)
option(SUBATHON_FETCH_GOOGLETEST "Explicitly allow fetching the pinned GoogleTest source" OFF)
include(cmake/AcceptanceSuites.cmake)
subathon_resolve_acceptance_suites(sbt_selected_suites)
foreach(suite IN LISTS subathon_acceptance_suites)
    _subathon_acceptance_flag("${suite}" flag)
    set(${flag} OFF)
endforeach()
foreach(suite IN LISTS sbt_selected_suites)
    _subathon_acceptance_flag("${suite}" flag)
    set(${flag} ON)
endforeach()
if(SUBATHON_BUILD_DEV_SERVER_TESTS)
    set(SUBATHON_BUILD_REFERENCE_INTAKE_BASELINE ON)
endif()
if(SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS)
    set(SUBATHON_BUILD_QT_PROBE ON)
endif()
message(STATUS "Selected acceptance suites: ${sbt_selected_suites}")
set(SUBATHON_GOOGLETEST_SOURCE_DIR "" CACHE PATH "Optional local GoogleTest source checkout")

# Product discovery also works in a build with no test packages.
find_package(Qt6 6.12 QUIET COMPONENTS Core OPTIONAL_COMPONENTS Sql)
if(BUILD_TESTING AND (SUBATHON_BUILD_EXACT_NUMERIC_TESTS OR SUBATHON_BUILD_APPLICATION_PLAN_TESTS
        OR SUBATHON_BUILD_SQLITE_PLAN_TESTS OR SUBATHON_BUILD_MODIFIER_STACKING_TESTS))
    find_package(Boost 1.83 CONFIG REQUIRED)
else()
    find_package(Boost 1.83 CONFIG QUIET)
endif()
if(SUBATHON_DROGON_ROOT OR (BUILD_TESTING AND SUBATHON_BUILD_NATIVE_WEBSOCKET_PROFILE_TESTS))
    include(cmake/DrogonDependency.cmake)
endif()
if(SUBATHON_SODIUM_ROOT OR (BUILD_TESTING AND (SUBATHON_BUILD_PASSWORD_HASHING_TESTS
        OR SUBATHON_BUILD_SESSION_RESOLUTION_TESTS OR SUBATHON_BUILD_SQLITE_ACCOUNT_SESSION_TESTS
        OR SUBATHON_BUILD_CREDENTIAL_RECORD_TESTS OR SUBATHON_BUILD_SQLITE_CREDENTIALS_TESTS
        OR SUBATHON_BUILD_SQLITE_CREDENTIAL_CATALOG_TESTS
        OR (sbt_bounded_http_successor AND SUBATHON_BUILD_BOUNDED_HTTP_FOUNDATION_TESTS))))
    include(cmake/SodiumDependency.cmake)
endif()
include(cmake/ProjectOptions.cmake)
add_subdirectory(core)
# Build server components when requested directly or pulled in by a consumer.
add_subdirectory(server EXCLUDE_FROM_ALL)
add_executable(subathon_scaffold_info scaffold/main.cpp)
subathon_target_options(subathon_scaffold_info)
if(BUILD_TESTING)
    if(SUBATHON_BUILD_SQLITE_WRITER_TESTS)
        find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql Test)
    elseif(SUBATHON_BUILD_QT_PROBE OR SUBATHON_BUILD_MILESTONE_TIMELINE_TESTS
            OR SUBATHON_BUILD_CONNECTOR_LIFECYCLE_TESTS)
        find_package(Qt6 6.12 REQUIRED COMPONENTS Core Test)
    endif()
    add_subdirectory(tests)
endif()
if(BUILD_TESTING AND SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Core Test LinguistTools)
    if(NOT Qt6LinguistTools_VERSION VERSION_EQUAL "6.12.0")
        message(FATAL_ERROR "Translation catalog baseline requires the selected Qt LinguistTools 6.12.0")
    endif()
    # Keep syntax/import checks separate from artifact generation below.
    add_custom_target(translation_catalog_interface_check ALL
        COMMAND "${CMAKE_COMMAND}" -E env
            "PYTHONPYCACHEPREFIX=${CMAKE_CURRENT_BINARY_DIR}/python-cache"
            "${Python3_EXECUTABLE}" -m py_compile
            "${PROJECT_SOURCE_DIR}/tools/translation_catalog.py"
            "${PROJECT_SOURCE_DIR}/tests/localization/test_translation_catalog.py"
        COMMAND "${CMAKE_COMMAND}" -E env
            "PYTHONPYCACHEPREFIX=${CMAKE_CURRENT_BINARY_DIR}/python-cache"
            "${Python3_EXECUTABLE}" -c
            "import runpy, sys; runpy.run_path(sys.argv[1], run_name='translation_catalog_interface')"
            "${PROJECT_SOURCE_DIR}/tools/translation_catalog.py"
        VERBATIM)
    set(sbt_catalog_source "${PROJECT_SOURCE_DIR}/localization/messages.json")
    set(sbt_catalog_text "${PROJECT_SOURCE_DIR}/localization/generated")
    set(sbt_catalog_build_text "${CMAKE_CURRENT_BINARY_DIR}/catalog-text")
    set(sbt_catalog_binary "${CMAKE_CURRENT_BINARY_DIR}/localization")
    set(sbt_catalog_tool "$<TARGET_FILE:Qt6::lrelease>")
    # Build in disposable outputs, then compare the tracked text without overwriting it.
    add_custom_target(translation_catalog_artifacts ALL
        COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/translation_catalog.py"
            generate --source "${sbt_catalog_source}" --text-output "${sbt_catalog_build_text}"
            --binary-output "${sbt_catalog_binary}" --lrelease "${sbt_catalog_tool}"
        COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/translation_catalog.py"
            check --source "${sbt_catalog_source}" --text-output "${sbt_catalog_text}"
            --binary-output "${sbt_catalog_binary}" --lrelease "${sbt_catalog_tool}"
        BYPRODUCTS
            "${sbt_catalog_binary}/messages_de.qm" "${sbt_catalog_binary}/messages_en.qm"
            "${sbt_catalog_build_text}/qt/messages_de.ts" "${sbt_catalog_build_text}/qt/messages_en.ts"
            "${sbt_catalog_build_text}/web/de.json" "${sbt_catalog_build_text}/web/en.json"
        COMMENT "Generate real QM catalogs and verify tracked translation text"
        VERBATIM)
    # Explicit contributor action; never list source-tree files as cleanable byproducts.
    add_custom_target(translation_catalog_update
        COMMAND "${Python3_EXECUTABLE}" -B "${PROJECT_SOURCE_DIR}/tools/translation_catalog.py"
            generate --source "${sbt_catalog_source}" --text-output "${sbt_catalog_text}"
            --binary-output "${sbt_catalog_binary}" --lrelease "${sbt_catalog_tool}"
        COMMENT "Regenerate tracked catalogs from localization/messages.json"
        VERBATIM)
endif()

if(BUILD_TESTING AND (SUBATHON_BUILD_QT_PROBE OR SUBATHON_BUILD_SQLITE_WRITER_TESTS
        OR SUBATHON_BUILD_MILESTONE_TIMELINE_TESTS OR SUBATHON_BUILD_CONNECTOR_LIFECYCLE_TESTS))
    add_subdirectory(tests/qt)
endif()
add_custom_target(check-project
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/check_project.py"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" VERBATIM)
