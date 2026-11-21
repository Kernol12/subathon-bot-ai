# SPDX-License-Identifier: GPL-3.0-or-later

function(add_storage_026_stage1_suite suite timeout)
    subathon_add_qt_suite(server_storage_026_stage1_${suite}
        NAME storage-026-stage1-${suite} SOURCES ${suite}_tests.cpp
        LIBRARIES subathon_contribution_review subathon_sqlite_milestone_target_edits
            subathon_sqlite_modifier_activations subathon_refund_entitlement
            subathon_sqlite_account_sessions
        LABELS "storage-026;acceptance;qt;server" TIMEOUT ${timeout})
endfunction()

add_storage_026_stage1_suite(catalog 180)
add_storage_026_stage1_suite(transitions 150)
add_storage_026_stage1_suite(receipts 120)
