# SPDX-License-Identifier: GPL-3.0-or-later
include_guard(GLOBAL)

set(subathon_acceptance_suites
    time-credit exact-numeric refund-entitlement timer-lifecycle receipt-intake contribution-evidence
    pause-policy recurring-schedule application-plan modifier-activation modifier-stacking modifier-clock sqlite-writer time-debt sqlite-work
    sqlite-migrations sqlite-retained sqlite-receipt sqlite-contribution sqlite-plan
    sqlite-bundle sqlite-original-claim sqlite-hold application-service sqlite-delivery-jobs
    translation-catalog rate-snapshot rate-override valuation-snapshot-codec rule-definition rule-selection milestone-progression milestone-timeline milestone-group-restart
    milestone-group-decisions sqlite-modifier-activations sqlite-financial-refunds sqlite-credential-catalog sqlite-rate-override-catalog sqlite-rate-override-writer
    contribution-review modifier-capture connector-lifecycle session-resolution session-installation-epoch
    password-hashing credential-record sqlite-credentials rate-credentials effective-permissions timer-editorial sqlite-reference-capture
    sqlite-account-sessions native-websocket-profile api-host api-host-native
    bounded-http-foundation account-capture sqlite-permission-graph sqlite-reference-authority reference-rule dev-server
    chat-commands scheduled-pause milestone-target-edit sqlite-milestone-target-edits downtime-recovery finished-timer rule-actions rule-chain rule-simulator
    frankfurter-rates exchange-rate-api open-exchange-rates)

# Test guards stay private derived variables; callers select the
# logical suites above instead of maintaining another set of cache switches.
function(_subathon_acceptance_flag suite output)
    string(REPLACE "-" "_" suffix "${suite}")
    string(TOUPPER "${suffix}" suffix)
    if(suite STREQUAL "sqlite-migrations")
        set(suffix SQLITE_MIGRATION)
    elseif(suite STREQUAL "sqlite-account-sessions")
        set(suffix SQLITE_ACCOUNT_SESSION)
    endif()
    set(${output} "SUBATHON_BUILD_${suffix}_TESTS" PARENT_SCOPE)
endfunction()

