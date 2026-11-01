# STORAGE-001 bounded Qt SQL writer implementation

Coordinator `/root`, implementer `/root/storage_implementer`, 2026-11-01.
Task branch `task/STORAGE-001-sqlite-writer`, from integrated mainc2ca25a after
[PR24](https://github.com/Kernol12/subathon-bot-ai/pull/24) merged all10checks.
Specification v0.45 §2/DB-01; [writer contract revision1](../contracts/sqlite-writer-v1.md).

Reviewed prerequisites remain unchanged: contract4d280461, header11d204aa and
22Qt test slotsf8418e83; separate baseline review3e7c8c7/finalb9d84ae and actual
22-case intended stub-red at6647490(exit8). [Baseline handoff](storagetest-001-handoff.md).
No assertion/interface/fixture is changed to make implementation pass.

Sourceauthorf2a6379 (integrated5562eaa), blob
`a8e5c8e80ebb9bde8321d01f7fbc1b1d826c2811`, implements private owner-thread QtSQL
connections, mutex-protected canonical writer lease, WAL/FULL/fkON/busy_timeout0,
owned FIFO batches, subtraction-based payload/result budgets, BEGINIMMEDIATE/
COMMIT with rollback/fail-closed handling, post-transaction exception-contained
callbacks and terminal drain-before-close. Separate readers use actual read-only
open flags plus query_only. No second authoritative thread or SQL handle escape.
[Author evidence](storage-001-implementation.md).

Root actually ran `cmake --preset sqlite-writer-debug`, build,
`ctest --preset sqlite-writer-debug -L sqlite-writer` and full preset: all exited0,
22/22 and25/25 respectively, with Qt6.12.0/GNU16.2.1/CMake4.4.5/Ninja1.13.2/
Python3.14.8. Author independently recorded the same pass counts.
Code/config review on actual850eef4 is in progress in a separate worktree.
[CI assessment](storage-001-ci.md) tracks ordinary automatic acceptance promotion;
actual Ubuntu execution, separate approval and latest-head integration gates remain.

Scope is the bounded writer/reader library with trusted internal SQL, not a running
server application. Active-count/handle lifetime/fatal rollback/callback destruction
paths require explicit code review beyond22 direct oracles. There is no hard runtime
deadline for arbitrary SQL. Application schema/migration, repository adapters,
effects/aggregate/job transactions, crash/power-loss/backup/recovery, transport,
provider/platform/deployment/OBS remain separate tasks. No release scope is removed.

Root owns build/labels/CI/map/progress/checkpoint/PR integration; support worktrees
and completed review history remain local and preserved. Final task status waits
for code approval and actual ordinary acceptance CI; no merge is claimed here.

## Independent approval and actual automatic acceptance

Separate reviewer `/root/storage_code_reviewer` approved actual source/config
candidate850eef4 ind6f9c49 (integrated422acfd), after independently running full
preset25/25 and inspecting beyond direct assertions. The review covers active
capacity through transaction completion, registry synchronization, bounded counters,
private SQL lifetimes, rollback/fatal-close pending callbacks and no implementation
member access after user callbacks. No source/assertion change was requested.
[Exact review](storage-001-code-review.md).

Actual promoted UbuntuCI41289246502 atPR25headed5da87 passed all22 unchanged real
SQLite cases plus environmentprobe1; Qt/ICU hashesOK and GNU13.3.0 observed.
All10checks passed. This completes bounded implementation/code/acceptance gates;
final completion metadata review, latest-head CI and normal merge rules remain.
[PR25](https://github.com/Kernol12/subathon-bot-ai/pull/25) carries publication.
Project/currentHTML/diff checks and actionlint pass; no redundant broader native
runs are claimed beyond the actual author/root/reviewer checks recorded above.

## Premerge ownership finding — approval held

A separate reviewer probe identified that plain Qt value-copy retained raw-data
backing for accepted SQL/QString/QByteArray inputs. Sourcea8e5c8e therefore did not
fully satisfy the existing owned queue promise despite the22 original cases.
Review finding9623943 holds the affected ownership approval; PR25 is draft and
STORAGE-001 in_review. Existing22 assertions and valid review/pass evidence remain.

A different test author supplied3 focused regressions in a separate file; their
actual oracles/lifetimes and new executable/CI registration require independent
approval before intended red. Production normalization must wait for that reviewed
red, then pass original22+new3 with separate code review and actual UbuntuCI.
This enforces the existing contract rather than imposing a caller lifetime shortcut.
No new test body/red or production fix is claimed in this entry.


New regression34c25c27 was independently approved94820fa, integratedf79d002,
before actual `ctest --preset sqlite-writer-debug -R '^sqlite-writer-regression\.'`.
Observed exit8: all3 body failures on unfixed sourcea8e5c8e: SQLreturned9vs1,
textzbcvsabc, blob7A6263vs616263; every fixture/init/cleanup passed, zero skips.
[Retained actual red](storage-001-ownership-red.txt). This completes the additional
reviewed/red gate; the targeted admission ownership fix may now proceed without
changing the original22/header/contract or the new3 assertions.
