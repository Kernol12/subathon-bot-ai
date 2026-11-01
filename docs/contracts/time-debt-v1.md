# Refund deficit and separate time debt — revision 1

Tasks: CORETEST-005 / CORE-005. Authority: specification v0.45 §4 Refund overflow,
NUM-02/03/04, and [event/state boundaries revision1](event-state-boundaries-v1.md).
This is a new internal pure numeric boundary. Existing
[time-credit](time-credit-v0.md), [timer lifecycle](timer-lifecycle-v1.md) and
[exact-decimal](exact-decimal-v1.md) contracts remain unchanged.

## Scope and preserved records

`core/include/subathon/core/time_debt.hpp` receives an already-calculated reversal
or eligible addition in whole milliseconds. It returns an owned proposal, never
authoritative state, persistence or an entitlement calculation. ACCT-006 owns
proportional cumulative partial refunds, original capped awards/rates and prior
reversal accounting. These functions never revalue or recompute an original award.

`TimeBalance` holds independent nonnegative signed64 `main_remaining_ms` and
`time_debt_ms`. Both may be positive: reopening preserves debt by default while
allowing an explicitly chosen Main value (§4/TIMER-10). No requirement to silently
net existing Main against debt is introduced. Only future eligible additions pay
debt first. A later policy selection cannot clear existing debt; a separate
authorized audited correction remains necessary.

Requests contain owned opaque references to caller-retained records. Reversal
requests preserve `reversal_ref`, `original_effect_ref` and the full nonpositive
`requested_adjustment_ms`; additions preserve `addition_ref` and nonnegative
`requested_ms`. Empty references reject as a local C++ precondition. These references
are not provider identifiers, deduplication proof or a finalized ledger schema.
Every returned request is an owned unchanged copy. The caller must retain and
resolve the original full record, including valuation/rule/application provenance.
No original record is deleted when timer subtraction is clamped or held.

## Reversal policy

`plan_time_reversal(balance,request,policy)` takes a signed nonpositive adjustment.
Zero is a valid no-op that still preserves its supplied record. `INT64_MIN` is also
representable request data: do not negate it in signed64 or reject it merely because
its positive magnitude cannot fit signed64. This primitive does not decide whether
the upstream entitlement permits that amount.

For an adjustment that fits within existing Main, every policy subtracts it fully,
retains debt and returns Applied. Exact exhaustion is not a deficit. For an amount
exceeding Main:

| Policy | Proposal |
| --- | --- |
| StopAtZero | Main0, existing debt retained; report actual Main subtraction and the full remaining signed unapplied amount |
| CarryTimeDebt | Main0; add excess reversal magnitude to separate debt and report both actual components |
| RequireReview | Hold the entire adjustment: both balance values unchanged, actual components0, full signed request unapplied, disposition RequiresReview |

RequireReview applies to a deficit, not every refund. This interpretation follows
the specification's **Refund overflow** heading, §18's "Refund exceeds remaining
time" scenario and this task's explicit deficit-policy scope. Equality and zero do
not require a new review hold. The setting has no invented default; callers must
explicitly supply one of the three policies. Authorized per-case overrides, audit,
review resolution and retry identity belong to the application layer.

Applied means this numeric proposal is not held by the deficit policy; it does not
set a contribution application/review status, grant permission or clear another hold.
All results retain the complete request and selected policy. Actual Main adjustment
and unapplied adjustment are nonpositive; debt addition is nonnegative. Conceptually
in exact arithmetic:

`requested_adjustment = main_adjustment - debt_added + unapplied_adjustment`.

Do not evaluate that identity using overflowing signed intermediates. There is no
partial proposal if the final debt would exceed signed64. The existing request
remains caller-owned for review; original entitlement/history must still be retained.

## Additions pay debt before caps

`plan_time_addition(balance,request,optional_cap)` partitions the nonnegative request
into `debt_repaid_ms`, `main_added_ms` and `cap_limited_ms`. Repay existing debt first.
Only the remainder can increase Main, subject to the optional Main cap. A full/zero
cap does not prevent debt repayment. A zero request changes neither balance.
The three nonnegative components sum exactly to the supplied request.

This boundary reports Main and debt effects separately; it does not invent a single
"applied time" ledger meaning or choose a later reversal allocation. Application
accounting must preserve both actual components and their source references, under
ACCT-004/006. Main time and debt never become negative, and no binary floating-point
value participates. Caps intentionally limit actual Main credit; they do not clamp
or erase debt or hide arithmetic overflow.

## Local validation and arithmetic errors

Invalid negative balance values, positive reversal adjustments, negative additions,
empty references, enum values outside the three policies, or a negative supplied cap
throw `std::invalid_argument`. Validate those conditions before generating any
arithmetic proposal. Exception text is not contractual.