# Resolve the real acceptance selection used by target creation. Read the common
# SUBATHON_BUILD_ACCEPTANCE_TESTS gate and optional SUBATHON_ACCEPTANCE_SUITES list;
# return sorted, unique canonical suite names through output. OFF selects none;
# bare ON selects time-credit. Explicit empty/unknown selections and any enabled
# retired per-feature cache option fail with an actionable diagnostic. The named
# suite closures preserve test registration: SQLite presets run the earlier
# repository suites, and host profiles also select the native transport baseline.
function(subathon_resolve_acceptance_suites output)
    # Refuse enabled retired requests before applying any new/default selection.
    # Cache provenance cannot reliably distinguish an old default from explicit -D.
    foreach(suite IN LISTS subathon_acceptance_suites)
        _subathon_acceptance_flag("${suite}" flag)
        if(DEFINED CACHE{${flag}})
            get_property(legacy_value CACHE "${flag}" PROPERTY VALUE)
            if(legacy_value)
                message(FATAL_ERROR
                    "Retired acceptance option ${flag} is enabled. Use cmake --fresh --preset <name> "
                    "and SUBATHON_ACCEPTANCE_SUITES instead of per-feature cache switches.")
            endif()
            unset(${flag} CACHE)
        endif()
    endforeach()

    if(NOT SUBATHON_BUILD_ACCEPTANCE_TESTS)
        set(${output} "" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED SUBATHON_ACCEPTANCE_SUITES)
        set(requested "${SUBATHON_ACCEPTANCE_SUITES}")
    else()
        set(requested time-credit)
    endif()
    if(requested STREQUAL "")
        message(FATAL_ERROR "Acceptance suite selection must not be empty")
    endif()

    # Keep the earlier SQLite test suites in each existing preset's selection.
    set(sqlite_test_prefix sqlite-writer sqlite-work sqlite-migrations sqlite-retained
        sqlite-receipt sqlite-contribution sqlite-plan sqlite-bundle
        sqlite-original-claim sqlite-hold application-service sqlite-reference-capture
        sqlite-account-sessions)
    set(selected)
    foreach(suite IN LISTS requested)
        if(suite STREQUAL "all")
            list(APPEND selected ${subathon_acceptance_suites})
            continue()
        endif()
        if(NOT suite IN_LIST subathon_acceptance_suites)
            message(FATAL_ERROR "Unknown acceptance suite: ${suite}")
        endif()
        list(FIND sqlite_test_prefix "${suite}" sqlite_index)
        if(suite STREQUAL "api-host" OR suite STREQUAL "api-host-native")
            # Native retains the Qt host baseline's registrations.
            list(APPEND selected ${sqlite_test_prefix} native-websocket-profile api-host "${suite}")
        elseif(suite STREQUAL "session-installation-epoch")
            list(APPEND selected ${sqlite_test_prefix} exact-numeric effective-permissions
                rate-snapshot rate-override valuation-snapshot-codec "${suite}")
        elseif(suite STREQUAL "reference-rule" OR suite STREQUAL "dev-server")
            list(APPEND selected ${sqlite_test_prefix} exact-numeric effective-permissions sqlite-permission-graph sqlite-reference-authority reference-rule "${suite}")
        elseif(suite STREQUAL "sqlite-reference-authority")
            list(APPEND selected ${sqlite_test_prefix} exact-numeric effective-permissions sqlite-permission-graph sqlite-reference-authority)
        elseif(suite STREQUAL "sqlite-permission-graph")
            # Separate child; existing host/account selections keep their closure.
            list(APPEND selected ${sqlite_test_prefix} exact-numeric effective-permissions sqlite-permission-graph)
        elseif(suite STREQUAL "bounded-http-foundation" OR suite STREQUAL "account-capture")
            # Account capture selects the existing foundation and native profiles.
            list(APPEND selected native-websocket-profile bounded-http-foundation "${suite}")
        elseif(suite STREQUAL "sqlite-delivery-jobs")
            list(APPEND selected ${sqlite_test_prefix} sqlite-delivery-jobs)
        elseif(suite STREQUAL "milestone-group-restart" OR suite STREQUAL "milestone-group-decisions"
               OR suite STREQUAL "sqlite-modifier-activations" OR suite STREQUAL "sqlite-financial-refunds"
               OR suite STREQUAL "sqlite-credentials")
            list(APPEND selected ${sqlite_test_prefix} sqlite-delivery-jobs
                exact-numeric milestone-progression milestone-timeline milestone-group-restart "${suite}")
            if(suite STREQUAL "sqlite-modifier-activations" OR suite STREQUAL "sqlite-financial-refunds"
               OR suite STREQUAL "sqlite-credentials")
                list(APPEND selected milestone-group-decisions)
            endif()
            if(suite STREQUAL "sqlite-credentials")
                list(APPEND selected sqlite-modifier-activations)
            endif()
        elseif(suite STREQUAL "sqlite-credential-catalog" OR suite STREQUAL "sqlite-rate-override-catalog"
               OR suite STREQUAL "sqlite-rate-override-writer")
            list(APPEND selected ${sqlite_test_prefix} sqlite-delivery-jobs
                exact-numeric milestone-progression milestone-timeline milestone-group-restart
                milestone-group-decisions sqlite-modifier-activations sqlite-financial-refunds
                sqlite-credential-catalog)
            if(NOT suite STREQUAL "sqlite-credential-catalog")
                list(APPEND selected effective-permissions sqlite-permission-graph
                    rate-snapshot rate-override valuation-snapshot-codec "${suite}")
            endif()
        elseif(suite STREQUAL "sqlite-milestone-target-edits")
            list(APPEND selected ${sqlite_test_prefix} sqlite-delivery-jobs
                exact-numeric milestone-progression milestone-timeline milestone-group-restart
                milestone-group-decisions sqlite-modifier-activations sqlite-financial-refunds
                milestone-target-edit "${suite}")
        elseif(suite STREQUAL "milestone-target-edit")
            list(APPEND selected exact-numeric milestone-progression "${suite}")
        elseif(suite STREQUAL "contribution-review" OR suite STREQUAL "modifier-capture")
            list(APPEND selected ${sqlite_test_prefix} sqlite-delivery-jobs
                exact-numeric effective-permissions sqlite-permission-graph
                rate-snapshot rate-override valuation-snapshot-codec milestone-progression
                milestone-timeline milestone-group-restart milestone-group-decisions
                sqlite-modifier-activations sqlite-financial-refunds sqlite-credential-catalog
                sqlite-rate-override-catalog sqlite-rate-override-writer milestone-target-edit
                sqlite-milestone-target-edits contribution-review "${suite}")
        elseif(NOT sqlite_index EQUAL -1)
            math(EXPR prefix_length "${sqlite_index} + 1")
            list(SUBLIST sqlite_test_prefix 0 ${prefix_length} prefix)
            list(APPEND selected ${prefix})
        elseif(suite STREQUAL "milestone-timeline")
            list(APPEND selected exact-numeric milestone-progression milestone-timeline)
        elseif(suite STREQUAL "modifier-clock")
            list(APPEND selected exact-numeric modifier-clock)
        elseif(suite STREQUAL "modifier-activation")
            list(APPEND selected exact-numeric application-plan modifier-activation)
        elseif(suite STREQUAL "rule-selection")
            list(APPEND selected exact-numeric rate-snapshot rule-definition "${suite}")
        elseif(suite STREQUAL "rate-override" OR suite STREQUAL "valuation-snapshot-codec"
               OR suite STREQUAL "rule-definition")
            list(APPEND selected exact-numeric rate-snapshot "${suite}")
        elseif(suite STREQUAL "milestone-progression" OR suite STREQUAL "effective-permissions"
               OR suite STREQUAL "rate-snapshot" OR suite STREQUAL "refund-entitlement")
            list(APPEND selected exact-numeric "${suite}")
        elseif(suite STREQUAL "chat-commands")
            list(APPEND selected exact-numeric effective-permissions "${suite}")
        elseif(suite STREQUAL "finished-timer")
            list(APPEND selected exact-numeric finished-timer)
        elseif(suite STREQUAL "timer-editorial")
            list(APPEND selected time-debt timer-lifecycle timer-editorial)
        elseif(suite STREQUAL "downtime-recovery")
            list(APPEND selected pause-policy recurring-schedule time-debt timer-lifecycle timer-editorial downtime-recovery)
        elseif(suite STREQUAL "scheduled-pause")
            list(APPEND selected recurring-schedule timer-lifecycle scheduled-pause)
        else()
            list(APPEND selected "${suite}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES selected)
    list(SORT selected)
    set(${output} "${selected}" PARENT_SCOPE)
endfunction()
