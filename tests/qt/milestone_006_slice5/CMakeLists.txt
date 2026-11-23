# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_milestone_006_slice5_replace
    NAME milestone-006-slice5-replace SOURCES replace_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006-slice5;acceptance;qt;server" TIMEOUT 180)
