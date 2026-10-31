# CORETEST-003 pending signal-policy decisions

Coordinator `/root`, 2026-10-31. Specification v0.45 §4, TIMER-03/05 defines
independent30-second offline/client delays and Unknown as distinct from confirmed
Offline, with notice/default retention and an optional separate timeout pause.
Existing contracts and task briefs contain no more precise answer for these cases.

Owner decision recorded: the owner answered **yes** to “Should Unknown cancel the
pending delay, so a later confirmed Offline signal starts a fresh 30 seconds?”
Unknown therefore cancels a pending offline delay; later confirmed Offline starts
a fresh full configured delay (30 seconds by default). No pending time is retained
or advanced through Unknown. This does not clear an already active Offline reason.

One clarification remains unanswered:
An optional Unknown timeout pause is active. Clear it on either known Online or
   Offline and give each later Unknown episode a fresh timeout (recommended), or
   clear only on confirmed Online and retain it through Offline?

These questions concern pending/Unknown-owned state only. Unknown cannot clear an
already established confirmed-Offline pause or unrelated manual/schedule/recovery/
client reasons. That agreed invariant is unchanged. No recommendation is an answer;
tests for the resolved edge still need authoring/review; full contract finalization
waits for the remaining explicit owner choice.

Coordination remains stopped after PR19. Existing draft cases and partial review
are preserved; recording this answer does not resume implementation. CORETEST-003 still requires a complete actual independent
review and intended stub-red before CORE-003 implementation. No provider capability,
live status observation or pause behavior has been implemented by this record.
