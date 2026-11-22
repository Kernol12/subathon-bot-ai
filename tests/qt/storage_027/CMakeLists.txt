# SPDX-License-Identifier: GPL-3.0-or-later

function(add_storage_027_stage1_suite suite timeout)
    subathon_add_qt_suite(server_storage_027_stage1_${suite}
        NAME storage-027-${suite} SOURCES ${suite}_tests.cpp
        LIBRARIES subathon_contribution_review subathon_sqlite_reference_authority
            subathon_sqlite_milestone_target_edits subathon_sqlite_modifier_activations
            subathon_refund_entitlement subathon_sqlite_account_sessions ${ARGN}
        LABELS "storage-027;acceptance;qt;server" TIMEOUT ${timeout})
endfunction()

add_storage_027_stage1_suite(catalog 180)
# Each command data row owns its disposable populated store.
add_storage_027_stage1_suite(commands 480)
add_storage_027_stage1_suite(readers 240
    subathon_reference_intake subathon_credential_storage)
