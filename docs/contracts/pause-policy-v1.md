# Independent pause reasons and delayed signals — proposed revision 1

Tasks: CORETEST-003 / CORE-003. Authority: specification v0.45
[§4](../specification.md#4-main-timer-grace-pauses-and-finishing), TIMER-03/05,
§18 overlapping pause/disconnect/unknown scenarios, and
[event-state boundaries revision 1](event-state-boundaries-v1.md).
The [timer lifecycle contract](timer-lifecycle-v1.md) remains unchanged.

State: proposed interface and explicit unconditional `PausePolicyNotImplemented`
stubs only. Tests are being authored. No independent approval, observed red or
production pause behavior is claimed. The policy questions below prevent finalizing
the affected branches; unrelated specification-defined cases can be reviewed first.

## Scope and public values

`core/include/subathon/core/pause_policy.hpp` provides two pure proposal functions:

- `set_pause_reason(state, reason, active)` changes exactly one externally managed
  Manual, Schedule or RecoveryReview reason. Repeated set/clear is idempotent.
- `observe_pause_signals(state, config, observation)` observes constant normalized
  stream/client signals for one explicit nonnegative monotonic duration. It owns
  StreamOffline, DesignatedClientDisconnected and StreamStatusUnknown reasons.

`PauseState.reasons` is the independent set; an empty set means no pause imposed
by this policy. A pending delay is separate from an active reason. Its optional
elapsed value is present only before that delay has expired, including zero at the
start. An active reason and its own pending delay cannot coexist. Once activated,
the pending value is absent, so long active episodes need no unbounded accumulator.
The unknown notice is independent of the pause set and does not claim an offline
signal. The initial empty snapshot describes no observations yet, not verified
online/client connectivity. Both normalized signals are supplied explicitly per call.

Offline and designated-client delays default independently to 30000ms. Either may
be any nonnegative signed64 duration, including zero. Unknown timeout defaults to
disabled (`nullopt`); an enabled timeout is a nonnegative signed64 duration. These
layouts, duration units and validation errors are bounded local engineering choices;
they introduce no persisted schema or user-visible policy beyond the specification.

## Explicit observation ordering and independent reasons

A supplied observation represents one interval during which its stream status and
client connection condition are unchanged. The caller splits at signal changes,
using an elapsed-zero observation at the change boundary when needed. Stream and
client pending durations advance independently; they use monotonic elapsed time even
while another reason already pauses the countdown. No UTC value or system clock is
read. Process-local observations are not portable restart checkpoints.

Before expiry, the pending condition adds no pause. At exactly its configured delay,
or later, its reason is present and its pending field is absent. Repeated unchanged
signals do not restart a pending delay or accumulate duplicate reasons. A confirmed
Online stream clears only StreamOffline; a connected designated client clears only
DesignatedClientDisconnected. Recovery before a pending delay expires cancels that
pending delay; a subsequent disconnect requires a fresh full delay. All other
independent reasons and pending signals remain unchanged by that recovery.

Unknown stream status always raises the separate uncertainty notice. With default
timeout disabled it adds no new reason and cannot clear an existing confirmed-offline,
manual, schedule, recovery or client-disconnect reason. An enabled timeout adds only
StreamStatusUnknown after a continuous unknown interval reaches its exact duration;
it never replaces or clears those existing reasons. Known status removes the uncertainty
notice. The pending-offline interruption and timeout-recovery policies remain gated
below; no test or caller may infer an answer from this draft's layout.

This interface returns a final proposal, not within-interval countdown history. A
caller integrating the timer must segment at delay expiry and use the pause set
applicable to each preceding interval; feeding the final paused flag back over the
whole pending interval would violate §4. Countdown phase, remaining values, hard-end
precedence, schedule occurrence calculation, downtime/recovery algorithms, permission
checks and atomic Qt/writer state commits remain separate contracts. No actual timer
integration, provider signal reliability or connection behavior follows from these
normalized synthetic values.

## Validation and signed bounds

Negative configured durations, negative elapsed/pending durations and incoherent
pending-plus-active snapshots reject with `std::invalid_argument`, without changing
the input. A pending duration must be strictly below its configured delay; zero-delay
conditions activate immediately and cannot have a pending snapshot. Unknown pending
requires an enabled timeout and its uncertainty notice. This revision does not
support reconfiguring a delay partway through an episode; callers must preserve the
episode configuration until a later reviewed configuration-change contract exists.
Signal-owned reasons cannot be changed through `set_pause_reason`; rejecting that
misuse preserves coherent signal ownership. Enum values outside declared alternatives
are not admitted domain values. Exception wording is not contractual.

Use remaining-delay comparison rather than unchecked `pending + elapsed`; a duration
of INT64_MAX can expire exactly without overflow. Once elapsed reaches the threshold,
only the active set member remains. No UTC instant is accepted, subtracted from a
monotonic duration, or persisted by this primitive.

## Unresolved product policy — final baseline gate

The specification, TIMER-03/05, existing contracts and repository evidence do not
resolve these combined cases. The coordinator has requested owner clarification.

1. Offline is pending but has not reached its delay, then stream status becomes
   Unknown. Must the offline pending duration cancel, freeze for later confirmed
   Offline, or continue advancing? Unknown itself is not confirmed Offline, and
   none of these choices may be silently encoded as supported product policy.
2. After optional unknown timeout activates StreamStatusUnknown, does any known
   stream status clear it, or only confirmed Online? Does each new Unknown episode
   start a fresh full timeout? The specification requires the independent reason
   and forbids clearing confirmed-offline/other reasons, but does not define these
   reset/recovery edges.

Until resolved, cases crossing those edges are intentionally unauthored, not skipped
tests or passing behavior claims. Full CORETEST-003 baseline approval and production
implementation remain blocked on recorded decisions, affected examples/tests and
independent review. This preserves full release scope rather than declaring those
interactions unsupported product behavior.

## Independently derived examples

The following use hand subtraction and set membership from §4; no implementation
has produced their expected values. Executable cases and complete evidence follow
in the authoring revision.

| Explicit sequence | Required result |
| --- | --- |
| Offline0, then elapsed29999, then1 with default delay | Pending0, pending29999/no new reason, then StreamOffline/no pending |
| Client disconnect0, then29999, then1 | Same threshold, independently of offline |
| Both pending with offline10/client20; advance10, then10 | First only StreamOffline, then both reasons |
| Manual + StreamOffline, then confirmed Online | Manual remains; no global resume |
| RecoveryReview + Schedule + client disconnect, then client connected | RecoveryReview and Schedule remain |
| Default Unknown for INT64_MAX, previously unpaused | Notice true, no new reason |
| Default Unknown while already StreamOffline + Manual | Both remain; notice true |
| Unknown timeout10, elapsed9 then1 | Notice throughout; only at10 add StreamStatusUnknown |
| Pending(INT64_MAX-1), elapsed1 with delayINT64_MAX | Activate exactly; no signed overflow |

## Ownership, review and CI gates

The author owns this contract, the new header/stub/tests, CORE-003 brief and authoring
evidence. The coordinator exclusively owns CMake/presets, shared progress/HTML,
acceptance map, integration and publication. Planned optional harness:
`SUBATHON_BUILD_PAUSE_POLICY_TESTS`, library `subathon_pause_policy`, executable
`core_pause_policy_acceptance`, preset `pause-policy-debug`, label `pause-policy`.
No existing production code or reviewed tests may be changed by this baseline.
The new intentionally red lane stays separate from stable automatic feature lanes.
An actual independent test review precedes the first test-body run; successful
compilation/discovery alone cannot establish observed red.
