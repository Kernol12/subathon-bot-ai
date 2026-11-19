include("${PROJECT_SOURCE_DIR}/cmake/AcceptanceRegistration.cmake")

if(SUBATHON_BUILD_CONNECTOR_LIFECYCLE_TESTS)
    subathon_add_qt_suite(server_connector_lifecycle_acceptance
        NAME connector-lifecycle SOURCES connector_lifecycle_tests.cpp
        LIBRARIES subathon_connector_lifecycle
        LABELS "connector-lifecycle;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_API_HOST_TESTS)
    # Exception to the usual whole-runner registration: the frozen native
    # profile installs once per process. Preserve every individual data row.
    add_executable(server_api_host_acceptance api_host_tests.cpp)
    set_target_properties(server_api_host_acceptance PROPERTIES AUTOMOC ON)
    target_link_libraries(server_api_host_acceptance PRIVATE
        subathon_api_host Qt6::Test subathon_test_fixtures)
    subathon_target_options(server_api_host_acceptance)
    include(api_host_cases.cmake)
    foreach(host_case IN LISTS subathon_api_host_cases)
        add_test(NAME "api-host.${host_case}" COMMAND server_api_host_acceptance "${host_case}")
        # Each invocation has at most one host. The 180s process watchdog covers
        # bounded fixture waits/cleanup; it never certifies blocked work stopped.
        set_tests_properties("api-host.${host_case}" PROPERTIES
            LABELS "api-host;acceptance;qt;server;intentional-red" TIMEOUT 180
            ENVIRONMENT "LD_LIBRARY_PATH=${sbt_drogon_root}/lib")
    endforeach()

    add_executable(server_session_clock_acceptance session_clock_tests.cpp)
    set_target_properties(server_session_clock_acceptance PROPERTIES AUTOMOC ON)
    target_link_libraries(server_session_clock_acceptance PRIVATE
        subathon_session_clock subathon_session_resolution Qt6::Test subathon_test_fixtures)
    subathon_target_options(server_session_clock_acceptance)
    include(session_clock_cases.cmake)
    foreach(clock_case IN LISTS subathon_session_clock_ni_cases)
        add_test(NAME "session-clock.${clock_case}"
            COMMAND server_session_clock_acceptance "${clock_case}")
        set_tests_properties("session-clock.${clock_case}" PROPERTIES
            LABELS "session-clock;acceptance;qt;server" TIMEOUT 180
            ENVIRONMENT "LD_LIBRARY_PATH=${sbt_drogon_root}/lib")
    endforeach()
    foreach(clock_case IN LISTS subathon_session_clock_passing_cases)
        add_test(NAME "session-clock-control.${clock_case}"
            COMMAND server_session_clock_acceptance "${clock_case}")
        set_tests_properties("session-clock-control.${clock_case}" PROPERTIES
            LABELS "session-clock-control;acceptance;qt;server" TIMEOUT 180
            ENVIRONMENT "LD_LIBRARY_PATH=${sbt_drogon_root}/lib")
    endforeach()
    subathon_add_qt_suite(server_session_clock_linux_acceptance
        NAME session-clock-linux SOURCES session_clock_linux_tests.cpp
        LIBRARIES subathon_session_clock
        LABELS "session-clock-linux;acceptance;qt;server" TIMEOUT 180)
    set_tests_properties(session-clock-linux.suite PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${sbt_drogon_root}/lib")
endif()

if(SUBATHON_BUILD_SQLITE_REFERENCE_CAPTURE_TESTS)
    subathon_add_qt_suite(server_sqlite_reference_capture_acceptance
        NAME sqlite-reference-capture SOURCES sqlite_reference_capture_tests.cpp
        LIBRARIES subathon_sqlite_reference_capture
        LABELS "sqlite-reference-capture;acceptance;qt;server")
    subathon_add_qt_suite(server_reference_capture_values_acceptance
        NAME reference-capture-values SOURCES reference_capture_values_qt.cpp
        LIBRARIES subathon_reference_capture_values
        LABELS "reference-capture-values;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_APPLICATION_SERVICE_TESTS)
    subathon_add_qt_suite(server_application_service_acceptance
        NAME application-service SOURCES application_service_tests.cpp
        LIBRARIES subathon_original_application_service
        LABELS "application-service;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_MILESTONE_TIMELINE_TESTS)
    subathon_add_qt_suite(server_milestone_timeline_acceptance
        NAME milestone-timeline SOURCES milestone_timeline_tests.cpp
        LIBRARIES subathon_milestone_timeline
        LABELS "milestone-timeline;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_MILESTONE_GROUP_RESTART_TESTS)
    subathon_add_qt_suite(server_milestone_group_restart_acceptance
        NAME milestone-group-restart SOURCES milestone_group_restart_tests.cpp
        LIBRARIES subathon_sqlite_milestone_groups subathon_original_application_service
        LABELS "milestone-group-restart;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_MILESTONE_GROUP_DECISIONS_TESTS)
    subathon_add_qt_suite(server_milestone_group_decisions_acceptance
        NAME milestone-group-decisions SOURCES milestone_group_decisions_tests.cpp
        LIBRARIES subathon_sqlite_milestone_groups subathon_original_application_service
        LABELS "milestone-group-decisions;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_MODIFIER_ACTIVATIONS_TESTS)
    subathon_add_qt_suite(server_sqlite_modifier_activations_acceptance
        NAME sqlite-modifier-activations SOURCES sqlite_modifier_activations_tests.cpp
        LIBRARIES subathon_sqlite_modifier_activations subathon_original_application_service
        LABELS "sqlite-modifier-activations;acceptance;qt;server" TIMEOUT 180)
endif()

if(SUBATHON_BUILD_SQLITE_CREDENTIALS_TESTS)
    subathon_add_qt_suite(server_sqlite_credentials_acceptance
        NAME sqlite-credentials
        SOURCES sqlite_credentials_tests.cpp ../support/credential_native_controls.cpp
        LIBRARIES subathon_credential_storage subathon_sqlite_modifier_activations Sodium::Sodium
        LABELS "sqlite-credentials;acceptance;qt;server;intentional-red" TIMEOUT 120)
    # A failed cold readiness probe must never affect the main process.
    subathon_add_qt_suite(server_credential_readiness_acceptance
        NAME credential-readiness
        SOURCES credential_readiness_tests.cpp ../support/credential_native_controls.cpp
        LIBRARIES subathon_credential_storage Sodium::Sodium
        LABELS "sqlite-credentials;acceptance;qt;server;intentional-red" TIMEOUT 30)
    foreach(target IN ITEMS server_sqlite_credentials_acceptance server_credential_readiness_acceptance)
        foreach(symbol IN ITEMS sodium_init randombytes_buf sodium_malloc sodium_mlock sodium_free)
            target_link_options(${target} PRIVATE "LINKER:--wrap=${symbol}")
        endforeach()
    endforeach()
endif()

if(SUBATHON_BUILD_SQLITE_DELIVERY_JOBS_TESTS)
    subathon_add_qt_suite(server_sqlite_delivery_jobs_acceptance
        NAME sqlite-delivery-jobs SOURCES sqlite_delivery_jobs_tests.cpp
        LIBRARIES subathon_sqlite_delivery_jobs subathon_original_application_service
        LABELS "sqlite-delivery-jobs;acceptance;qt;server")
    subathon_add_qt_suite(server_sqlite_delivery_catalog_acceptance
        NAME sqlite-delivery-catalog SOURCES sqlite_delivery_catalog_tests.cpp
        LIBRARIES subathon_delivery_catalog subathon_original_application_service
        LABELS "sqlite-delivery-catalog;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_HOLD_TESTS)
    subathon_add_qt_suite(server_sqlite_application_hold_acceptance
        NAME sqlite-hold SOURCES sqlite_application_hold_tests.cpp
        LIBRARIES subathon_sqlite_application_holds
        LABELS "sqlite-hold;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_ORIGINAL_CLAIM_TESTS)
    subathon_add_qt_suite(server_sqlite_original_claim_acceptance
        NAME sqlite-original-claim SOURCES sqlite_original_claim_tests.cpp
        LIBRARIES subathon_sqlite_application_bundle
        LABELS "sqlite-original-claim;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_BUNDLE_TESTS)
    subathon_add_qt_suite(server_sqlite_application_bundle_acceptance
        NAME sqlite-bundle SOURCES sqlite_application_bundle_tests.cpp
        LIBRARIES subathon_sqlite_application_bundle
        LABELS "sqlite-bundle;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_PLAN_TESTS)
    subathon_add_qt_suite(server_sqlite_application_plans_acceptance
        NAME sqlite-plan SOURCES sqlite_application_plans_tests.cpp
        LIBRARIES subathon_sqlite_application_plans
        LABELS "sqlite-plan;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_MIGRATION_TESTS)
    subathon_add_qt_suite(server_sqlite_migration_acceptance
        NAME sqlite-migrations SOURCES sqlite_migration_tests.cpp
        LIBRARIES subathon_sqlite_migrations
        LABELS "sqlite-migrations;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_RETAINED_TESTS)
    subathon_add_qt_suite(server_sqlite_retained_context_acceptance
        NAME sqlite-retained SOURCES sqlite_retained_context_tests.cpp
        LIBRARIES subathon_sqlite_retained_context
        LABELS "sqlite-retained;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_WORK_TESTS)
    subathon_add_qt_suite(server_sqlite_work_acceptance
        NAME sqlite-work SOURCES sqlite_writer_work_tests.cpp
        LIBRARIES subathon_sqlite_writer subathon_receipt_intake
        LABELS "sqlite-work;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_WRITER_TESTS)
    subathon_add_qt_suite(server_sqlite_writer_acceptance
        NAME sqlite-writer SOURCES sqlite_writer_tests.cpp
        LIBRARIES subathon_sqlite_writer
        LABELS "sqlite-writer;acceptance;qt;server")
    subathon_add_qt_suite(server_sqlite_writer_ownership_regression
        NAME sqlite-writer-regression SOURCES sqlite_writer_ownership_tests.cpp
        LIBRARIES subathon_sqlite_writer
        LABELS "sqlite-writer;regression;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_RECEIPT_TESTS)
    subathon_add_qt_suite(server_sqlite_receipts_acceptance
        NAME sqlite-receipt SOURCES sqlite_receipts_tests.cpp
        LIBRARIES subathon_sqlite_receipts
        LABELS "sqlite-receipt;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_CONTRIBUTION_TESTS)
    subathon_add_qt_suite(server_sqlite_contributions_acceptance
        NAME sqlite-contribution SOURCES sqlite_contributions_tests.cpp
        LIBRARIES subathon_sqlite_contributions
        LABELS "sqlite-contribution;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_TRANSLATION_CATALOG_TESTS)
    add_executable(translation_catalog_probe translation_catalog_probe.cpp)
    target_link_libraries(translation_catalog_probe PRIVATE Qt6::Core)
    subathon_target_options(translation_catalog_probe)
    # Standard unittest discovery runs all methods, including their setUp/tearDown.
    add_test(NAME translation-catalog.suite
        COMMAND "${CMAKE_COMMAND}" -E env
            "SUBATHON_TRANSLATION_LRELEASE=$<TARGET_FILE:Qt6::lrelease>"
            "SUBATHON_TRANSLATION_PROBE=$<TARGET_FILE:translation_catalog_probe>"
            "${Python3_EXECUTABLE}" -m unittest discover
                -s "${PROJECT_SOURCE_DIR}/tests/localization" -p test_translation_catalog.py -v)
    set_tests_properties(translation-catalog.suite PROPERTIES
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/localization"
        LABELS "translation-catalog;acceptance" TIMEOUT 120)
endif()

if(SUBATHON_BUILD_SQLITE_PERMISSION_GRAPH_TESTS)
    subathon_add_qt_suite(server_sqlite_permission_graph_acceptance
        NAME sqlite-permission-graph SOURCES sqlite_permission_graph_tests.cpp
        LIBRARIES subathon_sqlite_permission_graph
        LABELS "sqlite-permission-graph;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_SQLITE_REFERENCE_AUTHORITY_TESTS)
    subathon_add_qt_suite(server_sqlite_reference_authority_acceptance
        NAME sqlite-reference-authority SOURCES sqlite_reference_authority_tests.cpp
        LIBRARIES subathon_sqlite_reference_authority
        LABELS "sqlite-reference-authority;acceptance;qt;server")
endif()

if(SUBATHON_BUILD_REFERENCE_RULE_TESTS)
    subathon_add_qt_suite(server_reference_rule_authority_acceptance
        NAME reference-rule-authority SOURCES reference_rule_authority_tests.cpp
        LIBRARIES subathon_reference_application
        LABELS "reference-rule;acceptance;qt;server" TIMEOUT 180)
endif()

if(SUBATHON_BUILD_DEV_SERVER_TESTS)
    if(NOT TARGET subathon_dev_server)
        message(FATAL_ERROR "Development-server acceptance requires the subathon_dev_server target")
    endif()
    subathon_add_qt_suite(server_dev_server_acceptance
        NAME dev-server SOURCES dev_server_tests.cpp
        LABELS "dev-server;acceptance;qt;server")
    target_compile_definitions(server_dev_server_acceptance PRIVATE
        SUBATHON_DEV_SERVER_PATH="$<TARGET_FILE:subathon_dev_server>")
    add_dependencies(server_dev_server_acceptance subathon_dev_server)
endif()
