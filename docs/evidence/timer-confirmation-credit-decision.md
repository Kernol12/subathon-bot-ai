# Timer confirmation-wait credit — owner clarification

Recorded by coordinator `/root` on2026-10-31, task CORETEST-002/CORE-002,
branch `task/CORETEST-002-timer-baseline`, specification v0.45 §4.

The specification named the finish-confirmation end mode but did not specify
positive accepted time during its zero-time wait. The coordinator asked the owner
whether that credit should resume main automatically or remain pending. The owner
explicitly selected: “Resume the main countdown automatically — recommended”.

Accepted policy: while Active/Main is awaiting finish confirmation at zero,
positive actual available time clears the wait and resumes Main automatically.
Eligibility, input review, valuation, cap and debt handling remain caller duties;
only positive actual main time rescues, so zero actual credit keeps waiting.
Retain the grace budget and every independent pause reason. The caller observes
and enforces hard end before the command; ordinary credit cannot reopen Finished.
A later stale finish-confirmation proposal must fail its existing waiting-state
precondition after resumption.

This resolves the recorded product-policy blocker. It changes no first-start/Ready,
reopening, refund, permission or downtime policy; those remain their explicit tasks.
The new §4 bullet/§18 example clarify this edge within the existing v0.45 scope.
No timer implementation or passing behavior is claimed by the owner's decision.
Add specification-linked cases, obtain separate actual test review, then record
intended stub-red before CORE-002 production implementation.
