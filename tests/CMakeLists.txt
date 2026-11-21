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
if(SUBATHON_BUILD_FRANKFURTER_RATES_TESTS OR SUBATHON_BUILD_RULE_ACTIONS_TESTS OR SUBATHON_BUILD_CREDENTIAL_RECORD_TESTS OR SUBATHON_BUILD_SESSION_RESOLUTION_TESTS OR SUBATHON_BUILD_TIMER_EDITORIAL_TESTS OR SUBATHON_BUILD_PASSWORD_HASHING_TESTS OR SUBATHON_BUILD_EFFECTIVE_PERMISSIONS_TESTS OR SUBATHON_BUILD_TIME_DEBT_TESTS OR SUBATHON_BUILD_APPLICATION_PLAN_TESTS OR SUBATHON_BUILD_PAUSE_POLICY_TESTS OR SUBATHON_BUILD_RECURRING_SCHEDULE_TESTS OR SUBATHON_BUILD_TIME_CREDIT_TESTS OR SUBATHON_BUILD_EXACT_NUMERIC_TESTS OR SUBATHON_BUILD_TIMER_LIFECYCLE_TESTS OR SUBATHON_BUILD_RECEIPT_INTAKE_TESTS OR SUBATHON_BUILD_CONTRIBUTION_EVIDENCE_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/GoogleTestDependency.cmake")
    include(GoogleTest)
endif()

if(SUBATHON_BUILD_FRANKFURTER_RATES_TESTS)
    add_executable(server_frankfurter_rates_acceptance
        acceptance/frankfurter_adapter_tests.cpp
        acceptance/frankfurter_rate_service_tests.cpp)
    target_link_libraries(server_frankfurter_rates_acceptance PRIVATE
        subathon_frankfurter_rates subathon_valuation_snapshot_codec GTest::gtest_main)
    subathon_target_options(server_frankfurter_rates_acceptance)
    subathon_discover_gtests(server_frankfurter_rates_acceptance
        LABELS "frankfurter-rates;acceptance;simulation;server" TIMEOUT 10)
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

if(SUBATHON_BUILD_CREDENTIAL_RECORD_TESTS)
    add_executable(server_credential_record_acceptance
        acceptance/credential_record_tests.cpp support/credential_native_controls.cpp)
    target_link_libraries(server_credential_record_acceptance PRIVATE
        subathon_credential_record Sodium::Sodium GTest::gtest_main)
    subathon_target_options(server_credential_record_acceptance)
    foreach(symbol IN ITEMS sodium_init randombytes_buf sodium_malloc sodium_mlock sodium_free)
        target_link_options(server_credential_record_acceptance PRIVATE "LINKER:--wrap=${symbol}")
    endforeach()
    subathon_discover_gtests(server_credential_record_acceptance
        LABELS "credential-record;acceptance;server" TIMEOUT 30)
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

if(SUBATHON_BUILD_FINISHED_TIMER_TESTS)
    add_executable(core_finished_timer_acceptance acceptance/finished_timer_tests.cpp)
    target_link_libraries(core_finished_timer_acceptance PRIVATE subathon_finished_timer
        subathon_timer_editorial_commands subathon_pause_policy subathon_milestone_progression
        GTest::gtest_main)
    subathon_target_options(core_finished_timer_acceptance)
    subathon_discover_gtests(core_finished_timer_acceptance
        PREFIX "finished-timer." LABELS "finished-timer;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_DOWNTIME_RECOVERY_TESTS)
    add_executable(core_downtime_recovery_acceptance acceptance/downtime_recovery_tests.cpp)
    target_link_libraries(core_downtime_recovery_acceptance PRIVATE
        subathon_downtime_recovery GTest::gtest_main)
    subathon_target_options(core_downtime_recovery_acceptance)
    subathon_discover_gtests(core_downtime_recovery_acceptance
        PREFIX "downtime-recovery."
        LABELS "downtime-recovery;acceptance;core" TIMEOUT 30)
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

if(SUBATHON_BUILD_CHAT_COMMANDS_TESTS)
    add_executable(core_chat_commands_acceptance acceptance/chat_commands_tests.cpp)
    target_link_libraries(core_chat_commands_acceptance PRIVATE subathon_chat_commands GTest::gtest_main)
    subathon_target_options(core_chat_commands_acceptance)
    subathon_discover_gtests(core_chat_commands_acceptance
        LABELS "chat-commands;acceptance;core" TIMEOUT 30)
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

if(SUBATHON_BUILD_REFUND_ENTITLEMENT_TESTS)
    add_executable(core_refund_entitlement_acceptance acceptance/refund_entitlement_tests.cpp)
    target_link_libraries(core_refund_entitlement_acceptance PRIVATE
        subathon_refund_entitlement GTest::gtest_main)
    subathon_target_options(core_refund_entitlement_acceptance)
    subathon_discover_gtests(core_refund_entitlement_acceptance
        LABELS "refund-entitlement;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_MILESTONE_TARGET_EDIT_TESTS)
    add_executable(core_milestone_target_edit_acceptance acceptance/milestone_target_edit_tests.cpp)
    target_link_libraries(core_milestone_target_edit_acceptance PRIVATE
        subathon_milestone_target_edits GTest::gtest_main)
    subathon_target_options(core_milestone_target_edit_acceptance)
    subathon_discover_gtests(core_milestone_target_edit_acceptance
        LABELS "milestone-target-edit;acceptance;core" TIMEOUT 30)
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

if(SUBATHON_BUILD_RECURRING_SCHEDULE_TESTS)
    add_executable(core_recurring_schedule_acceptance acceptance/recurring_schedule_tests.cpp)
    target_link_libraries(core_recurring_schedule_acceptance PRIVATE subathon_recurring_schedule GTest::gtest_main)
    subathon_target_options(core_recurring_schedule_acceptance)
    subathon_discover_gtests(core_recurring_schedule_acceptance
        LABELS "recurring-schedule;acceptance;core")
endif()

if(SUBATHON_BUILD_SCHEDULED_PAUSE_TESTS)
    add_executable(server_scheduled_pause_acceptance acceptance/scheduled_pause_tests.cpp)
    target_link_libraries(server_scheduled_pause_acceptance PRIVATE
        subathon_scheduled_pause GTest::gtest_main)
    subathon_target_options(server_scheduled_pause_acceptance)
    subathon_discover_gtests(server_scheduled_pause_acceptance
        LABELS "scheduled-pause;acceptance;server" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_APPLICATION_PLAN_TESTS)
    add_executable(core_application_plan_acceptance acceptance/application_plan_tests.cpp)
    target_link_libraries(core_application_plan_acceptance PRIVATE subathon_application_plan GTest::gtest_main)
    subathon_target_options(core_application_plan_acceptance)
    subathon_discover_gtests(core_application_plan_acceptance
        LABELS "application-plan;acceptance;core")
endif()

if(SUBATHON_BUILD_MODIFIER_ACTIVATION_TESTS)
    add_executable(core_modifier_activation_acceptance acceptance/modifier_activation_tests.cpp)
    target_link_libraries(core_modifier_activation_acceptance PRIVATE
        subathon_modifier_activation subathon_application_plan GTest::gtest_main)
    subathon_target_options(core_modifier_activation_acceptance)
    subathon_discover_gtests(core_modifier_activation_acceptance
        NO_PRETTY_VALUES PREFIX "modifier-activation." TIMEOUT 30
        LABELS "modifier-activation;acceptance;core")
endif()

if(SUBATHON_BUILD_RATE_SNAPSHOT_TESTS)
    find_package(Boost 1.83 CONFIG REQUIRED)

    add_executable(core_rate_snapshot_acceptance acceptance/rate_snapshot_tests.cpp)
    target_link_libraries(core_rate_snapshot_acceptance PRIVATE subathon_rate_snapshots GTest::gtest_main)
    subathon_target_options(core_rate_snapshot_acceptance)
    subathon_discover_gtests(core_rate_snapshot_acceptance
        LABELS "rate-snapshot;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_VALUATION_SNAPSHOT_CODEC_TESTS)
    add_executable(core_valuation_snapshot_codec_acceptance acceptance/valuation_snapshot_codec_tests.cpp)
    target_link_libraries(core_valuation_snapshot_codec_acceptance PRIVATE subathon_valuation_snapshot_codec GTest::gtest_main)
    subathon_target_options(core_valuation_snapshot_codec_acceptance)
    subathon_discover_gtests(core_valuation_snapshot_codec_acceptance
        LABELS "valuation-snapshot-codec;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_RULE_DEFINITION_TESTS)
    add_executable(core_rule_definition_acceptance acceptance/rule_definition_tests.cpp)
    target_link_libraries(core_rule_definition_acceptance PRIVATE subathon_rule_definition GTest::gtest_main)
    subathon_target_options(core_rule_definition_acceptance)
    subathon_discover_gtests(core_rule_definition_acceptance
        LABELS "rule-definition;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_RULE_ACTIONS_TESTS)
    if(NOT TARGET subathon_rule_actions)
        message(FATAL_ERROR "rule-actions needs Boost headers >= 1.83")
    endif()
    add_executable(core_rule_actions_baseline acceptance/rule_actions_tests.cpp)
    target_link_libraries(core_rule_actions_baseline PRIVATE subathon_rule_actions GTest::gtest_main)
    subathon_target_options(core_rule_actions_baseline)
    subathon_discover_gtests(core_rule_actions_baseline PREFIX "rule-actions."
        LABELS "rule-actions;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_RULE_SELECTION_TESTS)
    add_executable(core_rule_selection_acceptance acceptance/rule_selection_tests.cpp)
    target_link_libraries(core_rule_selection_acceptance PRIVATE subathon_rule_selection GTest::gtest_main)
    subathon_target_options(core_rule_selection_acceptance)
    subathon_discover_gtests(core_rule_selection_acceptance
        LABELS "rule-selection;acceptance;core" TIMEOUT 30)
endif()

if(SUBATHON_BUILD_RATE_OVERRIDE_TESTS)
    add_executable(core_rate_override_acceptance acceptance/rate_override_tests.cpp)
    target_link_libraries(core_rate_override_acceptance PRIVATE subathon_rate_overrides GTest::gtest_main)
    subathon_target_options(core_rate_override_acceptance)
    subathon_discover_gtests(core_rate_override_acceptance
        LABELS "rate-override;acceptance;core" TIMEOUT 30)
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

if(SUBATHON_BUILD_REFERENCE_RULE_TESTS)
    add_executable(core_reference_rule_acceptance acceptance/reference_rule_tests.cpp)
    target_link_libraries(core_reference_rule_acceptance PRIVATE
        subathon_reference_rule GTest::gtest_main)
    subathon_target_options(core_reference_rule_acceptance)
    subathon_discover_gtests(core_reference_rule_acceptance
        PREFIX "reference-rule." LABELS "reference-rule;acceptance;core" TIMEOUT 30)
endif()

# API-011 retains every reviewed assertion and uses ordinary passing acceptance.
if(SUBATHON_BUILD_REFERENCE_INTAKE_BASELINE)
    if(NOT SUBATHON_BUILD_REFERENCE_RULE_TESTS)
        message(FATAL_ERROR "API-011 baseline requires reference-rule acceptance selection")
    endif()
    subathon_add_qt_suite(server_reference_intake_acceptance
        NAME reference-intake
        SOURCES qt/reference_intake_tests.cpp
        LIBRARIES subathon_reference_intake
        LABELS "reference-intake;acceptance;qt;server" TIMEOUT 180)
    subathon_add_qt_suite(server_reference_application_adapter_acceptance
        NAME reference-application-adapters
        SOURCES qt/reference_application_adapter_tests.cpp
        LIBRARIES subathon_reference_intake
        LABELS "reference-intake;acceptance;qt;server" TIMEOUT 180)
endif()

if(SUBATHON_BUILD_SESSION_INSTALLATION_EPOCH_TESTS)
    subathon_add_qt_suite(server_session_installation_epoch_acceptance
        NAME session-installation-epoch SOURCES qt/session_installation_epoch_tests.cpp
        LIBRARIES subathon_reference_intake
        LABELS "session-installation-epoch;acceptance;qt;server" TIMEOUT 180)
    subathon_add_qt_suite(server_session_installation_epoch_rates_acceptance
        NAME session-installation-epoch-rates SOURCES qt/session_installation_epoch_rates_tests.cpp
        LIBRARIES subathon_sqlite_rate_overrides subathon_session_resolution
        LABELS "session-installation-epoch;acceptance;qt;server" TIMEOUT 180)
endif()
