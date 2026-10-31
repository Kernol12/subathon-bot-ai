# CORETEST-002 reviewed/red timer baseline

Coordinator `/root`, 2026-10-31; branch `task/CORETEST-002-timer-baseline`,
[PR #14](https://github.com/Kernol12/subathon-bot-ai/pull/14). Base main8ad458e;
specification v0.45 §4/§18, TIMER-01/02, timer contract revision1.

Original author68e0119/3ccc52d and correctionef82bbcf supplied35 cases. Owner
explicitly selected [automatic confirmation-wait resumption](timer-confirmation-credit-decision.md).
Separate author `/root/timer_policy_author` added four cases in160c7e6, preserving
all35 prior bodies. Independent `/root/timer_final_reviewer` examined all39 cases,
contract, interfaces, stubs and optional harness and approved in2fb544f;
[full review](coretest-002-final-independent-review.md). Earlier partial review
findings remain historical; the full review closes them.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| 39 acceptance cases | `bf8c1f46d2fe8e6fdbd1ce2ed5841e679dc4a1a7` |
| Contract revision1 before status-only completion prose | `09c7c77d2e42b6cc5971f8bff8962c0486a84082` |
| Public header | `69cad7023b9928371877d45955b800aed9996cd5` |
| Three explicit stubs | `5584f845faef1dff68377363ec04c7b2effc4cae` |

After approval was integrated atcd59601, the coordinator ran
`ctest --preset timer-lifecycle-debug -L timer-lifecycle`: actual exit8, all39
bodies failed on `TimerLifecycleNotImplemented` (including wrong exception types
in validation cases). [Actual output](coretest-002-red-ctest.txt) retains the result.
No skips, dependency/compile failures, failure inversion or continuation-to-green
were used. Configure/build had already exited0 on the exact39-case revision;
author and reviewer also independently configured/built and listed39 without bodies.
Environment: Linux x86-64, GCC16.2.1, CMake4.4.5, GoogleTest1.18.0, Python3.14.8.

Coordinator regressions on this integrated behavior/harness: configure/build/CTest
`scaffold-debug`3/3, `acceptance-debug -L 'acceptance|regression'`23/23,
`exact-numeric-debug -L exact-numeric`33/33, and
`receipt-intake-debug -L receipt-intake`15/15 all exit0. Python tooling19/19 and
the progress DOM simulation also pass. Final status/project/render/diff and PR
latest-revision CI/merge checks remain separate recorded integration gates.

This completes a reviewed failing baseline, not timer implementation. Intentional
red stays separate from stable automatic scaffold/credit/numeric/receipt workflows.
CORE-002 must preserve the reviewed artifacts, implement this pure reducer, obtain
separate code review, pass39 cases and add/observe automatic timer PR/main CI.
No Qt queue, durable state, permission, actual-effect transaction, pause-reason set,
start/reopen, recovery, schedule/DST or live/provider/platform behavior is established.
Those existing downstream tasks and the full release scope remain required.
