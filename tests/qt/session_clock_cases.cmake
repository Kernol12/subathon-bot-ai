# SPDX-License-Identifier: GPL-3.0-or-later
# Proposed literal registration; source-only until independent review/admission.
# One invocation per process: real Host fixture installs its profile once.
set(subathon_session_clock_cases
    intervals_SC01_SC04
    regression_and_cancellation_SC05_SC06
    failures_and_transient_recovery_SC07_SC08_SC11
    duration_SC09:50ms
    duration_SC09:50ms-plus-ns
    guard_SC10:EAGAIN
    guard_SC10:count3
    guard_SC10:short7
    guard_SC10:EINTR
    reestablishment_floor_SC12_SC15
    restart_SC13_SC14
    startup_stored_session_SC13_SC14:current-kernel
    startup_stored_session_SC13_SC14:no-live-status
    invalid_frames_and_source_exception
    full_domain_supplier_SC22_SC23
    signed_conversion_SC19_SC21:negative-floor
    signed_conversion_SC19_SC21:MIN
    signed_conversion_SC19_SC21:MIN-minus1
    signed_conversion_SC19_SC21:MAX
    signed_conversion_SC19_SC21:MAX-plus1
    revocation_thread_reentrancy_ex_HL05
    independent_completion_sample_SC25
    publication_refusal_SC26:outage
    publication_refusal_SC26:rollback
    abnormal_unit_has_no_publication_SC27:commit-then-throw
    abnormal_unit_has_no_publication_SC27:known-rollback
    completion_throw_revokes_scope_SC27
    denied_bindings_have_no_reads
    utc_deadline_ex_HL15:UTCmin
    utc_deadline_ex_HL15:UTCmax
)
