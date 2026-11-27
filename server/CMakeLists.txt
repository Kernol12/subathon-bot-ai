# Framework-free services and their optional Qt SQL and network consumers.
add_library(subathon_receipt_intake STATIC src/receipt_intake.cpp)
target_include_directories(subathon_receipt_intake PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_receipt_intake PUBLIC subathon_core)
subathon_target_options(subathon_receipt_intake)

add_library(subathon_contribution_evidence STATIC src/contribution_evidence.cpp)
target_include_directories(subathon_contribution_evidence PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_contribution_evidence PUBLIC subathon_contribution)
subathon_target_options(subathon_contribution_evidence)

add_library(subathon_connector_lifecycle STATIC src/connector_lifecycle.cpp)
target_include_directories(subathon_connector_lifecycle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_connector_lifecycle PUBLIC subathon_contribution)
subathon_target_options(subathon_connector_lifecycle)

if(TARGET subathon_exact_numeric)
    add_library(subathon_streamelements_astro STATIC EXCLUDE_FROM_ALL src/streamelements_astro.cpp)
    target_include_directories(subathon_streamelements_astro PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_streamelements_astro PUBLIC subathon_core PRIVATE subathon_exact_numeric)
    subathon_target_options(subathon_streamelements_astro)
endif()

# The explicit acceptance suite requires dependencies that ordinary builds may omit.
if(BUILD_TESTING AND SUBATHON_BUILD_FRANKFURTER_RATES_TESTS)
    if(NOT TARGET subathon_rate_overrides)
        message(FATAL_ERROR "Frankfurter acceptance requires Boost >= 1.83 for the Core rate model")
    endif()
    find_package(Qt6 6.12 REQUIRED COMPONENTS Core)
endif()

if(TARGET subathon_rate_overrides AND TARGET Qt6::Core)
    add_library(subathon_frankfurter_rates STATIC EXCLUDE_FROM_ALL src/frankfurter_rates.cpp)
    target_link_libraries(subathon_frankfurter_rates PUBLIC
        subathon_connector_lifecycle subathon_rate_overrides Qt6::Core
        PRIVATE subathon_valuation_snapshot_codec)
    subathon_target_options(subathon_frankfurter_rates)
endif()

if(TARGET subathon_rate_snapshots)
    add_library(subathon_exchange_rate_api STATIC EXCLUDE_FROM_ALL src/exchange_rate_api.cpp)
    target_include_directories(subathon_exchange_rate_api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_exchange_rate_api PUBLIC subathon_rate_snapshots)
    subathon_target_options(subathon_exchange_rate_api)
endif()

if(TARGET subathon_rate_snapshots)
    add_library(subathon_open_exchange_rates STATIC EXCLUDE_FROM_ALL src/open_exchange_rates.cpp)
    target_include_directories(subathon_open_exchange_rates PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_open_exchange_rates PUBLIC subathon_rate_snapshots)
    subathon_target_options(subathon_open_exchange_rates)
endif()

