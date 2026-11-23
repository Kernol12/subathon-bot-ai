# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_milestone_006_slice4_disclosure
    NAME milestone-006-slice4-disclosure SOURCES disclosure_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006-slice4;intentional-red;acceptance;qt;server" TIMEOUT 180)