For this new numeric boundary, Main at or above a supplied cap has zero headroom:
retain existing Main, still repay debt first, then cap-limit the entire remainder.
Never reduce existing Main as a side effect of an addition. This supports the brief
"debt before time/cap application" without deciding how a real cap edit is accepted
or whether an authorized correction changes existing time; CORE-007/API-007 owns
those edits. The older `calculate_time_credit` helper retains its existing above-cap
rejection precondition unchanged. The two interfaces have different explicit input
scopes; this function does not broaden that older contract.

If an otherwise valid operation would produce Main or debt above `INT64_MAX`, throw
`std::overflow_error` and return no partial proposal. NUM-02/04 requires the caller
to hold the affected input for review and atomically commit no effects/jobs from it.
The exception does not itself persist a review record. An overflowing hypothetical
Main+request is not grounds for rejection when debt repayment/caps make the actual
result fit. The input objects remain unchanged on success or failure.

## Lifecycle, Grace and atomic caller composition

The numeric API deliberately admits no lifecycle restriction: it has no Ready,
Active or Finished field to reinterpret. It supplies no permission to start/reopen,
apply post-finish support, clear a pause or bypass a hard end. Callers select the
current eligible operation and advanced authoritative snapshot under existing
CORE-002/API/accounting rules. Late refund/history/reopen support is not discarded
by an invented Active-only numeric precondition.

Grace is a separate nonnegative countdown, never a debt balance. Pass only Main
remaining into `TimeBalance`; never substitute Grace or subtract a reversal from it.
For eligible Active additions, give CORE-002's `apply_applicable_time_credit` only
`main_added_ms`, not the requested amount or debt repayment. Thus exact debt-only
repayment cannot rescue Grace/confirmation wait; a positive Main remainder can,
while preserving the retained Grace budget. Pause reasons remain external.

After a reversal exhausts Main, compose the returned Main/debt with the existing
timer snapshot and immediately run the existing zero-elapsed CORE-002 observation
at the same command instant. It selects Immediate, Grace or Confirmation, including
paused-zero behavior, shared/fresh Grace and hard-end precedence. This primitive
does not duplicate that state machine or invent event timestamps. The application
must commit the chosen policy, full request, actual Main/debt changes, transitions
and review disposition atomically; this library supplies no such transaction.

## Independently derived examples

All numbers below are literal integer arithmetic/requirement examples, not values
generated by the implementation under test. Units are milliseconds.

| Input | Required proposal |
| --- | --- |
| Main100/debt7, reversal-150, StopAtZero | Main0/debt7; Main-100, debt+0, unapplied-50, Applied |
| Main100/debt7, reversal-150, CarryTimeDebt | Main0/debt57; Main-100, debt+50, unapplied0 |
| Main100/debt7, reversal-150, RequireReview | Main100/debt7 unchanged; actual0, full-150 held |
| Main100/debt7, reversal-100, any policy | Main0/debt7, full subtraction, no review hold |
| Main0/debt0, reversalINT64_MIN, StopAtZero | Full signed request retained; Main0/debt0, unappliedINT64_MIN |
| Main1/debt0, reversalINT64_MIN, CarryTimeDebt | Main0/debtINT64_MAX, Main-1/debt+INT64_MAX |
| Main0/debt0, reversalINT64_MIN, CarryTimeDebt | Overflow, no proposal |
| Main2/debtINT64_MAX, reversal-3, CarryTimeDebt | Overflow, no partial Main subtraction |
| Main55/debt2, addition10/cap60 | Debt repaid2, Main added5, capped3; Main60/debt0 |
| Main60/debt7, addition5/cap60 | Debt repaid5, Main added0/capped0; Main60/debt2 |
| Main100/debt5, addition8/cap60 | Existing Main100 retained; debt repaid5, Main added0/capped3, debt0 |
| Main0/debt5, addition5/cap0 | Exact debt repayment; Main0/debt0, no Grace/wait rescue |
| Main0/debt5, addition7/no cap | Debt repaid5, Main added2; eligible Active Grace can resume Main2 |
| MainINT64_MAX/debtINT64_MAX, additionINT64_MAX/no cap | Debt fully repaid, Main unchangedINT64_MAX, no overflowing Main+request |

Primary executable home: `tests/acceptance/time_debt_tests.cpp`, GoogleTest/CTest,
with literal expectations and focused composition of existing CORE-002 proposals.
No clocks, Qt, SQL, network, provider payloads, real supporter data or new dependency
enter the numeric target. Initial source functions are unconditional
`TimeDebtNotImplemented` stubs. Independent review of actual artifacts must precede
the real intended red run; CORE-005 implementation waits for that gate. Stable
automatic acceptance is required when CORE-005 replaces the stubs. Persistence,
refund entitlement, permission, deployment/live/platform and full application
integration remain explicit downstream/release requirements.
