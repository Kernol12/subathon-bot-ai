# Timer editorial commands — proposed revision 1

CORETEST-008, document stage only. Authority: specification v0.45
[§3.5](../specification.md#35-numerical-precision-and-validation),
[§4](../specification.md#4-main-timer-grace-pauses-and-finishing), TIMER-01,
NUM-02/03/04 and the [CORE-008 brief](../tasks/EPIC-001/CORE-008.md).
Prepared from integrated main0b631dc and coordinator claimc311f0b.

**Proposed document, not an executable baseline.** Three owner questions below
are unanswered. Their alternatives have no chosen default, supported behavior or
executable oracle. This stage introduces no header, stub, tests, numeric algorithm,
schema or production behavior. Independent actual document review and the complete
per-behavior test-first sequence still apply.

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
with [API-007](../tasks/EPIC-008/API-007.md); reopening with
[CORE-007](../tasks/EPIC-001/CORE-007.md); debt corrections require the separate
permission/audit in §4/TIMER-10. Those required behaviors remain in scope, with
explicit unresolved gates here rather than invented support.

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
  policy when that payload needs one. No default deficit policy is selected here.
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
  invalid input, unresolved policy and numerical overflow requiring review.

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
edit; pausing never changes that deadline by itself. Deadline-at/past-now and Ready
admission still depend on the owner decisions below.

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

A cap-only or deadline-only edit preserves unrelated numeric state, debt, Grace,
phase, confirmation wait, pauses/recovery and history except for lifecycle changes
that actually follow the observed authoritative deadline. Lowering below current
Main remains OPEN-CAP below; the preceding statement does not settle it.
Changing only a cap does not imply changing end mode or hard deadline. Existing
signed64 field and enum validity still apply. Never initialize/reopen from a Ready
or Finished enum merely because it can be represented in the input.

## Independent literal examples for settled behavior

All numbers are milliseconds; no example comes from an implementation under test.
M denotes INT64_MAX = 9223372036854775807. Unless a row says otherwise, its service
has allowed this command for the exact current revision, observation elapsed is0,
there is no reached hard end, and the timer remains Active. These conditions are
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

ED14 deliberately fixes only the already-settled deadline observation. It does not
choose whether a later configuration edit is accepted on a Finished state or
whether an independent clock observation is committed when an edit is refused.
Concrete server command/clock commit policy remains an explicit integration gate;
there is no hidden clock-only authoritative write in this pure proposal boundary.

## Unanswered owner decisions and dependent examples

The coordinator has asked the owner these questions. No answer has been recorded
at this authored revision; elapsed waiting time is not an answer. Alternatives
below are proposals for clarification, not enabled options or chosen defaults.

| Anchor | Genuine question and alternatives | Dependent example still without an expected command result |
| --- | --- | --- |
| OPEN-CAP | May a cap edit below current Main be accepted while retaining Main, must the edit be refused, or is an explicit separately audited time reduction required/available? CORE-005's addition behavior does not decide this edit | Current Main100/debt5/cap120, requested cap60: no selected after-state, rejection rule or implicit -40 effect |
| OPEN-READY | Does an enabled hard deadline affect Ready before first start, and must first start with an already-passed deadline refuse until change/disable or follow another explicit policy? CORE-002's Ready rejection is only its narrow API precondition | Ready, enabled deadline1000, UTC1000; later Start: lifecycle/admission result pending, coordinated with API-007 |
| OPEN-DEADLINE | May an authorized user enable/move a deadline to UTC <= the current observation, causing immediate finish, or must that edit be rejected? Enabled-deadline precedence does not itself choose input validation | Active/no deadline or future deadline, UTC1000, requested deadline999 or1000: no selected acceptance/rejection oracle |

Ready/start is not silently removed from release scope. Its current settings edits,
first-start observation/admission, passed-deadline validation and interaction with
setup validation need actual resolved examples with API-007. Finished/reopen and
absolute Main/debt corrections likewise need their explicit owning contracts;
ordinary deltas cannot silently substitute for those operations. Any additional
behavior question discovered during concrete design must be recorded and resolved
rather than given a speculative Active-only product default.

## Next gates and compatibility

Obtain actual separate document review of this revision. Incorporate the owner's
recorded answers without rewriting the existing numeric/countdown contracts, then
review the changed examples. Only after affected choices are resolved may their
minimal C++ boundary/stubs and specification-linked tests be authored/reviewed and
an intended missing-behavior red observed. An empty ready queue or existing core
passes does not establish this baseline or product behavior.

No existing header, ABI, schema, state token or assertion changes in this stage.
The future server adapter retains command/settings before-after provenance, current
access, actual revision CAS and exact applied effects atomically; independent code
review/acceptance and delivery/recovery/platform gates remain. All seven integrations,
three FX providers, renderers/UI/localization/themes and full release scope remain.
