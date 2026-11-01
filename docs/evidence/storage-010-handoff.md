# STORAGE-010 — writer-owned synchronous transaction work

Coordinator `/root`, implementer `/root/debt_code_reviewer`, different code reviewer
`/root/storage_work_code_reviewer`, 2026-11-01. Task branch
`task/STORAGE-010-writer-work` from merged PR29/main c60e1ed.
Specification v0.45 §2/§3, DB-01/SERVER-01;
[work contract revision1](../contracts/writer-transactions-v1.md).

[STORAGETEST-010](storagetest-010-handoff.md) supplies the independently reviewed
new24/old25 baseline and actual24 missing-feature red at75c07fc AFTER approval.
It was not repeated. All tests/interfaces/contracts remain hash-identical:
new24 eb9e3b6, old22 f8418e83, old3 34c25c27, header afc9f483, fault a2cd7b89,
work contract4ba20506 and oldwriter contract4d280461.

Author source ad66ded737f60e66b6dae5bffc2989914011212e integrated2aa14cd;
source blob `023451987e40a9330fef91c6c0955f0e527f14e6`. Author evidence5025715
integrated3ed2362; [implementation evidence](storage-010-implementation.md).
Only server/src/sqlite_writer.cpp contains production changes. Root ce0a325
retires the baseline stub, points its historical review link to immutable Git
c60e1ed/b47f and makes unchanged new24 acceptance unconditional alongside old25.
No assertion/review conclusion or interface is rewritten by that link maintenance.

Private per-item weak state owns phase/first error/budgets; foreign operations only
inspect immutable owner identity before rejecting. Real synchronous BEGIN/execute/
COMMIT uses the same private connection. Known active failures synchronously roll
back; uncertainty overrides earlier failure and fail-closes; committed effects
survive later body/protocol errors. Cleanup is phase-aware/noexcept. One mixed FIFO
retains the active slot through body/finalization; completion can destroy the writer
without later Impl access. The test seam is below physically successful matching
control, before acknowledgement classification, never a fabricated final result.
Different source/config reviewer approved those actual details in9216d8d,
integrated17adb35; [code review](storage-010-code-review.md).

Author and root configure/build `sqlite-work-debug` and full CTest pass53/53:
new24, unchangedold25, scaffold4 including Qt probe. Reviewer independently reproduced new24/full53 and approved source023451/sharedce0a325
in9216d8d. Root checked
all7 reviewed artifact hashes unchanged fromc60. Linux x86-64, GNU16.2.1,
Qt6.12.0, CMake4.4.5, Ninja1.13.2, Python3.14.8. No new dependency. Author honestly
records an initial wrong test-selection regex; it selected no cases and establishes
no red/pass. Corrected actual24 and full53 executions pass. No fixture was weakened.

[CI assessment](storage-010-ci.md): stable job/archive/action pins and exact
compiled registration guards remain. New24 run unconditionally on PR/main/manual
with normal failures. Actual [Ubuntu41300728417](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41300728417)
at a1e3a89 passed hashes/GNU13.3/probe1/exact25+24 discovery/real25+24 acceptance;
all11 checks there succeeded. STORAGE-010 is done for this internal mechanism.
Final completion-metadata/CI attribution review, latest-head checks/discussions/
current normal rules remain before PR30 merge. Root owns shared build/state/
HTML/map/checkpoint; author and code reviewer use isolated separate worktrees.
No owner question or running root command remains.

No application schema/migration/real repository adapter, domain encoding, effect/
aggregate/job/application bundle, recovery authorization, hard execution deadline,
power-loss/live provider or supported-platform release verification is supplied.
All seven integrations, three rate providers and full release scope remain. After
merge refresh main and select STORAGETEST-011 with its own contract/test/red gate.

Final completion metadata and actual CI attribution at f370eeb independently
approved19e383b. Reviewer independently fetched the exacta1e3a89 run41300728417
metadata/logs and preserved391 task graph/executable artifacts. No native suite
was repeated. Latest publication-head CI/discussion/current-rule gates remain.
