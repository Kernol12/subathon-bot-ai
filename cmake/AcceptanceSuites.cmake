# SPDX-License-Identifier: GPL-3.0-or-later
include_guard(GLOBAL)

set(subathon_acceptance_suites
    time-credit exact-numeric timer-lifecycle receipt-intake contribution-evidence
    pause-policy application-plan sqlite-writer time-debt sqlite-work
    sqlite-migrations sqlite-retained sqlite-receipt sqlite-contribution sqlite-plan
    sqlite-bundle sqlite-original-claim sqlite-hold application-service
    translation-catalog rate-snapshot milestone-progression session-resolution
    password-hashing effective-permissions timer-editorial sqlite-reference-capture
    sqlite-account-sessions native-websocket-profile api-host api-host-native)

# Existing library/test guards stay private derived variables; callers select the
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
# suite closures are the existing preset contract in TESTCLEAN-003's proposal.
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

    # SQLite repositories reuse the preceding compiled migration factories;
    # account/session storage therefore includes the initial-capture prefix.
    # Other selected suites retain their small closures.
    set(sqlite_chain sqlite-writer sqlite-work sqlite-migrations sqlite-retained
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
        list(FIND sqlite_chain "${suite}" sqlite_index)
        if(suite STREQUAL "api-host" OR suite STREQUAL "api-host-native")
            # Host headers/fixtures need the real store prefix and corrected
            # transport even in the Qt/fake lane. Native retains the Qt baseline.
            list(APPEND selected ${sqlite_chain} native-websocket-profile api-host "${suite}")
        elseif(NOT sqlite_index EQUAL -1)
            math(EXPR prefix_length "${sqlite_index} + 1")
            list(SUBLIST sqlite_chain 0 ${prefix_length} prefix)
            list(APPEND selected ${prefix})
        elseif(suite STREQUAL "milestone-progression" OR suite STREQUAL "effective-permissions"
               OR suite STREQUAL "rate-snapshot")
            list(APPEND selected exact-numeric "${suite}")
        elseif(suite STREQUAL "timer-editorial")
            list(APPEND selected time-debt timer-lifecycle timer-editorial)
        else()
            list(APPEND selected "${suite}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES selected)
    list(SORT selected)
    set(${output} "${selected}" PARENT_SCOPE)
endfunction()
