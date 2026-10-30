# Time-credit contract — revision 1

State: specified for TEST-001; independent REVIEW-001 and observed red TEST-002 remain
required before implementation. The specification's
§3 actual effects, §4 cap behavior and §18 "55-minute timer, 60-minute cap, requested +10"
scenario are authoritative. This contract does not decide other timer policies.

Public boundary: core/include/subathon/core/time_credit.hpp.
All time values are signed 64-bit milliseconds. This boundary plans nonnegative
credit for an eligible active main timer with no outstanding debt and no hard-end
conflict. Zero remaining time is a numeric boundary, not permission to reopen or
select a lifecycle transition. The six original valid examples remain unchanged.
`maximum_remaining_ms` is an optional configured remaining-time cap.

| Input | Expected output |
| --- | --- |
| Remaining 55 min, request 10 min, cap 60 min | Requested 10 min, applied 5 min, remaining 60 min |
| Remaining 20 min, request 10 min, cap 60 min | Requested/applied 10 min, remaining 30 min |
| Remaining 60 min, request 10 min, cap 60 min | Applied 0, remaining 60 min |
| Remaining 55 min, request 10 min, no cap | Applied 10 min, remaining 65 min |
| Remaining 55 min, request 5 min, cap 60 min | Applied 5 min, remaining 60 min |
| Remaining 55 min, request 0, cap 60 min | Applied 0, remaining 55 min |

## Validation and result

The following error API choices are local C++ contract decisions under CONTRACT-001;
the specification defines the accounting behavior, not the exception classes.

- Negative `remaining_ms`, negative `requested_ms`, or a negative configured cap
  throws `std::invalid_argument`. Signed corrections/refunds are valid product
  operations, but use a separate boundary; negative remaining time cannot encode debt.
- A supplied cap of zero is accepted: with zero remaining time there is no capacity.
- `remaining_ms` above a supplied cap throws `std::invalid_argument`, including for
  a zero award. This rejects a state outside this narrow boundary. It does **not**
  define what lowering the configured cap does to existing time; that product
  interaction remains a question for the later configuration/timer contract.
- For valid inputs, `requested_ms` is preserved exactly in the result. `applied_ms`
  is the request limited by available headroom when a cap is supplied, otherwise
  the whole request. `remaining_after_ms` is current time plus actual applied credit.
- If that final sum exceeds signed 64-bit range, throw `std::overflow_error` and
  return no credit proposal. The future caller must hold the input for review and
  commit no effects (§3.5, NUM-02/04). Do not wrap, saturate, or silently truncate.
- Cap limiting is an intended actual-effect rule, not arithmetic overflow. A large
  representable request whose uncapped sum would exceed the range still succeeds
  when the cap makes the actual final sum representable. Never evaluate an
  overflowing signed intermediate; use checked or exact arithmetic as appropriate.
- Validate the narrow-boundary input conditions before considering overflow.
  Error-message wording is not part of the acceptance contract.

The function only proposes credit; the future application transaction commits actual
effects. Reversals later use the actual credited amount, never the originally requested
amount. This boundary does not implement that transaction or a refund function.

## Responsibilities outside this boundary

The caller owns eligibility/review, lifecycle and phase, independent pause reasons,
debt-first additions, hard-end precedence and authoritative current-state selection.
This function receives none of those values and cannot enforce them. It must not be
treated as a direct contribution-to-state path. A future service/transaction contract
must cover those interactions, above-cap configuration changes, requested-versus-
actual effect provenance and reversals. §4 and TIMER-01/06/09/10, NUM-03 remain in
release scope. This contract supplies no lifecycle, debt, refund, persistence or
rounding implementation; its inputs are already whole milliseconds.

## Revision and compatibility

Revision 0 supplied only the six nonnegative examples. Revision 1 preserves their
values and public input/result layouts, defines validation/error signaling, and adds
boundary examples in [the contract evidence/test plan](../evidence/time-credit-contract.md).
There are no implemented downstream callers or persisted schema changes. TEST-001,
REVIEW-001, TEST-002 and later IMPL-001 must use this revision together. An independent
review must assess the local API decisions before this becomes an implementation
baseline; no separate review is claimed by the contract author.

Implementation status: explicit FeatureNotImplemented stub. Keep progress evidence
authoritative for authored, reviewed and observed-red status.
