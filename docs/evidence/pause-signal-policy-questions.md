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
   unrecovered pause reason. Known Offline retains established StreamOffline; known
   Online independently clears StreamOffline under the existing recovery rule.

No policy choice remains unanswered. These answers refine the draft contract and
specification; they do not establish tests or implementation. The existing30 known
cases and partial review005b6f4 remain preserved. The newly decided interactions
still need focused examples/tests and actual full independent review, then observed
intended stub-red before CORE-003 implementation.

The subsequent explicit coordinator-resume instruction restarts the task loop.
The decision records themselves changed no tests, stubs or implementation;
no provider capability, live status observation or pause behavior is verified here.

Recovery-rule composition: clearing StreamStatusUnknown is not a global Resume.
The previously agreed Online recovery still clears StreamOffline, and a connected
client still clears its own disconnect reason. Each rule changes only its reason;
Manual/Schedule/RecoveryReview and unrecovered signals remain. This clarifies the
earlier broad retention wording without changing either recorded owner choice or
the specification's existing automatic recovery requirement.
