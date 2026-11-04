# Individual permission override scope — owner decision

Recorded by coordinator `/root` on 2026-11-04 for ACCESSTEST-002/ACCESS-002,
[PR62](https://github.com/Kernol12/subathon-bot-ai/pull/62), specification v0.45
§10/AUTH-06.

The owner requested more context about an individual five-minute allowance on
timer A when a role permits ten minutes on every timer. The coordinator explained
the A/B outcomes for resource-scoped, whole-operation and complete-request-fallback
policies, recommending the resource-scoped option. The owner then explicitly said:
“I accept the recommendation for these.” This resolves the remaining D1 question.

For an operation, individual overrides match the request's resource bindings
before quantity and other limits are evaluated. A matching individual scope
selects that permission tier. A request exceeding its limit is denied without
falling back to a broader role grant. Other resources retain normal role rules.

| Example | Accepted outcome |
| --- | --- |
| Individual A/max5; role any timer/max10; request A/+10 | Denied by the individual limit; no role fallback |
| Same grants; request A/+5 | Allowed by the complete individual grant |
| Same grants; request B/+10 | Allowed by the complete role grant |

All examples concern one operation/request, not a cumulative time budget.
Whole-grant matching, individual/role deny precedence, default denial, protected
ownership and execution-time rights remain required. Concrete matching shapes,
consistent deny cases and independent examples still need actual logical review.

The three separate timer decisions remain as recorded in
[timer editorial owner decisions](timer-editorial-owner-decisions.md), including
the per-change Keep / Reduce / Abort warning. This acceptance does not replace
that requested warning with a global cap policy.

This records the selected product behavior. It is not API/test approval, an
implementation, an observed red result or permission to merge unfinished work.
