include("${PROJECT_SOURCE_DIR}/cmake/AcceptanceRegistration.cmake")

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
if(SUBATHON_BUILD_SESSION_RESOLUTION_TESTS OR SUBATHON_BUILD_TIMER_EDITORIAL_TESTS OR SUBATHON_BUILD_PASSWORD_HASHING_TESTS OR SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS OR SUBATHON_BUILD_TIME_DEBT_TESTS OR SUBATHON_BUILD_APPLICATION_PLAN_TESTS OR SUBATHON_BUILD_PAUSE_POLICY_TESTS OR SUBATHON_BUILD_TIME_CREDIT_TESTS OR SUBATHON_BUILD_EXACT_NUMERIC_TESTS OR SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS OR SUBATHON_BUILD_RECEIPT_INTAKE_TESTS OR SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/GoogleTestDependency.cmake")
    include(GoogleTest)
endif()

if(SUBATHON_BUILD_SCHEMA_VALIDATION_TESTS)
    add_subdirectory(schema_validation)
endif()

if(SUBATHON_BUILD_NATIVE_WEBSOCKET_PROFILE_TESTS)
    add_subdirectory(native_websocket_profile)
endif()

if(SUBATHON_BUILD_BOUNDED_HTTP_FOUNDATION_TESTS)
    add_subdirectory(bounded_http_foundation)
endif()

if(SUBATHON_BUILD_API_HOST_NATIVE_TESTS)
    add_subdirectory(api_host_lifecycle)
endif()

if(SUBATHON_BUILD_SESSION_RESOLUTION_TESTS)

    add_executable(server_session_resolution_acceptance acceptance/session_resolution_tests.cpp)
    target_link_libraries(server_session_resolution_acceptance PRIVATE
        subathon_session_resolution Sodium::Sodium GTest::gtest_main)
    subathon_target_options(server_session_resolution_acceptance)
    foreach(sbt_session_wrap IN ITEMS sodium_init crypto_generichash sodium_memzero sodium_memcmp)
        target_link_options(server_session_resolution_acceptance PRIVATE "LINKER:--wrap=${sbt_session_wrap}")
    endforeach()
    subathon_discover_gtests(server_session_resolution_acceptance
        LABELS "session-resolution;acceptance;server" TIMEOUT 10)
endif()

if(SUBATHON_BUILD_TIMER_EDITORIAL_TESTS)
    add_executable(core_timer_editorial_acceptance acceptance/timer_editorial_commands_tests.cpp)
    target_link_libraries(core_timer_editorial_acceptance PRIVATE
        subathon_timer_editorial_commands GTest::gtest_main)
    subathon_target_options(core_timer_editorial_acceptance)
    subathon_discover_gtests(core_timer_editorial_acceptance
        PREFIX "timer-editorial."
        LABELS "timer-editorial;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_PASSWORD_HASHING_TESTS)

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
            sodium_init sodium_malloc sodium_allocarray sodium_mlock
            sodium_munlock sodium_free crypto_pwhash_str crypto_pwhash_str_alg
            crypto_pwhash_argon2id_str crypto_pwhash_str_verify crypto_pwhash_argon2id_str_verify)
        target_link_options(server_password_hashing_acceptance PRIVATE "LINKER:--wrap=${sbt_password_wrap}")
    endforeach()
    subathon_discover_gtests(server_password_hashing_acceptance
        LABELS "password-hashing;acceptance;server" TIMEOUT 60)
endif()

if(SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS)
    # The allocation hook is armed only inside an isolated per-case evaluator call.
    add_executable(core_effective_permissions_acceptance
        acceptance/effective_permissions_tests.cpp
        acceptance/effective_permissions_regression_tests.cpp)
    target_link_libraries(core_effective_permissions_acceptance PRIVATE subathon_effective_permissions GTest::gtest_main)
    subathon_target_options(core_effective_permissions_acceptance)
    subathon_discover_gtests(core_effective_permissions_acceptance
        PREFIX "effective-permissions."
        LABELS "effective-permissions;acceptance;core" TIMEOUT 10)

    add_executable(core_delegated_permissions_acceptance
        acceptance/delegated_permissions_tests.cpp)
    target_link_libraries(core_delegated_permissions_acceptance PRIVATE
        subathon_effective_permissions GTest::gtest_main)
    subathon_target_options(core_delegated_permissions_acceptance)
    subathon_discover_gtests(core_delegated_permissions_acceptance
        PREFIX "delegated-permissions."
        LABELS "delegated-permissions;acceptance;core" TIMEOUT 60)

endif()

if(SUBATHON_BUILD_TIME_DEBT_TESTS)
    add_executable(core_time_debt_acceptance acceptance/time_debt_tests.cpp)
    target_link_libraries(core_time_debt_acceptance PRIVATE subathon_time_debt subathon_timer_lifecycle GTest::gtest_main)
    subathon_target_options(core_time_debt_acceptance)
    subathon_discover_gtests(core_time_debt_acceptance
        LABELS "time-debt;acceptance;core" TIMEOUT 5)
endif()
if(SUBATHON_BUILD_TIME_CREDIT_TESTS)
    add_executable(core_credit_acceptance
        acceptance/time_credit_tests.cpp
        acceptance/time_credit_regression_tests.cpp)
    target_link_libraries(core_credit_acceptance PRIVATE subathon_core GTest::gtest_main)
    subathon_target_options(core_credit_acceptance)
    include(GoogleTest)
    subathon_discover_gtests(core_credit_acceptance
        LABELS "acceptance;core")
endif()

if(SUBATHON_BUILD_EXACT_NUMERIC_TESTS)
    add_executable(core_exact_numeric_acceptance
        acceptance/exact_decimal_tests.cpp
        acceptance/exact_decimal_regression_tests.cpp)
    target_link_libraries(core_exact_numeric_acceptance PRIVATE subathon_exact_numeric GTest::gtest_main)
    subathon_target_options(core_exact_numeric_acceptance)
    subathon_discover_gtests(core_exact_numeric_acceptance
        LABELS "exact-numeric;acceptance;core")
endif()

if(SUBATHON_BUILD_MILESTONE_PROGRESSION_TESTS)
    add_executable(core_milestone_progression_acceptance acceptance/milestone_progression_tests.cpp)
    target_link_libraries(core_milestone_progression_acceptance PRIVATE subathon_milestone_progression GTest::gtest_main)
    subathon_target_options(core_milestone_progression_acceptance)
    subathon_discover_gtests(core_milestone_progression_acceptance
        NO_PRETTY_VALUES
        LABELS "milestone-progression;acceptance;core" TIMEOUT 30)

    find_package(Boost 1.83 CONFIG REQUIRED)
endif()

if(SUBATHON_BUILD_RECEIPT_INTAKE_TESTS)
    add_executable(server_receipt_intake_acceptance acceptance/receipt_intake_tests.cpp)
    target_link_libraries(server_receipt_intake_acceptance PRIVATE subathon_receipt_intake GTest::gtest_main)
    subathon_target_options(server_receipt_intake_acceptance)
    subathon_discover_gtests(server_receipt_intake_acceptance
        LABELS "receipt-intake;acceptance;simulation;server")
endif()

if(SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS)
    add_executable(core_timer_lifecycle_acceptance acceptance/timer_lifecycle_tests.cpp)
    target_link_libraries(core_timer_lifecycle_acceptance PRIVATE subathon_timer_lifecycle GTest::gtest_main)
    subathon_target_options(core_timer_lifecycle_acceptance)
    subathon_discover_gtests(core_timer_lifecycle_acceptance
        LABELS "timer-lifecycle;acceptance;core")
endif()

if(SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS)
    add_executable(server_contribution_evidence_acceptance acceptance/contribution_evidence_tests.cpp)
    target_link_libraries(server_contribution_evidence_acceptance PRIVATE subathon_contribution_evidence GTest::gtest_main)
    subathon_target_options(server_contribution_evidence_acceptance)
    subathon_discover_gtests(server_contribution_evidence_acceptance
        LABELS "contribution-evidence;acceptance;simulation;server")
endif()

if(SUBATHON_BUILD_PAUSE_POLICY_TESTS)
    add_executable(core_pause_policy_acceptance acceptance/pause_policy_tests.cpp)
    target_link_libraries(core_pause_policy_acceptance PRIVATE subathon_pause_policy GTest::gtest_main)
    subathon_target_options(core_pause_policy_acceptance)
    subathon_discover_gtests(core_pause_policy_acceptance
        LABELS "pause-policy;acceptance;core")
endif()

if(SUBATHON_BUILD_APPLICATION_PLAN_TESTS)
    add_executable(core_application_plan_acceptance acceptance/application_plan_tests.cpp)
    target_link_libraries(core_application_plan_acceptance PRIVATE subathon_application_plan GTest::gtest_main)
    subathon_target_options(core_application_plan_acceptance)
    subathon_discover_gtests(core_application_plan_acceptance
        LABELS "application-plan;acceptance;core")
endif()

if(SUBATHON_BUILD_RATE_SNAPSHOT_TESTS)
    find_package(Boost 1.83 CONFIG REQUIRED)

    add_executable(core_rate_snapshot_acceptance acceptance/rate_snapshot_tests.cpp)
    target_link_libraries(core_rate_snapshot_acceptance PRIVATE subathon_rate_snapshots GTest::gtest_main)
    subathon_target_options(core_rate_snapshot_acceptance)
    subathon_discover_gtests(core_rate_snapshot_acceptance
        LABELS "rate-snapshot;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_SQLITE_REFERENCE_CAPTURE_TESTS)
    # Link-check every new interface; no CTest registration or feature invocation.
    add_executable(reference_capture_interface_link scaffolding/reference_capture_interface.cpp)
    target_link_libraries(reference_capture_interface_link PRIVATE subathon_sqlite_reference_capture)
    subathon_target_options(reference_capture_interface_link)
endif()

if(SUBATHON_BUILD_SQLITE_ACCOUNT_SESSION_TESTS)
    subathon_add_qt_suite(server_sqlite_account_sessions_acceptance
        NAME sqlite-account-sessions SOURCES qt/sqlite_account_sessions_tests.cpp
        LIBRARIES subathon_sqlite_account_sessions subathon_sqlite_migrations subathon_session_resolution
        LABELS "sqlite-account-sessions;acceptance;qt;server")
endif()
