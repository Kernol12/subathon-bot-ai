# CORETEST-003 owner signal-policy decisions

Coordinator `/root`, 2026-10-31. Authority: specification v0.45 §4, TIMER-03/05.
Both previously open interactions now have explicit owner answers.

1. The owner answered **yes** to canceling a pending Offline delay when status becomes
   Unknown. A later confirmed Offline signal starts a fresh full configured delay
   (30 seconds by default); pending time is neither retained nor advanced through
   Unknown. An already established Offline pause remains active.
2. The owner answered **yes** to clearing the optional Unknown-timeout pause on either
   known Online or known Offline, with a fresh full configured timeout for each new
   Unknown episode. Known status ends the previous Unknown episode and resets its
   pending timeout as well. Clear only StreamStatusUnknown; retain every other
   independent pause reason, including an already established StreamOffline reason.

No policy choice remains unanswered. These answers refine the draft contract and
specification; they do not establish tests or implementation. The existing30 known
cases and partial review005b6f4 remain preserved. The newly decided interactions
still need focused examples/tests and actual full independent review, then observed
intended stub-red before CORE-003 implementation.

Coordination remains stopped after PR19 at the owner's request. Recording these
answers does not resume the task loop. No tests, stubs or implementation were changed;
no provider capability, live status observation or pause behavior is verified here.
