cmake_minimum_required(VERSION 3.25)
project(SubathonBot VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
option(SUBATHON_BUILD_APPLICATION_PLAN_TESTS "Build captured application-plan acceptance tests" OFF)
option(SUBATHON_BUILD_ACCEPTANCE_TESTS "Build reviewed time-credit acceptance and regression tests" OFF)
option(SUBATHON_BUILD_EXACT_NUMERIC_TESTS "Build reviewed exact numeric acceptance tests" OFF)
option(SUBATHON_BUILD_TIMER_EDITORIAL_TESTS "Build timer editorial command baseline and checks" OFF)
option(SUBATHON_BUILD_PASSWORD_HASHING_TESTS "Build password hashing baseline interfaces and tests" OFF)
if(SUBATHON_BUILD_PASSWORD_HASHING_TESTS)
    include(cmake/SodiumDependency.cmake)
endif()
option(SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS "Build effective permissions baseline and environment checks" OFF)
option(SUBATHON_BUILD_PAUSE_POLICY_TESTS "Build pause policy acceptance tests" OFF)
option(SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS "Build timer lifecycle acceptance tests" OFF)
option(SUBATHON_BUILD_RECEIPT_INTAKE_TESTS "Build reviewed receipt intake simulation acceptance tests" OFF)
option(SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS "Build contribution evidence acceptance tests" OFF)
option(SUBATHON_FETCH_GOOGLETEST "Explicitly allow fetching the pinned GoogleTest source" OFF)
option(SUBATHON_BUILD_QT_PROBE "Build a Qt 6.12 Core/Test environment probe" OFF)
option(SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS "Build translation catalog generation and acceptance checks" OFF)
if(SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS)
    set(SUBATHON_BUILD_QT_PROBE ON)
endif()
option(SUBATHON_BUILD_SQLITE_WRITER_TESTS "Build Qt SQL SQLite writer acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_WORK_TESTS "Build reviewed writer transaction-work acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_MIGRATION_TESTS "Build reviewed migration catalog acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_RETAINED_TESTS "Build reviewed retained-context acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_RECEIPT_TESTS "Build reviewed identified-receipt SQLite acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_CONTRIBUTION_TESTS "Build reviewed contribution SQLite acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_PLAN_TESTS "Build reviewed captured-plan SQLite acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_BUNDLE_TESTS "Build application-bundle SQLite baseline interfaces and tests" OFF)
option(SUBATHON_BUILD_SQLITE_ORIGINAL_CLAIM_TESTS "Build typed original-claim SQLite acceptance tests" OFF)
option(SUBATHON_BUILD_SQLITE_HOLD_TESTS "Build durable application-hold SQLite acceptance tests" OFF)
option(SUBATHON_BUILD_APPLICATION_SERVICE_TESTS "Build original application-service baseline interfaces and tests" OFF)
if(SUBATHON_BUILD_APPLICATION_SERVICE_TESTS)
    set(SUBATHON_BUILD_SQLITE_HOLD_TESTS ON)
endif()
if(SUBATHON_BUILD_SQLITE_HOLD_TESTS)
    set(SUBATHON_BUILD_SQLITE_ORIGINAL_CLAIM_TESTS ON)
endif()
if(SUBATHON_BUILD_SQLITE_ORIGINAL_CLAIM_TESTS)
    set(SUBATHON_BUILD_SQLITE_BUNDLE_TESTS ON)
endif()
option(SUBATHON_BUILD_TIME_DEBT_TESTS "Build reviewed time-debt acceptance tests" OFF)
if(SUBATHON_BUILD_TIMER_EDITORIAL_TESTS)
    set(SUBATHON_BUILD_TIME_DEBT_TESTS ON)
    set(SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS ON)
endif()
set(SUBATHON_GOOGLETEST_SOURCE_DIR "" CACHE PATH "Optional local GoogleTest source checkout")
include(cmake/ProjectOptions.cmake)
if(SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS)
    # Shared scope lets the independent SDK executable use the imported headers.
    find_package(Boost 1.83 CONFIG REQUIRED)
endif()
add_subdirectory(core)
if(SUBATHON_BUILD_SQLITE_WRITER_TESTS OR SUBATHON_BUILD_SQLITE_WORK_TESTS OR SUBATHON_BUILD_SQLITE_MIGRATION_TESTS OR SUBATHON_BUILD_SQLITE_RETAINED_TESTS OR SUBATHON_BUILD_SQLITE_RECEIPT_TESTS OR SUBATHON_BUILD_SQLITE_CONTRIBUTION_TESTS OR SUBATHON_BUILD_SQLITE_PLAN_TESTS OR SUBATHON_BUILD_SQLITE_BUNDLE_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Core Sql Test)
endif()
if(SUBATHON_BUILD_PASSWORD_HASHING_TESTS OR SUBATHON_BUILD_RECEIPT_INTAKE_TESTS OR SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS OR SUBATHON_BUILD_SQLITE_WRITER_TESTS OR SUBATHON_BUILD_SQLITE_WORK_TESTS OR SUBATHON_BUILD_SQLITE_MIGRATION_TESTS OR SUBATHON_BUILD_SQLITE_RETAINED_TESTS OR SUBATHON_BUILD_SQLITE_RECEIPT_TESTS OR SUBATHON_BUILD_SQLITE_CONTRIBUTION_TESTS OR SUBATHON_BUILD_SQLITE_PLAN_TESTS OR SUBATHON_BUILD_SQLITE_BUNDLE_TESTS)
    add_subdirectory(server)
endif()
add_executable(subathon_scaffold_info scaffold/main.cpp)
subathon_target_options(subathon_scaffold_info)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
if(SUBATHON_BUILD_QT_PROBE)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Core Test)
endif()
if(SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS)
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

if(SUBATHON_BUILD_QT_PROBE OR SUBATHON_BUILD_SQLITE_WRITER_TESTS OR SUBATHON_BUILD_SQLITE_WORK_TESTS OR SUBATHON_BUILD_SQLITE_MIGRATION_TESTS OR SUBATHON_BUILD_SQLITE_RETAINED_TESTS OR SUBATHON_BUILD_SQLITE_RECEIPT_TESTS OR SUBATHON_BUILD_SQLITE_CONTRIBUTION_TESTS OR SUBATHON_BUILD_SQLITE_PLAN_TESTS OR SUBATHON_BUILD_SQLITE_BUNDLE_TESTS)
    if(NOT BUILD_TESTING)
        message(FATAL_ERROR "The Qt test lanes require BUILD_TESTING=ON")
    endif()
    add_subdirectory(tests/qt)
endif()
add_custom_target(progress
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/progress.py" render
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" VERBATIM)
add_custom_target(check-project
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/check_project.py"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" VERBATIM)
message(STATUS "Foundation scaffold only: no server, desktop app or live integrations implemented")
