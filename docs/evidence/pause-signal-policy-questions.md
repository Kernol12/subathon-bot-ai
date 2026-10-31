# CORETEST-003 pending signal-policy decisions

Coordinator `/root`, 2026-10-31. Specification v0.45 §4, TIMER-03/05 defines
independent30-second offline/client delays and Unknown as distinct from confirmed
Offline, with notice/default retention and an optional separate timeout pause.
Existing contracts and task briefs contain no more precise answer for these cases.

Owner clarification was requested asynchronously; no response is assumed:

1. Unknown arrives while a confirmed-Offline delay is pending. Cancel pending delay
   and start fresh on a later confirmed Offline (recommended), freeze the remaining
   delay until Offline is confirmed again, or continue through Unknown?
2. An optional Unknown timeout pause is active. Clear it on either known Online or
   Offline and give each later Unknown episode a fresh timeout (recommended), or
   clear only on confirmed Online and retain it through Offline?

These questions concern pending/Unknown-owned state only. Unknown cannot clear an
already established confirmed-Offline pause or unrelated manual/schedule/recovery/
client reasons. That agreed invariant is unchanged. No recommendation is an answer;
dependent tests/contract finalization wait for an explicit owner choice.

Author `/root/pause_test_author` continues independently specified draft cases in
its isolated worktree. CORETEST-003 still requires a complete actual independent
review and intended stub-red before CORE-003 implementation. No provider capability,
live status observation or pause behavior has been implemented by this record.
