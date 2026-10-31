# Main/grace lifecycle reducer — revision 1

Tasks: CORETEST-002 / CORE-002. Authority: specification v0.45
[§4](../specification.md#4-main-timer-grace-pauses-and-finishing), TIMER-01/02 and
§18 main/grace/hard-end scenarios; shared ownership invariants remain in
[event-state boundaries revision1](event-state-boundaries-v1.md).
Existing [time-credit](time-credit-v0.md) and [exact-decimal](exact-decimal-v1.md)
contracts are unchanged.

State: authored draft. The owner resolved positive credit during confirmation wait
in the [recorded clarification](../evidence/timer-confirmation-credit-decision.md).
Independent full review and intended red remain pending. Revision 1 has never been
finalized or implemented; this refinement requires no API or layout migration.
No production behavior is implemented by this baseline.

## Bounded public interface

`core/include/subathon/core/timer_lifecycle.hpp` provides pure value proposals:

- `advance_timer(state,config,step) -> TimerAdvance`: accepts an already-started
  Active or Finished state, returns the next snapshot and ordered transition proposals.
  Ready remains a representable lifecycle value, but this is not a start command.
  Advancing Ready throws `std::invalid_argument` as a local API precondition.
- `apply_applicable_time_credit(state,actual_ms) -> TimerState`: applies an already
  eligible, nonnegative, cap/debt-resolved amount to an Active timer snapshot.
  It is not contribution intake, payment review or a direct authoritative state write.
- `confirm_timer_finish(state) -> TimerState`: an already-authorized confirmation
  finishes an Active/Main zero-time state explicitly awaiting confirmation. Other
  states reject the command; operation deduplication and permission checks are external.

The caller first advances to each command observation, then computes current actual
credit against the returned authoritative state, then proposes the command. In
particular, a natural grace expiry or enabled hard end observed at that instant
finishes before the same-instant ordinary credit can rescue it. A Finished or Ready
snapshot cannot accept this credit command; reject it with invalid_argument rather
than implicitly reopen/start or falsely report timer credit. Recording post-finish
inputs and configurable score/automation behavior remain outside this timer primitive.

First start, Ready with a passed hard end, startup after a passed deadline, reopening,
manual signed adjustments, cap/deadline/configuration edits and their permission/audit
rules remain explicit API-007/CORE-008/CORE-007 contract gates. Ready rejection here
must not be interpreted as a product decision to ignore or automatically finish a
not-yet-started event. No start/reopen support is claimed by this bounded reducer.

## State, defaults and representations

These C++ layouts and snapshot conventions are local engineering choices consistent
with the agreed state model; they do not introduce a new product lifecycle.

| Property | Representation / invariant |
| --- | --- |
| Lifecycle | Ready, Active, Finished |
| Countdown phase | Main or Grace, independent of lifecycle |
| Main remaining | Nonnegative signed64 milliseconds |
| Grace remaining | Nonnegative signed64 milliseconds; retained while Main runs |
| Awaiting confirmation | Explicit flag on Active/Main with main remaining0 |
| End mode | Immediate, Grace (default), Confirmation |
| Grace duration | Default120000ms; configurable nonnegative signed64 duration |
| Grace reuse | Fresh (default) or Shared |
| Hard end | Optional signed64 UTC millisecond instant, not a countdown duration |

`grace_remaining_ms` is the retained shared budget; there is no duplicate field that
could disagree. In Fresh mode, each Main expiration resets it to the configured full
grace duration. In Shared mode, each Main expiration uses its retained remainder.
Main progression never consumes that budget. A zero configured grace or exhausted
shared budget has no artificial extra tick: enter Grace and immediately expire it.

Confirmation wait remains Active/Main with main0 and its explicit flag, not Finished,
a fourth lifecycle, negative time or a fake Grace phase. Awaiting confirmation cannot
coexist with positive main time or Grace. Positive actual applicable time clears the
wait and resumes Main automatically, retaining the grace budget. Zero actual credit
retains the wait and both remaining fields. A stale finish confirmation after
resumption rejects under the existing waiting-state precondition.

Phase is retained after finish, and no ordinary advance restarts a Finished snapshot.
For hard-end finish specifically, retained remaining fields are the pre-call snapshot
values. This is a representation choice: it preserves them without pretending to
calculate exact countdown values at an interpolated wall deadline. A consumer must
not label them precise elapsed-at-deadline history.

Pause reasons and their overlap/delay/unknown-status policies belong to CORE-003.
This reducer receives only the observation that a segment is paused; it never clears
any reason. Accepting already-eligible credit can switch Grace to Main or resume a
confirmation wait while the external pause remains active; the next paused observation
still freezes Main and preserves the retained grace budget.

## Explicit clocks and bounded observation ordering

`TimerClockStep` supplies nonnegative `elapsed_ms`, separate `utc_now_ms`, and a
constant `paused` observation for one interval. Elapsed duration comes from an
in-process monotonic source at the caller boundary. UTC is only a wall instant for
the hard-end comparison; its magnitude or difference is never substituted for elapsed.
No process-local monotonic point is persisted, and this domain code reads no clock.
Tests supply these values directly, without sleeps or Qt/event-loop dependencies.

The caller splits intervals at known pause/configuration/command changes. A boolean
at the interval end does not retroactively describe an interval containing a pause
change. Schedule/DST segmentation, clock skew/discontinuity handling and reconstructing
restart gaps remain CORE-004/CORE-006/STORAGE-003 work. This primitive does not invent
a wall/monotonic mapping, tolerance, historical transition UTC timestamps or downtime
policy from one observation.

For a valid Active snapshot, the declared observation priority is:

1. If `utc_now_ms >= hard_end_utc_ms` for an enabled deadline, finish immediately,
   clear confirmation wait and return HardEndReached, Finished. This applies even
   during pause, Grace or confirmation wait. No ordinary phase progression is
   projected first. This is deterministic observation priority, not a claim that an
   unknown within-interval wall chronology put the deadline before every expiry.
2. Resolve a clock already at zero according to its end mode before deciding whether
   the supplied pause permits further elapsed consumption. This recognizes an
   existing expiry; it does not consume time during pause.
3. If paused, consume no elapsed time in either phase. Otherwise consume Main first.
   At Main zero, emit MainExpired and select the configured ending mode.
4. Immediate emits Finished. Confirmation emits ConfirmationRequired and waits;
   leftover elapsed does not consume Grace or implicitly confirm.
5. Grace emits GraceStarted. Fresh resets the full budget; Shared retains it. Consume
   any unpaused overshoot in Grace in the same observation, rather than losing it
   or deferring it to a later tick. At Grace zero emit GraceExpired, Finished.
6. A Finished snapshot remains unchanged with no new transitions on later observations.

Remaining values never become negative. Use subtraction/bounded comparisons instead
of adding Main+Grace into a possibly overflowing signed64 temporary. Very large
elapsed values can cross both phases in one step and finish without wrapping.
`TimerAdvance.transitions` preserves important intermediate events even if only a
Finished snapshot remains. A long Grace-mode step yields exactly
MainExpired → GraceStarted → GraceExpired → Finished (at most four proposals).
These are ordered domain proposals, not already committed audit/events, provider
messages, generated-event payloads or timestamped effects. The application layer
must capture/commit their eventual event ancestry and state atomically under the
shared contract; this primitive cannot claim that wiring or discard the trace.

## Credit, validation and errors

A positive actual amount while in Grace sets Main remaining to that amount, switches
to Main and retains unused Grace for the selected repeated-grace behavior. Zero
actual amount does not rescue. While awaiting confirmation at Main0, positive actual
credit sets Main remaining to that amount and clears the wait automatically, without
consuming or resetting retained Grace or changing the hard end. Zero actual credit
keeps waiting. In Main, add the actual amount exactly; if the result cannot fit
signed64, throw `std::overflow_error` and return no partial proposal.
Actual amounts are not requested amounts: eligibility, cap/debt calculations,
provenance and review are resolved by the caller against the advanced snapshot.
No negative credit input can encode refunds or corrections in this boundary.

Invalid negative remaining values, grace duration, elapsed duration or credit amount
throw `std::invalid_argument`. Incoherent phase/wait fields also reject instead of
silently repairing authoritative state: Grace requires main0 and no confirmation
wait; confirmation wait requires Active/Main/main0 and Confirmation mode when config
is supplied. `confirm_timer_finish` requires that waiting state. Exception wording
is not contractual. Validation applies before producing a proposal, leaving caller
input and authoritative state untouched. C++ enum values outside their declared
alternatives are not admitted domain values.

Confirmation-wait resumption follows the owner's explicit choice recorded in
specification §4/§18. The actual amount is the caller's already-resolved result,
so a positive requested amount reduced to zero by debt/caps does not resume Main.
The caller still advances first: a reached hard end finishes even while waiting
and paused, and credit against that Finished result rejects without reopening it.

## Independently derived examples

All values and transitions below derive manually from duration subtraction, explicit
command ordering and the cited §4 requirements, not an implementation under test.

| Input / sequence | Required snapshot / transitions |
| --- | --- |
| Default Main1000, elapsed999 | Active/Main1, Grace budget120000, no transition |
| Default Main1000, elapsed1000 | Active/Grace120000; MainExpired, GraceStarted |
| Default Main1000, elapsed1001 | Active/Grace119999; overshoot1 consumed immediately |
| Default Main1000, elapsed121000 | Finished/Grace0; all four ordered phase/finish transitions |
| Grace1000, elapsed999 then1 | Active/Grace1, then Finished/Grace0 at exact boundary |
| Immediate Main1000, elapsed1000 | Finished/Main0; no Grace entry |
| Confirmation Main1000, elapsed121000 | Active/Main0 awaiting confirmation; unused Grace120000 |
| Confirmation wait, another500000ms | Still waiting, no Grace consumed or repeated transition |
| Authorized confirm of waiting snapshot | Finished/Main0, waiting cleared |
| Confirmation wait/Main0/Grace73000 plus actual5000 | Active/Main5000, waiting cleared, Grace73000 retained; stale confirm rejects |
| Confirmation wait/Main0/Grace73000 plus actual0 | Still Active/Main0 awaiting confirmation, Grace73000 retained |
| Confirmation wait accepted actual5000, next paused elapsed1000 | Main5000/Grace73000, waiting cleared; external pause still freezes Main |
| Paused confirmation wait/Main0/Grace73000 advanced at enabled deadline, then credit5000 | Finished/Main0/Grace73000, waiting cleared; HardEndReached, Finished; credit rejects |
| Grace90000 plus eligible actual5000 | Active/Main5000, retained Grace90000 |
| Grace90000 plus actual0 | Still Active/Grace90000 |
| Rescued Main5000 expires under Fresh | Grace120000 |
| Rescued Main5000 expires under Shared | Grace90000 |
| Shared Main5000 advances4000 then1500 | Main1000/budget90000, then Grace89500 |
| Paused Main or Grace, elapsed500000 | Phase and both remaining fields retained |
| Paused Grace accepted rescue5000, next paused elapsed1000 | Main5000; pause still freezes it |
| Enabled deadline1000, UTC999 versus1000 | Before: active; exact: Finished, even paused/in Grace |
| Enabled deadline reaches confirmation wait | Finished without authorized-confirm command |
| Deadline disabled, UTCINT64_MAX, elapsed100 from Main1000 | Main900; UTC cannot become elapsed duration |
| Grace1 advanced1 before same-instant credit | Finished; ordinary credit rejects, cannot reopen |
| Grace1 advanced0 before eligible credit5000 | Main5000; rescue occurs while Grace still exists |
| MainINT64_MAX, elapsedINT64_MAX | Grace120000, without overflowing a Main+Grace sum |
| Main1000, elapsedINT64_MAX | Finished with both countdowns0, no negative values |
| Ready advanced | Invalid argument: start contract required, no chosen Ready deadline policy |

Executable home: `tests/acceptance/timer_lifecycle_tests.cpp`. Pure explicit-value
GoogleTests cover this bounded behavior and the local validation/representation
choices. They are authored examples until separately reviewed and observed red;
they establish no persistence, server loop, provider or supported-platform behavior.

## Baseline and downstream gates

The baseline provides a minimal header and three unconditional
`TimerLifecycleNotImplemented` stubs only. Coordinator-owned CMake adds the separate
optional `timer-lifecycle-debug` lane; existing stable credit/numeric/scaffold lanes
must stay green. Do not invert, skip or relabel intentional failures as acceptance
success. CORE-002 must add its stable passing timer suite to automatic CI and observe
that workflow before implementation completion.

No prior assertions, numeric contracts, schemas or persisted data are changed.
First-start/deadline gates and downstream clock/pause/recovery/schedule/accounting
wiring remain explicit. All release scope, including post-finish/reopen/history
policies, remains required; these boundaries do
not implement or remove it. Independent review of the actual final contract/test
revision and a genuine intended stub-red run precede production implementation.
