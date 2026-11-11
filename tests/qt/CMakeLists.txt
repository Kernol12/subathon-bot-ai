include("${PROJECT_SOURCE_DIR}/cmake/AcceptanceRegistration.cmake")

if(SUBATHON_BUILD_API_HOST_TESTS)
    subathon_add_qt_suite(server_api_host_acceptance
        NAME api-host SOURCES api_host_tests.cpp LIBRARIES subathon_api_host
        LABELS "api-host;acceptance;qt;server;intentional-red" TIMEOUT 3000)
    # The reviewed corrected SDK owns transitive native loading; do not inherit
    # arbitrary loader paths. Qt's selected imported targets retain their RPATHs.
    set_tests_properties(api-host.suite PROPERTIES
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
