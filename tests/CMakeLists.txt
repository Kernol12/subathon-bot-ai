add_executable(scaffold_contract_smoke scaffolding/contract_smoke.cpp)
target_link_libraries(scaffold_contract_smoke PRIVATE subathon_core)
subathon_target_options(scaffold_contract_smoke)
add_test(NAME scaffold.contract_boundary COMMAND scaffold_contract_smoke)
set_tests_properties(scaffold.contract_boundary PROPERTIES LABELS "scaffold")
add_test(NAME scaffold.python_tools
    COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s "${PROJECT_SOURCE_DIR}/tests/tooling" -v)
set_tests_properties(scaffold.python_tools PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" LABELS "scaffold")
add_test(NAME scaffold.project_consistency
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/check_project.py")
set_tests_properties(scaffold.project_consistency PROPERTIES LABELS "scaffold")
if(SUBATHON_BUILD_SESSION_RESOLUTION_TESTS OR SUBATHON_BUILD_SCHEMA_TRANSPORT OR SUBATHON_BUILD_TIMER_EDITORIAL_TESTS OR SUBATHON_BUILD_PASSWORD_HASHING_TESTS OR SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS OR SUBATHON_BUILD_TIME_DEBT_TESTS OR SUBATHON_BUILD_APPLICATION_PLAN_TESTS OR SUBATHON_BUILD_PAUSE_POLICY_TESTS OR SUBATHON_BUILD_ACCEPTANCE_TESTS OR SUBATHON_BUILD_EXACT_NUMERIC_TESTS OR SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS OR SUBATHON_BUILD_RECEIPT_INTAKE_TESTS OR SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/GoogleTestDependency.cmake")
    include(GoogleTest)
endif()

if(SUBATHON_BUILD_SCHEMA_TRANSPORT)
    add_subdirectory(schema_validation)
endif()

if(SUBATHON_BUILD_SESSION_RESOLUTION_TESTS)
    # SDK-only foundation; building/discovery never runs this body or any selected SDK function.
    add_executable(session_resolution_environment acceptance/session_resolution_environment.cpp)
    target_link_libraries(session_resolution_environment PRIVATE Sodium::Sodium)
    subathon_target_options(session_resolution_environment)
    add_test(NAME scaffold.session_resolution_environment COMMAND session_resolution_environment)
    set_tests_properties(scaffold.session_resolution_environment PROPERTIES
        LABELS "scaffold;session-resolution-environment" TIMEOUT 10)

    add_executable(server_session_resolution_acceptance acceptance/session_resolution_tests.cpp)
    target_link_libraries(server_session_resolution_acceptance PRIVATE
        subathon_session_resolution Sodium::Sodium GTest::gtest_main)
    subathon_target_options(server_session_resolution_acceptance)
    foreach(sbt_session_wrap IN ITEMS sodium_init crypto_generichash sodium_memzero sodium_memcmp)
        target_link_options(server_session_resolution_acceptance PRIVATE "LINKER:--wrap=${sbt_session_wrap}")
    endforeach()
    gtest_discover_tests(server_session_resolution_acceptance DISCOVERY_MODE PRE_TEST
        TEST_LIST subathon_session_resolution_cases PROPERTIES TIMEOUT 10)
    # Closed ordinary-case inventory and full labels applied after passive native discovery.
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SessionResolutionLabels.cmake")
endif()

if(SUBATHON_BUILD_TIMER_EDITORIAL_TESTS)
    add_executable(core_timer_editorial_acceptance acceptance/timer_editorial_commands_tests.cpp)
    target_link_libraries(core_timer_editorial_acceptance PRIVATE
        subathon_timer_editorial_commands GTest::gtest_main)
    subathon_target_options(core_timer_editorial_acceptance)
    gtest_discover_tests(core_timer_editorial_acceptance TEST_PREFIX "timer-editorial."
        TEST_LIST subathon_timer_editorial_cases PROPERTIES TIMEOUT 30)
    # Apply complete list labels after native discovery in CTest, including older CMake.
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TimerEditorialLabels.cmake")
endif()

if(SUBATHON_BUILD_PASSWORD_HASHING_TESTS)
    # Foundation and input preparation link only the explicit SDK, never the SUT.
    add_executable(password_hashing_environment acceptance/password_hashing_environment.cpp)
    target_link_libraries(password_hashing_environment PRIVATE Sodium::Sodium)
    subathon_target_options(password_hashing_environment)
    add_test(NAME scaffold.password_hashing_environment COMMAND password_hashing_environment)
    set_tests_properties(scaffold.password_hashing_environment PROPERTIES
        LABELS "scaffold;password-hashing-environment" TIMEOUT 30)

    # Explicit preparation target: building never runs it; it is not a CTest test.
    add_executable(password_hashing_fixture_generator EXCLUDE_FROM_ALL
        scaffolding/password_hashing_fixture_generator.cpp)
    target_link_libraries(password_hashing_fixture_generator PRIVATE Sodium::Sodium)
    subathon_target_options(password_hashing_fixture_generator)

    add_executable(server_password_hashing_acceptance
        acceptance/password_hashing_tests.cpp
        support/password_hashing_failures.cpp)
    target_link_libraries(server_password_hashing_acceptance PRIVATE
        subathon_password_hashing Sodium::Sodium GTest::gtest_main)
    subathon_target_options(server_password_hashing_acceptance)
    # Private error observations/injection belong only to the feature-test executable.
    foreach(sbt_password_wrap IN ITEMS
            sodium_init sodium_malloc sodium_allocarray sodium_mlock sodium_memzero
            sodium_munlock sodium_free crypto_pwhash_str crypto_pwhash_str_alg
            crypto_pwhash_argon2id_str crypto_pwhash_str_verify crypto_pwhash_argon2id_str_verify)
        target_link_options(server_password_hashing_acceptance PRIVATE "LINKER:--wrap=${sbt_password_wrap}")
    endforeach()
    gtest_discover_tests(server_password_hashing_acceptance DISCOVERY_MODE PRE_TEST
        TEST_LIST subathon_password_cases PROPERTIES TIMEOUT 60)
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PasswordHashingLabels.cmake")
endif()

if(SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS)
    add_executable(core_effective_permissions_acceptance acceptance/effective_permissions_tests.cpp)
    target_link_libraries(core_effective_permissions_acceptance PRIVATE subathon_effective_permissions GTest::gtest_main)
    subathon_target_options(core_effective_permissions_acceptance)
    gtest_discover_tests(core_effective_permissions_acceptance TEST_PREFIX "effective-permissions."
        TEST_LIST subathon_permission_cases PROPERTIES TIMEOUT 10)
    # Private allocator fault injection is confined to this separate executable.
    add_executable(core_effective_permissions_regression
        acceptance/effective_permissions_regression_tests.cpp)
    target_link_libraries(core_effective_permissions_regression PRIVATE
        subathon_effective_permissions GTest::gtest_main)
    subathon_target_options(core_effective_permissions_regression)
    gtest_discover_tests(core_effective_permissions_regression
        TEST_PREFIX "effective-permissions-regression."
        TEST_LIST subathon_permission_regression_cases PROPERTIES TIMEOUT 10)
    # Apply the list at CTest time; older GoogleTest helpers flatten list properties.
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EffectivePermissionLabels.cmake")

    # Primitive SDK proof is independent of the permission evaluator and GTest.
    add_executable(effective_permissions_environment acceptance/effective_permissions_environment.cpp)
    target_link_libraries(effective_permissions_environment PRIVATE Boost::headers)
    subathon_target_options(effective_permissions_environment)
    add_test(NAME scaffold.effective_permissions_environment COMMAND effective_permissions_environment)
    set_tests_properties(scaffold.effective_permissions_environment PROPERTIES
        LABELS "scaffold;effective-permissions-environment" TIMEOUT 10)
endif()

if(SUBATHON_BUILD_TIME_DEBT_TESTS)
    add_executable(core_time_debt_acceptance acceptance/time_debt_tests.cpp)
    target_link_libraries(core_time_debt_acceptance PRIVATE subathon_time_debt subathon_timer_lifecycle GTest::gtest_main)
    subathon_target_options(core_time_debt_acceptance)
    gtest_discover_tests(core_time_debt_acceptance PROPERTIES
        LABELS "time-debt;acceptance;core" TIMEOUT 5)
endif()
if(SUBATHON_BUILD_ACCEPTANCE_TESTS)
    add_executable(core_credit_acceptance acceptance/time_credit_tests.cpp)
    target_link_libraries(core_credit_acceptance PRIVATE subathon_core GTest::gtest_main)
    subathon_target_options(core_credit_acceptance)
    include(GoogleTest)
    gtest_discover_tests(core_credit_acceptance PROPERTIES LABELS "acceptance;draft")
    add_executable(core_credit_regression acceptance/time_credit_regression_tests.cpp)
    target_link_libraries(core_credit_regression PRIVATE subathon_core GTest::gtest_main)
    subathon_target_options(core_credit_regression)
    gtest_discover_tests(core_credit_regression PROPERTIES LABELS "regression;core")
endif()

if(SUBATHON_BUILD_EXACT_NUMERIC_TESTS)
    add_executable(core_exact_numeric_acceptance acceptance/exact_decimal_tests.cpp)
    target_link_libraries(core_exact_numeric_acceptance PRIVATE subathon_exact_numeric GTest::gtest_main)
    subathon_target_options(core_exact_numeric_acceptance)
    gtest_discover_tests(core_exact_numeric_acceptance PROPERTIES LABELS "exact-numeric;acceptance;core")
    add_executable(core_exact_numeric_regression acceptance/exact_decimal_regression_tests.cpp)
    target_link_libraries(core_exact_numeric_regression PRIVATE subathon_exact_numeric GTest::gtest_main)
    subathon_target_options(core_exact_numeric_regression)
    gtest_discover_tests(core_exact_numeric_regression PROPERTIES LABELS "exact-numeric;regression;core")
endif()

if(SUBATHON_BUILD_MILESTONE_PROGRESSION_TESTS)
    add_executable(core_milestone_progression_acceptance acceptance/milestone_progression_tests.cpp)
    target_link_libraries(core_milestone_progression_acceptance PRIVATE subathon_milestone_progression GTest::gtest_main)
    subathon_target_options(core_milestone_progression_acceptance)
    gtest_discover_tests(core_milestone_progression_acceptance NO_PRETTY_VALUES
        TEST_LIST subathon_milestone_progression_cases PROPERTIES TIMEOUT 30)
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MilestoneProgressionLabels.cmake")

    # This independent arithmetic foundation has no project headers or libraries.
    find_package(Boost 1.83 CONFIG REQUIRED)
    add_executable(milestone_progression_environment acceptance/milestone_progression_environment.cpp)
    target_link_libraries(milestone_progression_environment PRIVATE Boost::headers)
    subathon_target_options(milestone_progression_environment)
    add_test(NAME scaffold.milestone_progression_environment COMMAND milestone_progression_environment)
    set_tests_properties(scaffold.milestone_progression_environment PROPERTIES
        LABELS "scaffold;milestone-progression-environment" TIMEOUT 15)
endif()

if(SUBATHON_BUILD_RECEIPT_INTAKE_TESTS)
    add_executable(server_receipt_intake_acceptance acceptance/receipt_intake_tests.cpp)
    target_link_libraries(server_receipt_intake_acceptance PRIVATE subathon_receipt_intake GTest::gtest_main)
    subathon_target_options(server_receipt_intake_acceptance)
    gtest_discover_tests(server_receipt_intake_acceptance PROPERTIES LABELS "receipt-intake;acceptance;simulation;server")
endif()

if(SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS)
    add_executable(core_timer_lifecycle_acceptance acceptance/timer_lifecycle_tests.cpp)
    target_link_libraries(core_timer_lifecycle_acceptance PRIVATE subathon_timer_lifecycle GTest::gtest_main)
    subathon_target_options(core_timer_lifecycle_acceptance)
    gtest_discover_tests(core_timer_lifecycle_acceptance PROPERTIES LABELS "timer-lifecycle;acceptance;core")
endif()

if(SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS)
    add_executable(server_contribution_evidence_acceptance acceptance/contribution_evidence_tests.cpp)
    target_link_libraries(server_contribution_evidence_acceptance PRIVATE subathon_contribution_evidence GTest::gtest_main)
    subathon_target_options(server_contribution_evidence_acceptance)
    gtest_discover_tests(server_contribution_evidence_acceptance PROPERTIES LABELS "contribution-evidence;acceptance;simulation;server")
endif()

if(SUBATHON_BUILD_PAUSE_POLICY_TESTS)
    add_executable(core_pause_policy_acceptance acceptance/pause_policy_tests.cpp)
    target_link_libraries(core_pause_policy_acceptance PRIVATE subathon_pause_policy GTest::gtest_main)
    subathon_target_options(core_pause_policy_acceptance)
    gtest_discover_tests(core_pause_policy_acceptance PROPERTIES LABELS "pause-policy;acceptance;core")
endif()

if(SUBATHON_BUILD_APPLICATION_PLAN_TESTS)
    add_executable(core_application_plan_acceptance acceptance/application_plan_tests.cpp)
    target_link_libraries(core_application_plan_acceptance PRIVATE subathon_application_plan GTest::gtest_main)
    subathon_target_options(core_application_plan_acceptance)
    gtest_discover_tests(core_application_plan_acceptance PROPERTIES LABELS "application-plan;acceptance;core")
endif()

if(SUBATHON_BUILD_RATE_SNAPSHOT_TESTS)
    find_package(Boost 1.83 CONFIG REQUIRED)
    # Independent selected Boost arithmetic only; never link the new rate target.
    add_executable(rate_snapshot_environment acceptance/rate_snapshot_environment.cpp)
    target_link_libraries(rate_snapshot_environment PRIVATE Boost::headers)
    subathon_target_options(rate_snapshot_environment)
    add_test(NAME scaffold.rate_snapshot_environment COMMAND rate_snapshot_environment)
    set_tests_properties(scaffold.rate_snapshot_environment PROPERTIES
        LABELS "scaffold;rate-snapshot-environment" TIMEOUT 30)

    add_executable(core_rate_snapshot_acceptance acceptance/rate_snapshot_tests.cpp)
    target_link_libraries(core_rate_snapshot_acceptance PRIVATE subathon_rate_snapshots GTest::gtest_main)
    subathon_target_options(core_rate_snapshot_acceptance)
    gtest_discover_tests(core_rate_snapshot_acceptance
        TEST_LIST subathon_rate_snapshot_cases PROPERTIES TIMEOUT 30)
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RateSnapshotLabels.cmake")
endif()

if(SUBATHON_BUILD_SQLITE_REFERENCE_CAPTURE_TESTS)
    # Link-check every new interface; no CTest registration or feature invocation.
    add_executable(reference_capture_interface_link scaffolding/reference_capture_interface.cpp)
    target_link_libraries(reference_capture_interface_link PRIVATE subathon_sqlite_reference_capture)
    subathon_target_options(reference_capture_interface_link)
endif()
