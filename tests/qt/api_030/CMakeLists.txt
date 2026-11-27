# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_api_030_stage1
    NAME api-030
    SOURCES recovery_tests.cpp
    LIBRARIES subathon_contribution_review subathon_modifier_capture
        subathon_refund_entitlement subathon_sqlite_account_sessions
    LABELS "api-030;acceptance;qt;server"
    TIMEOUT 150)
