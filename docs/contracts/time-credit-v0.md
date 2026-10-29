# Draft time-credit contract — revision 0

State: DRAFT, awaiting CONTRACT-001 / TEST-001 / REVIEW-001. The specification's
§3 actual effects, §4 cap behavior and §18 "55-minute timer, 60-minute cap, requested +10"
scenario are authoritative. This contract does not decide other timer policies.

Public boundary: core/include/subathon/core/time_credit.hpp.
All time values are signed 64-bit milliseconds. The six initial tests use nonnegative
valid inputs, no outstanding debt, active main timer, no hard-end conflict and no overflow.
`maximum_remaining_ms` is an optional configured remaining-time cap.

| Input | Expected output |
| --- | --- |
| Remaining 55 min, request 10 min, cap 60 min | Requested 10 min, applied 5 min, remaining 60 min |
| Remaining 20 min, request 10 min, cap 60 min | Requested/applied 10 min, remaining 30 min |
| Remaining 60 min, request 10 min, cap 60 min | Applied 0, remaining 60 min |
| Remaining 55 min, request 10 min, no cap | Applied 10 min, remaining 65 min |
| Remaining 55 min, request 5 min, cap 60 min | Applied 5 min, remaining 60 min |
| Remaining 55 min, request 0, cap 60 min | Applied 0, remaining 55 min |

The function only proposes credit; the future application transaction commits actual
effects. Reversals later use the actual credited amount, never the originally requested
amount. This boundary does not implement that transaction or a refund function.

Before implementing, reviewers must resolve the validation/error result contract for
negative input, existing time above a newly lowered cap, invalid cap and checked overflow,
and separate lifecycle/debt/hard-end handling rather than guessing. Add cases and revise
this contract if the boundary changes. No unchecked signed arithmetic is permitted.

Implementation status: explicit FeatureNotImplemented stub. Tests: authored draft, no
independent review or observed GoogleTest red run. Keep progress evidence authoritative.
