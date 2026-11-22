# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_milestone_006_stage1_timing
    NAME milestone-006-timing SOURCES timing_tests.cpp
    LIBRARIES subathon_milestone_timeline Qt6::Sql
    LABELS "milestone-006;intentional-red;acceptance;qt;server")

subathon_add_qt_suite(server_milestone_006_stage1_store
    NAME milestone-006-store SOURCES store_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
        subathon_sqlite_account_sessions subathon_refund_entitlement subathon_reference_intake
    LABELS "milestone-006;acceptance;qt;server" TIMEOUT 180)
