# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_milestone_006_slice2_settlement
    NAME milestone-006-slice2-settlement SOURCES settlement_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006-slice2;acceptance;qt;server" TIMEOUT 180)

subathon_add_qt_suite(server_milestone_006_slice2_commands
    NAME milestone-006-slice2-commands SOURCES command_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement
    LABELS "milestone-006-slice2;acceptance;qt;server" TIMEOUT 180)

subathon_add_qt_suite(server_milestone_006_slice2_owner
    NAME milestone-006-slice2-owner SOURCES owner_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement
    LABELS "milestone-006-slice2;acceptance;qt;server" TIMEOUT 180)
