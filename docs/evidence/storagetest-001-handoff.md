# STORAGETEST-001 reviewed/red writer baseline

Coordinator `/root`, 2026-11-01, branch `task/STORAGETEST-001-writer-baseline`,
based on integrated mainba4e94c. Specification v0.45 §2/DB-01 and internal
[writer contract revision1](../contracts/sqlite-writer-v1.md).

Author `/root/storage_test_author` supplieda0838ff/83b6c34/819edbb. Separate
`/root/storage_test_reviewer` examined integrated candidate48db98c, all22 oracles,
interface/stubs and build/CI harness, independently compiled/discovered22, then
approved in3e7c8c7. Approval was integrated at6647490 before any behavioral run.
[Independent review](storagetest-001-review.md); [author evidence](storagetest-001-author.md).

| Artifact | Exact Git blob |
| --- | --- |
| Contract revision1 | `4d280461966f3fd9fcb13ab6d054c88228ca05b8` |
| Public interface | `11d204aab3340bc670df08716ae1c80cae0c337f` |
| Explicit stubs | `f0edd7e6c504c152f6412b186b80c5d9485e1ba3` |
| 22 Qt Test slots | `f8418e83dbd88d74505c047e6b3e2c30304ade69` |

After approval integration6647490, root ran
`ctest --preset sqlite-writer-debug -L sqlite-writer`: actual exit8,22/22 failed
on `StorageError::feature_not_implemented` (enum1) instead of the required success
or specified validation/error result. Every actual QSQLITE fixture/init succeeded;
zero skipped cases, compilation failures or dependency failures count as red.
[Full observed output](storagetest-001-red-ctest.txt) retains process exit8.
There was no failure inversion or assertion/fixture weakening.

Root `cmake --preset sqlite-writer-debug`, build and CTest `--show-only` succeeded;
after final busy-timeout/owned-input assertions, the test target rebuilt and all22
were discovered again before approval/red. Linux x86-64, GNU16.2.1, Qt6.12.0,
CMake4.4.5, Ninja1.13.2, Python3.14.8. The native scaffold3 and Qt environment
probe pass after the build changes. Project/progress/diff checks pass; final
integration/tooling and actual Ubuntu CI remain recorded below when performed.

The tests use real disposable files and independently observed literal SQL results.
They cover owner-thread rejection, effective WAL/FULL/foreign_keys/busy_timeout0,
owned queued inputs, FIFO committed visibility, canonical single-writer lease,
actual OPEN_READONLY readers, WAL snapshots, admission/resource boundaries,
statement/COMMIT rollback, busy no-replay, callback exception containment and
terminal drain-before-close lifecycle. Active-transaction count and private handle
ownership still need explicit implementation code review; arbitrary trusted SQL
has no hard runtime deadline. These limits do not shrink product scope.

[CI assessment](storagetest-001-ci.md): pinned official Qt archive build/probe/
discovery runs automatically; actual writer cases remain explicit intentional red
until STORAGE-001. The first GitHub attempt rejected the workflow before jobs due
to an unavailable runner context in job-level env; fixed setup/review/actual runner
results must be verified before merge. No passing writer behavior is claimed.

No application schema/migration, repository adapter, crash/power-loss durability,
backup/recovery, app-level effects/jobs/aggregates, transport/provider integration,
Windows or release/platform verification follows from this baseline.
STORAGE-001 must start from the integrated reviewed/red prerequisite, preserve the
22 cases/public boundary, implement only this writer/reader slice, obtain separate
code review and enable/observe ordinary automatic acceptance.

[PR24](https://github.com/Kernol12/subathon-bot-ai/pull/24) is currently draft;
final integration review, fixed-workflow actual Ubuntu run, all latest-head checks,
resolved discussions and normal merge rules remain publication gates.

## Baseline completion and publication

Actual corrected UbuntuCI41288299328 atda98505 passed: verified official Qt/ICU
archive hashes, GNU13.3.0, Qt event-loop1/1 and all22 compiled registrations.
All10PR checks passed. Final integration review0f4cf52 separately verified unchanged
baseline, actual red and CI logs. STORAGETEST-001 is done as reviewed/red baseline;
STORAGE-001 waits for this prerequisite merge. Narrow completion-metadata review
and final-head CI/normal rules remain publication gates. Existing valid numeric,
timer/pause/accounting tests are unchanged and remain automatically checked.

Actual PR24 mergedc2ca25a at09:23:31Z on2026-11-01. Finalheadf413511 had all10
checks passed, no review threads and current normal rules satisfied. No bypass or
GitHub approval waiver was used (rules require zero GitHub approvals). STORAGE-001
is now separately claimed from integrated prerequisite; preserve all baseline and
independent-review evidence above.
