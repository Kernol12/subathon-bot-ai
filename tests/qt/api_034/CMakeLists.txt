# SPDX-License-Identifier: GPL-3.0-or-later

subathon_add_qt_suite(server_api_034_stage1
    NAME api-034
    SOURCES enrollment_tests.cpp
    LIBRARIES subathon_contribution_review subathon_sqlite_milestone_target_edits
        subathon_sqlite_modifier_activations subathon_refund_entitlement
        subathon_sqlite_account_sessions subathon_sqlite_permission_graph
        subathon_session_resolution subathon_connector_lifecycle
    LABELS "api-034;intentional-red;server;qt" TIMEOUT 240)
