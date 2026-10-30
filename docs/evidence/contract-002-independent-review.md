# Independent review — CONTRACT-002

Author: `/root/domain_contract`; reviewer: `/root/independent_reviewer`.
Date: 2026-10-30. Verdict: **approved for integration**.

Reviewed exact authored commit `1904ccb21dc05ccdc08f33d745dd99bba8995305`,
based on main `2bc4e6420ac7c56ce9fb29bc678798d2987ae8df`:

| Document | Reviewed Git blob |
| --- | --- |
| [Internal boundary contract revision 1](../contracts/event-state-boundaries-v1.md) | `8b60da43f1362c46c54730e8ca029b858964ea59` |
| [Task brief](../tasks/CONTRACT-002.md) | `c5b4d6bf094f440251302318c52e7a4e140c3723` |
| [Authoring evidence](contract-002-authoring.md) | `da2d6b296a367042313fc301442135b04670fe6f` |

## Scope and specification assessment

Read the complete authored contract, brief and handoff against specification v0.45
§§2–5,8,18 and relevant DB-01/SERVER-01/NUM-01–04/TIMER-01–10/MILE/OPS-04
requirements. Existing time-credit revision 1 is preserved. No blocking requirement
conflict or unsupported capability claim was found.

The contract preserves distinct receipt/event/contribution/plan/effect/job records,
independent evidence/valuation/review holds, durable arrival context and current-state
cap/debt/lifecycle application. It requires original-effect/rate/rule provenance and
requested versus actual credit for reversals, without turning a recorded entitlement
into unconditional timer subtraction. Completed milestones and performed actions
remain recorded. Unknown provider fields stay unknown; identifiers and duplicate-safety
claims require provider evidence and correctly scoped source identity.

The single owned Qt queue/writer thread is identified as a local engineering choice
within the agreed architecture. Core remains free of Qt/network/SQL dependencies;
Drogon, connectors and delivery callbacks enqueue owned values. Separate intake and
atomic application commits preserve durable pending inputs on failed application,
require identity/uniqueness checks on retry and publish only committed results.
Required effects/state/application/jobs are committed together, while uncertain remote
sending cannot be mistaken for local exactly-once execution or blindly replayed.

Monotonic elapsed progression and separate UTC instants are explicit clock-domain
choices. Recovery does not persist process-local ticks as portable timestamps or
subtract incompatible clocks. Checkpoint Freeze/Count elapsed/Review behavior,
independent pauses, hard-end precedence, review-wait freezing, separate debt/grace,
schedule timezone/DST behavior and reopening safeguards agree with the specification.
Concrete clock-discontinuity algorithms and limits remain assigned to downstream
reviewed baselines rather than silently choosing unsupported product behavior.

## Examples and boundary checks

Manually checked all fourteen examples against their cited requirements; expected
values do not come from an implementation. In particular:

- B04: 58 minutes leaves 2 minutes of capacity at cap60, so requested600000ms
  produces actual120000ms and remaining3600000ms after revalidation.
- B05: one-quarter of actual300000ms/40000000 micro-points is75000/10000000;
  full remaining refund reverses225000/30000000 against the original provenance.
- B06: cumulative1/3,2/3,full of1 micro-point rounds half-even to0,1,1;
  increments0,1,0 preserve exact total and whole-count handling waits for full refund.
- B08/B10: failed transaction/overflow cannot leak a partial effect or required job.
- B11/B12: five-minute gap from600000ms yields300000 under Count elapsed and600000
  under Freeze; waiting for Recovery review remains paused and does not clear Manual.
- B13/B14 retain hard-end precedence and specified DST defaults/alternatives.

The examples are contract examples only, not executable tests or observed red/pass
feature evidence. No test-first stage is bypassed: each production slice still needs
its independently reviewed executable baseline and actual intended red before code.
The task does not finalize wire formats, SQL migrations or all domain reducers; those
are deliberately assigned to dependent bounded baselines. All release scope remains.

## Independent commands and CI assessment

In isolated reviewer worktree `/tmp/subathon-coordinator-review`, branch
`task/coordinator-independent-review`, cherry-picked exact author docs as `343ea60`.
No shared progress or HTML was edited by the reviewer.

| Command | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | exit0;377 records,122 decisions, local links/presets/current HTML pass |
| `python3 tools/progress.py render --check` | exit0; current snapshot |
| `python3 -m unittest discover -s tests/tooling -v` | exit0;19/19 tooling cases pass |
| `git diff --check` | exit0 |
| `git hash-object docs/contracts/event-state-boundaries-v1.md docs/tasks/CONTRACT-002.md docs/evidence/contract-002-authoring.md` | exit0; exact reviewed blobs above |

Documentation-only changes introduce no executable suite, dependency or workflow;
existing project/link/progress and reviewed credit CI are appropriate. Downstream
new stable suites must enter CI under the evolving-CI policy. Coordinator must still
check integrated progress/HTML, current-main compatibility and latest PR CI/reviews/
merge gates. This review does not replace a required GitHub approval if rules change.

Also inspected the coordinator's integration navigation change in docs/ARCHITECTURE.md
(blob `101323f`; observed in `/tmp/subathon-contract002` before its integration commit).
It accurately links revision1, distinguishes internal invariants from unfinished
C++/SQL/wire layouts, and preserves per-slice reviewed-test/red requirements. This
paragraph is approved for integration; the approval does not cover unseen later edits.

No production clock/persistence/adapter, Qt server bridge, platform/renderer/UI,
packaging, OBS or live service was verified here. No credentials or supporter data
were used. Concrete baseline/implementation and live/platform gates remain open.
