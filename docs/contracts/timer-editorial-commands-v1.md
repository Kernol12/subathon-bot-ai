# Timer editorial commands — proposed revision 1

CORETEST-008, document stage only. Authority: specification v0.45
[§3.5](../specification.md#35-numerical-precision-and-validation),
[§4](../specification.md#4-main-timer-grace-pauses-and-finishing), TIMER-01,
NUM-02/03/04 and the [CORE-008 brief](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-001/CORE-008.md).
Original partial proposal: main0b631dc/claimc311f0b. This completed logical
continuation starts at `5601a06b` after current-main integration and the
[recorded owner decisions](https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/timer-editorial-owner-decisions.md).

**Completed logical candidate, not an executable baseline.** All three original
owner questions are resolved in specification §4/§18/TIMER-01. Their warning and
boundary outcomes below replace the old pending alternatives. The prior partial
review remains valid for unchanged material; different actual full review of this
revision and the coordinator's specification transcription is still required.
No API/header/stub/tests, numeric algorithm, schema or production behavior is supplied.

## Purpose and existing boundaries

Define a small pure projection for authorized manual/editorial timer commands:

| Command | Requested change |
| --- | --- |
| Signed time delta | An already finalized signed64 millisecond adjustment, retaining the full requested amount and explicitly selected negative-deficit policy |
| Replace remaining-time cap | A nonnegative signed64 cap or absent, with original/replacement settings retained |
| Replace hard deadline | A signed64 UTC instant or absent, with original/replacement settings retained |

A time **delta** differs from setting an absolute Main value during first start or
reopening. It also differs from an explicit debt correction. None can be encoded
by overwriting the other, resetting Grace, turning debt into negative Grace or
silently changing a lifecycle flag. First-start admission remains coordinated
with [API-007](https://github.com/Kernol12/subathon-bot-ai/issues/290); reopening with
[CORE-007](https://github.com/Kernol12/subathon-bot-ai/issues/183); debt corrections require the separate
permission/audit in §4/TIMER-10. Those required behaviors remain in release scope.
The lifecycle matrix below states exactly which obligations this projection owns
and which require those separately reviewed owning command contracts.

Reuse [CORE-002 lifecycle](timer-lifecycle-v1.md) and
[CORE-005 numeric debt](time-debt-v1.md). The older
[time-credit revision1](time-credit-v0.md) keeps its narrow above-cap rejection.
Do not duplicate its arithmetic or broaden that interface to accept these commands.
The current [event/state contract](event-state-boundaries-v1.md) continues to own
record separation, authoritative queue/commit and requested/actual provenance.

This core boundary neither derives a financial refund entitlement nor changes
an original effect. ACCT-006 owns original actual awards, proportional refunds and
reversal allocation. A manual command has its own retained command identity and
source/actor/reason; a shared numeric helper does not manufacture a financial
original-effect reference or prove entitlement. Concrete adapters must retain this
distinction before calling a helper whose parameter names mention reversals.

## Owned input and proposal model

The following are logical owned values, not a finalized C++ or wire layout:

- Current snapshot: opaque nonempty authoritative revision ref, complete existing
  TimerState, separate nonnegative signed64 time debt, optional Main cap and full
  existing TimerConfig. Independent pause/recovery state stays caller-owned and
  supplies explicit constant-pause clock segments.
- Command: nonempty operation ref and expected revision ref, exactly one command
  payload above, honestly optional actor and reason, and the explicit negative
  policy when that payload needs one. A cap edit below observed Main additionally
  retains the per-attempt Keep/Reduce/Abort choice for that proposed cap and
  expected revision, with any warning-preview Main retained honestly as preview
  data. Final actual effects use observed-before Main and current authorization.
  No global cap-choice or deficit default exists.
- Current access result: allowed, denied or unknown, supplied by the trusted
  authoritative service for this exact command/current revision. An allowed result
  retains the actual permission revision ref; the other alternatives supply no
  grant. These are DATA from a trusted boundary, not a public capability or proof
  that a caller is authenticated.
- Observation: explicit ordered TimerClockStep segments and a final UTC observation;
  elapsed monotonic duration and UTC stay independent. The caller supplies valid
  segmentation at pause/configuration/command boundaries. Core reads no clock and
  invents no restart/downtime interval.
- Proposal: checked original revision, retained command, original settings/state,
  observed-before timer, optional accepted after-state/settings, ordered lifecycle
  transitions and the exact requested/actual adjustment components or setting edit.
  Its disposition distinguishes a proposal from denial, revision conflict,
  invalid input, cap choice required, explicit abort, prerequisite operation,
  timer-only review and numerical overflow requiring review. No accepted edit is
  represented by an absent accepted-after value, not a fabricated zero effect.

Proposals have no writer/SQL handle, authoritative permission object, generated
state token, implicit next revision or delivery job. The host owns fresh revision
issuance and the actual atomic compare-and-swap/commit. A pure expected/current
byte comparison cannot prove durable freshness, global token uniqueness, operation
idempotency, authorization or that a proposal has been published.

Expected revision compares against the complete current timer/settings revision
supplied by the authoritative host. It must cover the configuration used for cap,
deadline and end-mode interpretation as well as remaining state. The host must
not treat an old state-only token as proof that unrelated settings/grants remain
current. A stale preview yields explicit conflict or a separately requested new
current preview; never silently apply the old proposal to a newer revision.

Two commands checked against r1 may each produce a pure proposal. Once the host
commits one as r2, it must refuse the other's r1 write. Core does not serialize
threads or claim to resolve that race by returning a proposal. A zero delta or
identical settings request still preserves its command/audit identity; whether a
server allocates a new durable state token is its separately reviewed protocol.

## Settled composition and validation obligations

Check the current access result and revision before generating an editorial
change. Denied/unknown access produces no accepted edit; mismatched expected
revision produces conflict with no reused old proposal. Concrete API work must
fix exact validation priority, finite ownership/resource ceilings and typed error
representation before executable tests; this document does not choose them by
accident through a C++ exception class.

For an already-started timer, first apply each supplied observation segment under
its actual applicable settings using CORE-002, preserving its ordered transitions.
In particular an enabled old hard deadline observed at UTC >= deadline is reached
before a same-observation editorial command can pretend the timer was still
Active. A later settings edit cannot erase that observation or implicitly reopen.
The old deadline's pre-call remaining-field convention stays CORE-002's convention.
No client timestamp or UTC subtraction replaces the explicit observation.

A new edit does not retroactively replace the settings used for an earlier elapsed
segment. Configuration/permission mutations are serialized by the host. Changing
or disabling a future deadline before it is reached is an explicitly authorized
edit; pausing never changes that deadline by itself. If the observed timer remains
Active, an authorized replacement deadline at/before that final UTC is accepted
and immediately finishes it, as defined below. Ready admission uses the separate
resolved first-Start gate; settings edits never implicitly start or reopen.

For a permitted Active signed delta:

- Nonnegative delta uses CORE-005 debt-first addition and the actual current cap.
  Retain full request, debt repaid, actual Main added and cap-limited remainder.
  Compose only actual Main added through CORE-002 on the running timer before that
  amount is installed; never add requested time twice or rescue with debt repayment.
- Negative delta uses the explicitly supplied CORE-005 deficit policy. Retain the
  full signed request, actual nonpositive Main adjustment, nonnegative debt added
  and signed unapplied remainder. INT64_MIN remains valid request DATA; never
  negate it in signed64. CORE-005's RequiresReview remains a timer-only result.
- When an applied subtraction exhausts Main, compose a zero-elapsed CORE-002
  observation at the same final UTC/pause/configuration for its actual ending
  policy. Grace/debt remain separate, and existing independent pauses remain.
- Overflow yields no accepted partial editorial effects or jobs. Preserve the full
  request for application-layer review. No binary floating-point shortcut, wrap,
  saturation or hidden cap clamp substitutes for exact checked final values.

A RequireReview timer proposal does not itself persist a hold, resolve existing
holds or select a global application status. For a compound command containing
other effects, whole-input deferral versus timer-held plus a complete result must
be defined with CORE-008/ACCT-005 before enablement. Financial reversal policy stays
ACCT-006. This document selects neither mixed policy nor automatic continuation.
Unknown policy is explicit unresolved work, never a successful zero adjustment.

## Lifecycle scope and command ordering

The complete proposal distinguishes original snapshot, observed-before snapshot,
and accepted-after editorial snapshot. An unchanged observed value is not an
accepted edit. First validate the complete current inputs/access/revision; the
later API freezes exact error/resource priority. For a valid authorized current
request, observe under the OLD settings, select the command against observed-before,
then preserve the chronological transition list without replay or interpolation.

| Observed lifecycle | Signed delta | Cap replacement | Deadline replacement |
| --- | --- | --- | --- |
| Ready | No accepted delta; explicit first-start/absolute-Main prerequisite in API-007 | May replace cap with the per-attempt choice below; retain Ready | May change/disable deadline; retain Ready even when it is already passed |
| Active | Apply the complete signed operation using CORE-005/CORE-002 | May replace cap; Reduce can cause the existing selected zero-Main ending transitions | May change/disable deadline; a replacement <= final observation finishes immediately |
| Finished | No ordinary delta or implicit reopening; require CORE-007's explicit reopen/post-finish/correction policy | May replace current cap; retain Finished and immutable history | May change/disable current deadline in preparation for explicit reopening; retain Finished |

The signed-delta Active admission is a bounded LOCAL projection condition, not a
product ban or a claim that Ready/Finished manual behavior is solved. In those
states return an explicit prerequisite/not_ready outcome with no accepted ordinary
delta; the API-007 first-start/absolute-Main and CORE-007 post-finish/reopen/correction
contracts remain mandatory release/integration work. This condition is not inferred
from CORE-002's Ready enum restriction. The setting-edit admissions are routine
logical choices permitting configuration/reopening preparation; they do not grant
a Start/Reopen operation.
A cap Reduce on Ready/Finished explicitly changes the CURRENT retained Main field,
with its audit, but emits no countdown/finish/reopen transitions and rewrites no
historical snapshot or original credited effect. First Start/reopening still use
their owning absolute-Main validation/permission and explicit grace/debt/history
rules. Post-finish ordinary support/score/action policy is CORE-007, not this
three-command projection. Do not call CORE-002's Active-only credit operation on
Ready/Finished or disguise lifecycle refusal as cap limitation/unapplied credit.

A Ready timer has Main phase, no confirmation wait and nonnegative retained Main/
Grace values; it does not count down. Observing elapsed intervals while it remains
Ready leaves these fields/lifecycle unchanged and emits no transitions. Use an
explicit Ready branch rather than invoking CORE-002's rejecting Ready precondition.
Finished observation follows CORE-002's unchanged/no-new-transition convention.
Active/Finished validation otherwise preserves CORE-002's field/enum rules. Debt,
optional current cap and configured Grace duration must be nonnegative signed64;
Main above cap is valid after Keep, never an invalid-state shortcut. Confirmation
wait requires Active/Main0 and the Confirmation end mode. All phases/debt/pauses
remain independent; negative Grace or a synthesized clear-pause operation is invalid.

Every observation has at least one explicit step, possibly elapsed0, and its final
step UTC equals the final command observation. Each nonnegative elapsed signed64
step has a constant pause/configuration interpretation supplied by the host.
All steps in one proposal use the supplied unchanged OLD TimerConfig/cap; the
host sequences separate proposals across configuration changes rather than passing
unrepresented historical settings through one current-config value. Pause may vary
between explicit constant-pause steps. UTC instants are signed64 and are compared
directly, never subtracted into elapsed or added to a duration. No monotonic/UTC
correspondence or wall-clock tolerance is invented. Invalid/unresolved segmentation
or policy cannot produce an accepted edit.

If old observations finish the event, a subsequent signed delta requires the
Finished prerequisite even when its input snapshot was Active. A later cap/deadline
edit can be accepted on that observed Finished state, but cannot undo the old
HardEndReached/Finished or natural expiry transitions. Once Finished has been
observed, moving the deadline into the future is not reopening. An identical
settings request preserves its identity/audit and cannot generate repeated finish
transitions. Independent access/whole-command authorization is still required.

## Resolved per-attempt cap warning

Compare a proposed finite cap with observed-before Main. Removing the cap or
setting it >= Main needs no below-Main warning/reduction. If it is lower, return
choice-required with no accepted edit until the exact attempt has an explicit
Keep, Reduce or Abort. The warning must explain original/proposed cap, current Main
and the consequences, including that final actual amounts are revalidated against
current observed state; a raw acknowledgement field cannot prove that the UI showed
it or supply permission. The trusted service/UI integration must implement that
warning. Never silently select Keep or a stored/global alternative.

| Choice for the affected attempt | Complete editorial outcome |
| --- | --- |
| Keep | Accept the new cap; retain observed Main/debt/Grace/wait/phase. Main may exceed cap; future additions still repay debt first and get zero Main headroom while Main>=cap |
| Reduce | Accept cap and an explicit audited Main reduction together. Requested/actual editorial Main adjustment = new cap minus observed Main; Main becomes cap, debt is unchanged, capped/unapplied reduction is0. If Active Main becomes0, compose the existing zero-elapsed ending reducer at the same UTC/pause/configuration; otherwise preserve lifecycle/phase/Grace/wait |
| Abort | No accepted cap or Main reduction; retain original settings. Preserve the attempt/choice and observed-before data as facts, without representing them as accepted editorial effects |

Reduce's exact magnitude is at most INT64_MAX because 0<=new cap<Main<=INT64_MAX.
It creates no deficit/debt, clears no existing debt and never borrows a financial
original-effect reference. Authorization must cover the ENTIRE chosen cap+reduction
operation, not merely a cap permission plus a client-selected Reduce flag. Both
changes accept as one unit or neither does. This is not an arbitrary mixed-effects
input or permission to apply only half of an operation after overflow/refusal.

After Keep, Main above the new cap is legitimate. Route eligible positive deltas
through the already-reviewed CORE-005 addition contract: zero Main headroom while
Main>=cap, still repay debt, and retain the exact remaining cap-limited amount.
Only actual Main credit is passed to the applicable CORE-002 composition. Do not
call the older time-credit-v0 helper on its forbidden above-cap input, weaken its
assertions or infer that Keep creates extra headroom. ED42–44 bind the existing
CORE-005 behavior to an explicit post-Keep sequence; the new wrapper composition
still requires its own independently reviewed tests/red before implementation.

A stale revision/settings change invalidates that attempt's confirmation; the host
must conflict/revalidate against current facts, not apply an old snapshot or reuse
a choice for another cap. Elapsed observation within an otherwise current attempt
is still applied first: Reduce uses the actual excess at that observation, not the
preview's old amount. If Main no longer exceeds the new cap, no reduction is needed;
a prior Reduce choice must never add time. The future API/protocol must retain both
preview and actual facts without requiring the countdown to freeze for a warning.
Abort cancels the two editorial changes; it neither reverses already elapsed/
committed time nor postpones a hard end.
Whether an observation from an aborted/refused attempt commits as a separate clock
operation remains the explicit server command/clock transaction integration gate.
This pure model supplies no hidden clock-only write or permission to ignore the
server's independently required deadline/checkpoint progression.

## Resolved hard-deadline and first-Start boundaries

An authorized deadline edit on an observed Active event first preserves old-setting
observation. With a new enabled deadline <= final UTC, install that setting in the
candidate and make a zero-elapsed CORE-002 observation under it. Retain its exact
HardEndReached, Finished transitions, pre-call remaining-field convention and cleared
confirmation wait. Paused Main, Grace and confirmation wait do not defer it. A
future/disabled deadline leaves the observed current timer intact. No deadline edit
changes Main/debt/Grace directly or retroactively supplies a countdown segment.

For Ready, changing/enabling a deadline at/past the observation retains Ready; it
does not fabricate an active timer just to finish it. The first Start admission
rule, owned jointly with API-007, is now fixed: if enabled deadline <= authoritative
observation, reject Start and remain Ready without starting countdown or emitting
hard-end/finish transitions. Change/disable the deadline first. A future/disabled
deadline passes this PARTICULAR gate, not every setup/permission/initial-Main check.
API-007 must still implement/review the actual first-Start operation and isolated
setup/live transition; this document grants no Start via a successful settings edit.

For Finished, changing/disabling the deadline preserves Finished and its retained
fields/history. An explicit CORE-007 reopen must resolve any still-passed deadline,
choose absolute Main and grace reset/retain, preserve debt unless separately
corrected, and handle pending groups/holds without replay. Those prerequisites
remain required; a future deadline by itself is not permission or a reopen result.

## Independent literal examples for settled behavior

All numbers are milliseconds; no example comes from an implementation under test.
M denotes INT64_MAX = 9223372036854775807. Unless a row says otherwise, its service
has allowed this command for the exact current revision, observation elapsed is0,
there is no reached hard end, and observed-before remains Active. These conditions are
explicit prerequisites, not a default permission/lifecycle policy.

| ID | Input | Required observation/proposal |
| --- | --- | --- |
| ED01 | Main55/debt2, delta+10, cap60 | Debt repaid2, Main added5, capped3; Main60/debt0 |
| ED02 | Existing Main100/debt5, delta+8, cap60 | Existing Main100 retained; debt repaid5, Main added0, capped3; debt0. This tests an addition, not acceptance of a cap edit |
| ED03 | Grace90/Main0/debt5, delta+5, no cap | Actual Main0/debt repaid5; remain Grace90, debt0; existing pause reasons remain |
| ED04 | Confirmation wait/Main0/retained Grace73/debt5, delta+7, no cap | Debt repaid5, actual Main2; resume Main2, clear wait, retain Grace73 and pauses |
| ED05 | Main100/debt7, delta-150, StopAtZero | Main0/debt7; Main-100, debt+0, unapplied-50; compose selected zero-Main ending policy |
| ED06 | Main100/debt7, delta-150, CarryTimeDebt | Main0/debt57; Main-100, debt+50, unapplied0; compose ending policy |
| ED07 | Main100/debt7, delta-150, RequireReview | Timer-specific unchanged100/7, actual components0, full-150 remains held DATA. No global/durable hold status is inferred |
| ED08 | Main1/debt0, deltaINT64_MIN, CarryTimeDebt | Main0/debtM; Main-1, debt+M, unapplied0; no signed negation of INT64_MIN |
| ED09 | Main0/debt0, deltaINT64_MIN, CarryTimeDebt | Unrepresentable debt9223372036854775808: no partial proposal, full request retained for review |
| ED10 | MainM/debt0, delta+1, no cap | Unrepresentable final Main; no accepted editorial effects |
| ED11 | Main100/debt0, actual unpaused elapsed10 then delta+5/no cap | Observed-before Main90; editorial Main+5; after Main95. Elapsed countdown is not misreported as part of requested+5 |
| ED12 | Main55/debt2/cap60, replace cap100 | Cap before60/after100; Main55/debt2 and all unrelated fields retained |
| ED13 | Active paused Main50, old deadline1000, UTC999, disable deadline | Explicit authorized disable may change optional deadline to absent; pause alone never does; Main50 retained |
| ED14 | Active paused or Grace, enabled deadline1000, UTC1000 | Existing CORE-002 observation is Finished with HardEndReached then Finished. No edit may portray this as uninterrupted Active time; final edit/start/reopen admission is a separate gate |
| ED15 | Current revision r2, command expected r1 | Revision conflict; no accepted old proposal or inferred overwrite |
| ED16 | Current access denied or unknown, otherwise valid request | No accepted editorial change and no fabricated grant |

The original ED01–16 rows above are preserved byte for byte from the approved
partial document. The following literal bindings complete their previously partial
lifecycle outcomes. These are EXAMPLE choices, not a change to the product default
Grace duration/end mode. Unmentioned debt/Main/settings are exactly as in the
referenced row, and independent pause/recovery reasons are unchanged.

The complete common EXAMPLE input for ED01–16 is Active/Main100/Grace73/debt0,
not waiting, explicit Immediate end mode/configured Grace120/Fresh reuse, no cap
or hard deadline, one unpaused elapsed0/UTC500 step, allowed access with permission
revision p1 and matching expected/current timer revision r1. Each original row
replaces exactly the fields/command it states; ED15/16 use delta+5. These are
literal fixture bindings, not product defaults or authority certificates.

ED11 replaces elapsed with10. ED03 is Active/Grace90/Main0/debt5, Grace mode,
paused; ED04 is Active/Main0/Grace73/debt5 already awaiting confirmation,
Confirmation mode, paused. ED09 starts already awaiting confirmation at
Active/Main0/Grace73/debt0 in Confirmation mode; a zero-elapsed observation keeps
it Active, avoiding accidental finish before its debt-overflow case. ED13 uses its
paused/UTC999/old-deadline1000 fields. ED14 is fully bound by ED34/35 below.

| Completion | Full literal outcome at the same observation |
| --- | --- |
| ED01/02 | ED01 Active/Main60/Grace73/debt0; ED02 Active/Main100/Grace73/debt0. No transitions; wait false |
| ED03/04 | ED03 Active/Grace90/Main0/debt0 with no wait; ED04 Active/Main2/Grace73/debt0 with wait cleared. No TimerTransition is invented by credit; pause remains |
| ED05/06 | Finished/Main0/Grace73, wait false; debt7 for ED05, debt57 for ED06. Editorial transitions MainExpired, Finished; full original actual/unapplied components retained |
| Full ED07 | Observed Active/Main100/Grace73/debt7, wait false, no transitions; timer-review-required, no accepted edit, actual components0 and full -150 held. No persisted/global status is inferred |
| Full ED08 | Finished/Main0/Grace73/debtM, wait false; MainExpired, Finished; actual Main-1/debt+M/unapplied0 |
| ED09/10 | ED09 observed Active/Main0/Grace73/debt0 still waiting; ED10 observed Active/MainM/Grace73/debt0 not waiting. No transitions, accepted-after absent, full requests retained, numeric-overflow-review; no partial timer change/jobs |
| ED11/12 | ED11 observed Main90 then accepted Active/Main95/Grace73/debt0; ED12 accepted Active/Main55/Grace73/debt2/cap100. Wait false, no transitions; preserve observed versus editorial components |
| Full ED13 | Explicit Immediate config, paused Active/Main50/Grace73/debt0; accepted deadline absent at UTC999, otherwise identical; no transitions |
| ED15/16 | No accepted edit or lifecycle transitions generated by the editorial operation; never reuse an old observation/proposal after revision/access refusal. An independently committed clock command is not implied |

## New independent owner-decision and lifecycle examples

All duration values below are milliseconds, including the illustrative100/120/60
cap examples; the separately recorded minute-valued UI example has the same exact
proportions. Every command is current and authorized for the whole chosen operation
unless explicitly refused. Each listed step and config is supplied, not read from a
clock. H,F abbreviates HardEndReached, Finished in that order; no other transitions
are omitted where the row says none. Rows are logical examples, not executed tests.

| ID | Complete relevant input | Required complete outcome |
| --- | --- | --- |
| ED17 | Active/Main100/Grace73/debt5, Immediate, cap120, deadline absent, elapsed0/UTC500; cap60 with Keep | Accepted Active/Main100/Grace73/debt5/cap60, wait false; actual Main0/debt0, no transitions; warning/Keep retained |
| ED18 | Same ED17 input; cap60 with Reduce | Accepted Active/Main60/Grace73/debt5/cap60, wait false; requested/actual Main-40, debt0, unapplied0; no transitions; explicit audited correction |
| ED19 | Same ED17 input; cap60 with Abort | Aborted, accepted-after absent; observed Active/Main100/Grace73/debt5 and original cap120 retained; actual editorial components0, no transitions |
| ED20 | Same ED17 input; cap60 with no choice | Choice-required, no accepted edit; warning includes Main100, cap120→60 and reduction40; no fallback/default |
| ED21 | Active/Main100/Grace73/debt5, Immediate, cap120, deadline absent; unpaused elapsed10/UTC500, then cap60/Reduce | Observed Main90; accepted Main60/debt5/Grace73/cap60, still Active/Main, wait false; editorial Main-30, countdown-10 separate; no transitions |
| ED22 | Active/Main100/Grace73/debt5, Immediate, no hard end, paused elapsed0/UTC500; cap0/Reduce | Finished/Main0/Grace73/debt5/cap0, wait false; editorial Main-100/debt0; MainExpired, Finished even paused; pause reason unchanged |
| ED23 | Same ED22 except Confirmation end mode | Accepted Active/Main0/Grace73/debt5/cap0 awaiting confirmation; MainExpired, ConfirmationRequired; pause unchanged |
| ED24 | Same ED22 except Grace mode, configured120; Fresh versus Shared | Accepted Active/Grace120 (Fresh) or Grace73 (Shared), Main0/debt5/cap0, wait false; MainExpired, GraceStarted; paused zero step consumes no Grace |
| ED25 | Same ED17, replace cap100 (equal Main) or absent, with no below-Main choice | Accepted Active/Main100/Grace73/debt5, cap100 or absent respectively, wait false; actual Main0/debt0, no warning/reduction or transitions |
| ED26 | Ready/Main100/Grace73/debt5, Immediate, cap120, no deadline, elapsed10/UTC500; cap60/Reduce | Accepted Ready/Main60/Grace73/debt5/cap60, wait false; explicit Main-40, no countdown or lifecycle transitions; Start remains separate |
| ED27 | Finished/Main100/Grace73/debt5, Immediate, cap120, no deadline, elapsed10/UTC500; cap60/Reduce | Accepted Finished/Main60/Grace73/debt5/cap60, wait false; explicit current Main-40, no replay/start/reopen transitions or historical rewrite |
| ED28 | Ready/Main100/Grace73/debt0, Immediate, deadline1000, UTC1000 or1001; first Start requested with other setup checks valid | Start rejected/remain Ready/Main100/Grace73/debt0, wait false; no countdown/H,F; API-007 must implement this resolved gate |
| ED29 | Same Ready state; deadline1001 at UTC1000, or deadline absent | Deadline-admission passes; pure check leaves Ready unchanged/no transitions. Full Start still requires API-007's current setup/absolute-Main transaction, not a settings command |
| ED30 | Ready/Main100/Grace73/debt0, Immediate, deadline absent; elapsed10/UTC1000, set deadline999 or1000 | Accepted setting, retain Ready/Main100/Grace73/debt0, wait false, no transitions; a subsequent Start is blocked until deadline changes/disables |
| ED31 | Active/Main50/Grace73/debt5, Immediate, paused elapsed0/UTC1000; old deadline absent or2000, set999 or1000 | Accepted new deadline and Finished/Main50/Grace73/debt5, wait false; H,F once. Main is retained pre-call value, not set to0 |
| ED32 | Active/Grace90/Main0/debt5, Grace mode, paused elapsed0/UTC1000, no old deadline, set1000 | Accepted deadline; Finished/Grace90/Main0/debt5, wait false; H,F once, no Grace consumption/reset |
| ED33 | Active/Main0/Grace73/debt5 awaiting confirmation, Confirmation mode, paused elapsed0/UTC1000, no old deadline, set999 | Accepted deadline; Finished/Main0/Grace73/debt5, wait cleared; H,F once, no authorized-confirm command needed |
| ED34 | Active/Main50/Grace73/debt5, Immediate, unpaused elapsed10/UTC1000; old deadline1000, then disable it | Old observation first yields Finished/Main50/Grace73/debt5 and H,F, not Main40. Accept disabled setting on Finished; remain Finished, no second transition or reopening |
| ED35 | Active/Grace90/Main0/debt5, Grace mode, unpaused elapsed5/UTC1000, old deadline1000, then set2000 | Old observation yields Finished/Grace90/Main0/debt5 with H,F, not Grace85; accept future setting, retain Finished and those fields, no replay |
| ED36 | Finished/Main50/Grace73/debt5, Immediate, old deadline1000, UTC1001; disable or set2000 | Accepted setting, retain Finished/Main50/Grace73/debt5, wait false; no transitions. Separate CORE-007 reopen/absolute-Main/grace/debt/hold prerequisites remain |
| ED37 | Active/Grace1/Main0/debt5, Grace mode, no deadline; unpaused elapsed1/UTC500 then delta+10 | Observed Finished/Grace0/Main0/debt5; GraceExpired, Finished; no accepted ordinary delta, explicit Finished prerequisite; actual editorial Main0/debt0, full request10 retained, not mislabeled cap-limited |
| ED38 | Active/Main50/Grace73/debt5, Immediate, old deadline1000, UTC1000; cap20 with Abort | Observation H,F retains Main50/Grace73/debt5; cap/reduction aborted, cap unchanged, no accepted edit. No core clock write is implied; host's required clock/CAS handling remains explicit |
| ED39 | Two cap commands expect r1/Main100/cap120; one commits r2, then the other tries its old Reduce | Second is revision conflict, no accepted cap/time overwrite or reused confirmation. Recompute any warning against current coherent r2 before a new attempt |
| ED40 | Active/MainM/Grace73/debt5, Immediate, cap absent/deadline absent, elapsed0/UTC500; cap0/Reduce | Finished/Main0/Grace73/debt5/cap0, wait false; requested/actual Main -9223372036854775807, debt0; MainExpired, Finished, no signed overflow |
| ED41 | Active/Grace90/Main0/debt5 in Grace mode, or Active/Main0/Grace73/debt5 awaiting confirmation in Confirmation mode; cap120, no deadline, paused elapsed0/UTC500; replace cap0 | Accepted cap0, same phase/Main/Grace/debt/wait and no transitions in either case; Main0 is not above cap0, so no below-Main reduction/warning is invented |
| ED42 | ED17 Keep has accepted Active/Main100/Grace73/debt5/cap60, Immediate/no deadline; next authorized delta+8 at elapsed0/UTC500 | Debt repaid5, actual Main added0, capped3; accepted Active/Main100/Grace73/debt0/cap60, wait false; no transitions. Above-cap state remains valid |
| ED43 | Same post-Keep snapshot as ED42; instead next delta+3 | Debt repaid3, actual Main added0, capped0; accepted Active/Main100/Grace73/debt2/cap60, wait false; no transitions |
| ED44 | Same post-Keep snapshot as ED42; next unpaused elapsed45/UTC545 then authorized delta+12 | Observed Main55/debt5; repay5, actual Main+5, capped2; accepted Active/Main60/Grace73/debt0/cap60, wait false, no transitions. Countdown-45 is separate from editorial+5 |

ED34/35 complete ED14's old-deadline observation without retrospectively changing
its original literal statement. ED38 preserves observation/Abort distinctions and
requires explicit host clock/command commit design; it grants no partial durable
write. Missing Start/Reopen/debt-correction or mixed-input contracts remain visible
owning prerequisites, not newly unanswered versions of the three resolved choices.

## Finite input/output and compatibility obligations

Proposed local logical defaults are 4096 bytes per opaque reference or optional
actor/reason field, 4096 clock segments, 8MiB complete owning input and 16MiB complete
owning output with at most 32768 transition/container entries. These are configurable
finite evaluation profiles, not universal wire/storage caps. Empty required refs
reject; honestly absent optional actor/reason differs from present-empty. References
compare exact bytes (NUL/high bytes included); a correlation cap is not a domain cap.

CORE-002 contributes at most four transitions per segment; one editorial finishing
observation adds at most four, so the checked projection bound is 4*(segments+1).
Retain the actual ordered list, not fabricated transitions to fill the bound.
The API must freeze precise B/I accounting, validation/unknown/permission/revision
priority, overflow and before-copy/append projections for every original/observed/
accepted snapshot, command/settings/reference and transition list. Oversize work is
a distinct resource refusal, never a truncated successful proposal. Signed64
numeric and UTC extrema remain valid field values where their owning domains allow;
negative remaining/debt/cap/elapsed and invalid enums reject. No Main+Grace total,
INT64_MIN negation, float, UTC-as-elapsed shortcut or partial overflow effect occurs.
Existing std and selected exact arithmetic suffice; no new dependency is proposed.

## Next gates and compatibility

Obtain actual separate full logical review of this revision and the coordinator's
specification transcription. All three owner answers are incorporated; no further
selection is needed for them. Preserve the valid prior partial review and original
numeric/countdown contracts. A separate concrete API review precedes minimal inert
source/harness, independent specification-linked tests/review and a genuine intended
missing-behavior red before production. An empty ready queue or existing core
passes does not establish this baseline or product behavior.

No existing header, ABI, schema, state token or assertion changes in this stage.
The future server adapter retains command/settings before-after provenance, current
access, actual revision CAS and exact applied effects atomically; independent code
review/acceptance and delivery/recovery/platform gates remain. All seven integrations,
three FX providers, renderers/UI/localization/themes and full release scope remain.