add_library(subathon_scheduled_pause STATIC EXCLUDE_FROM_ALL src/scheduled_pause.cpp src/schedule_command.cpp)
target_include_directories(subathon_scheduled_pause PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(subathon_scheduled_pause PUBLIC
    subathon_recurring_schedule subathon_timer_lifecycle)
subathon_target_options(subathon_scheduled_pause)

if(TARGET subathon_modifier_stacking)
    add_library(subathon_modifier_stacking_handoff STATIC EXCLUDE_FROM_ALL src/modifier_stacking.cpp)
    target_include_directories(subathon_modifier_stacking_handoff
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${PROJECT_SOURCE_DIR}/core/src")
    target_link_libraries(subathon_modifier_stacking_handoff PUBLIC subathon_modifier_stacking)
    subathon_target_options(subathon_modifier_stacking_handoff)
endif()

if(TARGET subathon_milestone_progression)
    add_library(subathon_milestone_timeline STATIC src/milestone_timeline.cpp)
    target_include_directories(subathon_milestone_timeline PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_milestone_timeline PUBLIC subathon_milestone_progression)
    subathon_target_options(subathon_milestone_timeline)
endif()

if(BUILD_TESTING AND SUBATHON_BUILD_RULE_SIMULATOR_TESTS)
    add_library(subathon_rule_simulator STATIC EXCLUDE_FROM_ALL src/rule_simulator.cpp)
    target_include_directories(subathon_rule_simulator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_rule_simulator PUBLIC
        subathon_rule_actions subathon_application_plan subathon_modifier_stacking_handoff
        subathon_modifier_clock subathon_contribution subathon_refund_entitlement
        subathon_milestone_timeline subathon_timer_lifecycle subathon_time_debt
        subathon_pause_policy subathon_connector_lifecycle subathon_downtime_recovery
        subathon_core subathon_rule_selection subathon_milestone_progression subathon_rule_chain)
    subathon_target_options(subathon_rule_simulator)
endif()

if(TARGET Sodium::Sodium)
    add_library(subathon_session_resolution STATIC
        src/session_resolution.cpp src/session_transport.cpp src/session_policy.cpp)
    target_include_directories(subathon_session_resolution PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_session_resolution PRIVATE Sodium::Sodium)
    subathon_target_options(subathon_session_resolution)

    add_library(subathon_password_hashing STATIC src/password_hashing.cpp)
    target_include_directories(subathon_password_hashing PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_password_hashing PRIVATE Sodium::Sodium)
    subathon_target_options(subathon_password_hashing)

    add_library(subathon_credential_record STATIC src/credential_record.cpp)
    target_include_directories(subathon_credential_record PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_credential_record PUBLIC subathon_connector_lifecycle PRIVATE Sodium::Sodium)
    subathon_target_options(subathon_credential_record)
endif()

if(TARGET Qt6::Core AND TARGET Qt6::Sql)
    add_library(subathon_sqlite_v22_catalog STATIC src/sqlite_v22_catalog.cpp)
    subathon_target_options(subathon_sqlite_v22_catalog)

    add_library(subathon_sqlite_catalog_inspection STATIC src/sqlite_catalog_inspection.cpp)
    target_include_directories(subathon_sqlite_catalog_inspection PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_catalog_inspection PRIVATE Qt6::Sql)
    subathon_target_options(subathon_sqlite_catalog_inspection)

    add_library(subathon_sqlite_writer STATIC src/sqlite_writer.cpp)
    target_include_directories(subathon_sqlite_writer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_writer PUBLIC Qt6::Core Qt6::Sql
        PRIVATE subathon_sqlite_v22_catalog subathon_sqlite_catalog_inspection)
    subathon_target_options(subathon_sqlite_writer)

    if(TARGET subathon_credential_record)
        add_library(subathon_credential_storage STATIC src/credential_storage.cpp)
        target_link_libraries(subathon_credential_storage PUBLIC
            subathon_credential_record subathon_sqlite_writer PRIVATE Sodium::Sodium)
        subathon_target_options(subathon_credential_storage)
    endif()

    if(TARGET subathon_credential_storage AND TARGET subathon_rate_snapshots)
        add_library(subathon_rate_credentials STATIC src/rate_credentials.cpp)
        target_link_libraries(subathon_rate_credentials PUBLIC subathon_credential_storage
            PRIVATE Sodium::Sodium subathon_rate_snapshots)
        subathon_target_options(subathon_rate_credentials)
    endif()

    add_library(subathon_sqlite_migrations STATIC src/sqlite_migrations.cpp)
    target_include_directories(subathon_sqlite_migrations PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_migrations PUBLIC subathon_sqlite_writer Qt6::Core
        PRIVATE subathon_sqlite_v22_catalog subathon_sqlite_catalog_inspection)
    subathon_target_options(subathon_sqlite_migrations)

    add_library(subathon_sqlite_retained_context STATIC src/sqlite_retained_context.cpp)
    target_include_directories(subathon_sqlite_retained_context PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_retained_context PUBLIC subathon_sqlite_migrations Qt6::Core)
    subathon_target_options(subathon_sqlite_retained_context)

    add_library(subathon_sqlite_receipts STATIC src/sqlite_receipts.cpp)
    target_include_directories(subathon_sqlite_receipts PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_receipts PUBLIC subathon_sqlite_retained_context subathon_receipt_intake Qt6::Core)
    subathon_target_options(subathon_sqlite_receipts)

    add_library(subathon_sqlite_contributions STATIC src/sqlite_contributions.cpp)
    target_include_directories(subathon_sqlite_contributions PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_contributions PUBLIC subathon_sqlite_receipts subathon_contribution_evidence Qt6::Core)
    subathon_target_options(subathon_sqlite_contributions)
endif()

if(TARGET subathon_sqlite_contributions AND TARGET subathon_application_plan)
    add_library(subathon_sqlite_application_plans STATIC src/sqlite_application_plans.cpp)
    target_include_directories(subathon_sqlite_application_plans PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_application_plans PUBLIC subathon_sqlite_contributions subathon_application_plan Qt6::Core)
    subathon_target_options(subathon_sqlite_application_plans)

    add_library(subathon_sqlite_application_bundle STATIC src/sqlite_application_bundle.cpp)
    target_include_directories(subathon_sqlite_application_bundle PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_application_bundle PUBLIC subathon_sqlite_application_plans Qt6::Core)
    subathon_target_options(subathon_sqlite_application_bundle)

    add_library(subathon_sqlite_application_holds STATIC src/sqlite_application_holds.cpp)
    target_include_directories(subathon_sqlite_application_holds PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_application_holds PUBLIC subathon_sqlite_application_bundle Qt6::Core)
    subathon_target_options(subathon_sqlite_application_holds)

    add_library(subathon_original_application_service STATIC
        src/application_service.cpp
        src/application_service_command.cpp)
    target_include_directories(subathon_original_application_service PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_original_application_service PUBLIC subathon_sqlite_application_holds subathon_exact_numeric subathon_time_debt subathon_timer_lifecycle Qt6::Core)
    subathon_target_options(subathon_original_application_service)

    # The initial-capture baseline supplies only reviewed inert declarations/markers.
    add_library(subathon_reference_capture_values STATIC src/reference_capture_values.cpp)
    target_include_directories(subathon_reference_capture_values PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_reference_capture_values PUBLIC subathon_original_application_service)
    subathon_target_options(subathon_reference_capture_values)

    add_library(subathon_sqlite_reference_capture STATIC src/sqlite_reference_capture.cpp)
    target_include_directories(subathon_sqlite_reference_capture PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_reference_capture PUBLIC
        subathon_reference_capture_values subathon_sqlite_application_holds)
    subathon_target_options(subathon_sqlite_reference_capture)

    # Physical account/session storage reuses the compiled additive migration prefix.
    add_library(subathon_sqlite_account_sessions STATIC src/sqlite_account_sessions.cpp)
    target_include_directories(subathon_sqlite_account_sessions PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_account_sessions PUBLIC subathon_sqlite_reference_capture)
    subathon_target_options(subathon_sqlite_account_sessions)

    add_library(subathon_current_catalog STATIC src/sqlite_permission_catalog.cpp src/sqlite_reference_catalog.cpp)
    target_link_libraries(subathon_current_catalog PUBLIC subathon_sqlite_account_sessions)
    subathon_target_options(subathon_current_catalog)

    add_library(subathon_sqlite_delivery_jobs STATIC src/delivery_jobs.cpp)
    target_include_directories(subathon_sqlite_delivery_jobs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_delivery_jobs PUBLIC subathon_sqlite_application_bundle)
    subathon_target_options(subathon_sqlite_delivery_jobs)

    add_library(subathon_delivery_catalog STATIC src/sqlite_delivery_catalog.cpp)
    target_link_libraries(subathon_delivery_catalog PUBLIC subathon_current_catalog)
    subathon_target_options(subathon_delivery_catalog)

    if(TARGET subathon_milestone_timeline)
        add_library(subathon_sqlite_milestone_groups STATIC
            src/sqlite_milestone_groups.cpp src/milestone_group_decisions.cpp)
        target_include_directories(subathon_sqlite_milestone_groups PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_link_libraries(subathon_sqlite_milestone_groups PUBLIC
            subathon_milestone_timeline subathon_sqlite_delivery_jobs subathon_delivery_catalog)
        subathon_target_options(subathon_sqlite_milestone_groups)

        if(TARGET subathon_modifier_activation)
            add_library(subathon_sqlite_modifier_activations STATIC EXCLUDE_FROM_ALL
                src/sqlite_modifier_activations.cpp)
            target_link_libraries(subathon_sqlite_modifier_activations PUBLIC
                subathon_sqlite_milestone_groups subathon_modifier_activation)
            subathon_target_options(subathon_sqlite_modifier_activations)

            if(TARGET subathon_modifier_stacking_handoff)
                add_library(subathon_modifier_capture STATIC EXCLUDE_FROM_ALL
                    src/modifier_capture.cpp src/modifier_capture_codec.cpp)
                target_include_directories(subathon_modifier_capture
                    PRIVATE "${PROJECT_SOURCE_DIR}/core/src")
                target_link_libraries(subathon_modifier_capture PUBLIC
                    subathon_modifier_stacking_handoff subathon_sqlite_modifier_activations
                    subathon_sqlite_application_plans)
                subathon_target_options(subathon_modifier_capture)
            endif()

            add_library(subathon_sqlite_financial_refunds STATIC
                src/sqlite_financial_refunds.cpp
                src/financial_refund_application.cpp)
            target_link_libraries(subathon_sqlite_financial_refunds
                PUBLIC subathon_sqlite_milestone_groups subathon_original_application_service
                PRIVATE subathon_refund_entitlement subathon_sqlite_modifier_activations)
            subathon_target_options(subathon_sqlite_financial_refunds)

            add_library(subathon_sqlite_credential_catalog STATIC src/sqlite_credential_catalog.cpp)
            target_link_libraries(subathon_sqlite_credential_catalog PUBLIC subathon_sqlite_financial_refunds)
            subathon_target_options(subathon_sqlite_credential_catalog)
        endif()
    endif()

    # Authoritative graph storage reuses the account prefix and pure permission validation.
    add_library(subathon_sqlite_permission_graph STATIC src/sqlite_permission_graph.cpp)
    target_include_directories(subathon_sqlite_permission_graph PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_permission_graph PUBLIC subathon_sqlite_account_sessions subathon_effective_permissions subathon_current_catalog)
    subathon_target_options(subathon_sqlite_permission_graph)
endif()

if(TARGET subathon_sqlite_credential_catalog AND TARGET subathon_sqlite_permission_graph
        AND TARGET subathon_valuation_snapshot_codec AND TARGET subathon_rate_overrides)
    add_library(subathon_sqlite_rate_overrides STATIC EXCLUDE_FROM_ALL src/sqlite_rate_overrides.cpp)
    target_link_libraries(subathon_sqlite_rate_overrides PUBLIC
        subathon_sqlite_credential_catalog subathon_sqlite_permission_graph
        subathon_valuation_snapshot_codec subathon_rate_overrides)
    subathon_target_options(subathon_sqlite_rate_overrides)

    add_library(subathon_sqlite_milestone_target_edits STATIC EXCLUDE_FROM_ALL
        src/sqlite_milestone_target_edits.cpp)
    target_link_libraries(subathon_sqlite_milestone_target_edits
        PUBLIC subathon_sqlite_rate_overrides subathon_milestone_target_edits
            subathon_sqlite_modifier_activations)
    subathon_target_options(subathon_sqlite_milestone_target_edits)

    add_library(subathon_contribution_review STATIC EXCLUDE_FROM_ALL
        src/connector_intake_start.cpp
        src/connector_history_profile.cpp
        src/contribution_review.cpp
        src/sqlite_milestone_interruption.cpp
        src/sqlite_milestone_playback.cpp
        src/sqlite_connector_intake.cpp
        src/sqlite_connector_intake_read.cpp
        src/sqlite_connector_intake_history.cpp
        src/sqlite_connector_intake_page.cpp
        src/sqlite_connector_intake_receipt.cpp)
    target_link_libraries(subathon_contribution_review PUBLIC subathon_sqlite_milestone_target_edits)
    subathon_target_options(subathon_contribution_review)
endif()

# Current reference authority and finite selected historical reads.
if(TARGET subathon_sqlite_permission_graph AND TARGET subathon_session_resolution)
    add_library(subathon_sqlite_reference_authority STATIC src/sqlite_reference_authority.cpp)
    target_include_directories(subathon_sqlite_reference_authority PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_sqlite_reference_authority PUBLIC
        subathon_sqlite_permission_graph subathon_current_catalog subathon_session_resolution
        subathon_sqlite_reference_capture subathon_sqlite_application_bundle
        subathon_sqlite_application_holds subathon_original_application_service)
    subathon_target_options(subathon_sqlite_reference_authority)

    add_library(subathon_reference_application STATIC src/reference_application.cpp)
    target_include_directories(subathon_reference_application PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_reference_application PUBLIC
        subathon_reference_rule subathon_sqlite_reference_authority)
    subathon_target_options(subathon_reference_application)

    add_library(subathon_reference_intake STATIC
        src/reference_configuration.cpp src/reference_runtime.cpp src/reference_intake.cpp)
    target_link_libraries(subathon_reference_intake PUBLIC subathon_reference_application)
    subathon_target_options(subathon_reference_intake)
endif()

if(TARGET Drogon::Drogon)
    set(sbt_native_source "${CMAKE_CURRENT_SOURCE_DIR}/src/native_websocket_profile.cpp")
    if(sbt_bounded_http_successor)
        # The SDK seal owns this build copy; never compile it alongside the original.
        set(sbt_native_source "${sbt_drogon_root}/share/subathon-bounded-http-foundation/native_websocket_profile.cpp")
        file(READ "${sbt_profile_manifest}" sbt_successor_manifest)
        string(JSON sbt_native_sha GET "${sbt_successor_manifest}" first_party derived_sha256)
        file(SHA256 "${sbt_native_source}" sbt_actual_native_sha)
        if(NOT sbt_native_sha STREQUAL sbt_actual_native_sha)
            message(FATAL_ERROR "Successor native build-copy digest mismatch")
        endif()
    endif()
    add_library(subathon_native_websocket_profile STATIC "${sbt_native_source}")
    target_include_directories(subathon_native_websocket_profile PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_native_websocket_profile PRIVATE Drogon::Drogon)
    subathon_target_options(subathon_native_websocket_profile)
endif()

if(TARGET Drogon::Drogon AND TARGET Sodium::Sodium AND sbt_bounded_http_successor)
    # Successor component only: never link these definitions into the frozen NI target.
    # Shared pure preflight also guards the successor SDK adapter.
    add_library(subathon_bounded_http_storage OBJECT src/bounded_http_storage.cpp)
    target_include_directories(subathon_bounded_http_storage PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    subathon_target_options(subathon_bounded_http_storage)

    # Successor producer and SDK interfaces are reviewed together in the tier A cycle.
    add_library(subathon_bounded_http_development STATIC src/bounded_http_sdk_adapter.cpp
        src/account_capture_observer.cpp
        $<TARGET_OBJECTS:subathon_bounded_http_storage>)
    set(sbt_account_seal "${sbt_drogon_root}/dependency-build.json")
    set(sbt_account_profile "${sbt_drogon_root}/share/subathon-bounded-http-foundation/manifest.json")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${sbt_account_seal}" "${sbt_account_profile}")
    file(SHA256 "${sbt_account_seal}" ACCOUNT_CAPTURE_ARTIFACT_SHA256)
    file(SHA256 "${sbt_account_profile}" ACCOUNT_CAPTURE_PROFILE_SHA256)
    file(READ "${sbt_account_seal}" sbt_account_seal_json)
    string(JSON sbt_account_seal_profile GET "${sbt_account_seal_json}" profile_manifest_sha256)
    if(NOT sbt_account_seal_profile STREQUAL ACCOUNT_CAPTURE_PROFILE_SHA256)
        message(FATAL_ERROR "Selected account producer seal/profile disagreement")
    endif()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/account_capture_identity.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/account-capture-producer-private/account_capture_identity.hpp" @ONLY)
    target_include_directories(subathon_bounded_http_development PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/account-capture-producer-private")
    target_link_libraries(subathon_bounded_http_development PRIVATE Drogon::Drogon Sodium::Sodium)
    target_include_directories(subathon_bounded_http_development PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")
    subathon_target_options(subathon_bounded_http_development)
endif()

# APITEST-001 contains explicit unimplemented production pieces. The Qt baseline
# uses fake transport; the native child links the separate shipped Drogon adapter.
if(TARGET subathon_current_catalog AND TARGET subathon_native_websocket_profile)
    add_library(subathon_api_host STATIC
        src/api_host.cpp src/api_host_signal_linux.cpp src/api_host_elapsed_linux.cpp)
    target_include_directories(subathon_api_host PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_link_libraries(subathon_api_host PUBLIC
        subathon_sqlite_account_sessions subathon_native_websocket_profile Qt6::Core Threads::Threads
        PRIVATE subathon_current_catalog)
    subathon_target_options(subathon_api_host)
    add_library(subathon_session_clock STATIC src/session_clock.cpp src/session_clock_host.cpp)
    target_link_libraries(subathon_session_clock PUBLIC subathon_api_host)
    subathon_target_options(subathon_session_clock)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET subathon_sqlite_permission_graph
        AND TARGET subathon_session_resolution)
    find_package(Threads REQUIRED)
    add_library(subathon_diagnostics STATIC
        src/diagnostics_authority.cpp src/diagnostics_export.cpp
        src/diagnostics_json.cpp src/diagnostics_log.cpp)
    target_link_libraries(subathon_diagnostics PUBLIC
        subathon_connector_lifecycle subathon_sqlite_receipts subathon_sqlite_permission_graph
        subathon_session_resolution PRIVATE Threads::Threads)
    target_compile_definitions(subathon_diagnostics PRIVATE
        SUBATHON_DIAGNOSTICS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        SUBATHON_DIAGNOSTICS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        SUBATHON_DIAGNOSTICS_VERSION_PATCH=${PROJECT_VERSION_PATCH})
    subathon_target_options(subathon_diagnostics)
endif()

if(TARGET subathon_api_host AND TARGET subathon_bounded_http_development)
    add_library(subathon_api_host_drogon STATIC src/api_host_drogon.cpp)
    target_link_libraries(subathon_api_host_drogon PUBLIC subathon_api_host PRIVATE Drogon::Drogon subathon_bounded_http_development)
    target_compile_definitions(subathon_api_host_drogon PRIVATE QT_NO_KEYWORDS)
    subathon_target_options(subathon_api_host_drogon)
endif()

if(TARGET subathon_reference_intake AND TARGET subathon_password_hashing)
    add_executable(subathon_dev_server dev/main.cpp dev/development_setup.cpp)
    target_link_libraries(subathon_dev_server PRIVATE subathon_reference_intake subathon_password_hashing)
    set_target_properties(subathon_dev_server PROPERTIES OUTPUT_NAME subathon-dev-server)
    subathon_target_options(subathon_dev_server)
endif()
