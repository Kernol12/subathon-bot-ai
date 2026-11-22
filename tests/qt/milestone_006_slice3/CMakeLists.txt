# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_milestone_006_slice3_application
    NAME milestone-006-slice3-application SOURCES application_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006-slice3;intentional-red;acceptance;qt;server" TIMEOUT 180)

subathon_add_qt_suite(server_milestone_006_slice3_recovery
    NAME milestone-006-slice3-recovery SOURCES recovery_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006-slice3;intentional-red;acceptance;qt;server" TIMEOUT 180)

subathon_add_qt_suite(server_milestone_006_slice3_composition
    NAME milestone-006-slice3-composition SOURCES composition_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006-slice3;intentional-red;acceptance;qt;server" TIMEOUT 180)
