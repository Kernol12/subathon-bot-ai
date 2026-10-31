# ACCTTEST-002 reviewed/red contribution baseline

Coordinator `/root`, 2026-10-31, branch `task/ACCTTEST-002-eligibility-baseline`,
[PR #17](https://github.com/Kernol12/subathon-bot-ai/pull/17). Base main8ad458e;
specification v0.45 §§3/5/14/18, contribution eligibility/linkage revision1.

Author `/root/receipt_test_author` supplied the21-case candidateb0fd97e. Independent
`/root/baseline_reviewer` reviewed it and the optional harness in9dd0fec, requesting
two focused corrections. Author929152c made late matching support conditions differ
while retaining original-condition assertions, and added two contract-declared invalid
observation shapes. The separate reviewer approved those actual corrections and the
full baseline ina561508; [review](accttest-002-independent-review.md) retains both
findings and their resolution. No original assertion was deleted or weakened.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| Core interface | `65ad81ac0dc3fc808421258ab89b30ff610be085` |
| Core explicit stub | `fb140178ba0adf2f11ac37f8a311977217ebc9c5` |
| Server interface/port | `82c2e5f7315b3f2a9ab15fdec87104f6456c970a` |
| Server explicit stub | `1af4468cbc94ea5f8e85d23b00c64293589486ad` |
| 21 corrected cases | `702a7bf1d8162ba748ee9477ea48ffa73047ef65` |
| Primitive fake | `8d35e502a9656d0e65555efb48b3bd8650bd0c5e` |
| Contract before status-only completion prose | `5eec5921c355331bb97d351b7e20ad00873dae7a` |

After approval was integrated at4d1400f, coordinator configure/build of
`contribution-evidence-debug` exited0, then actual
`ctest --preset contribution-evidence-debug -L contribution-evidence` exited8:
all21 bodies fail on explicit core/service missing-behavior exceptions.
[Actual output](accttest-002-red-ctest.txt) preserves the failures. No skips,
dependency/compile failures, inversion or continuation-to-green were counted as red.
Author/reviewer had independently compiled/listed without executing bodies first.
Environment: Linux x86-64, GCC16.2.1, CMake4.4.5, GoogleTest1.18.0, Python3.14.8.

Coordinator configure/build/CTest regressions: scaffold3/3, credit23/23,
numeric33/33, receipt15/15 pass using their existing presets/labels. Final tooling,
status integration review and latest PR CI/merge remain separately checked gates.
The earlier [usage checkpoint](accttest-002-checkpoint.md) is historical; its
unavailable-review/no-red state is superseded by this actual evidence.

Baseline completion supplies reviewed tests and intended red, not eligibility or
linkage behavior. ACCT-002 must preserve the baseline, implement both pieces,
obtain separate code review and add/observe stable automatic contribution CI.
The primitive fake supplies only transaction storage and known-uncommitted rollback;
actual provider correlation, SQL uniqueness/durability/concurrency, Qt writer ownership,
application/effects/jobs, refund reconciliation and release/platform/live gates remain.
Default fixtures are synthetic; no credentials, supporter data or live operation used.
