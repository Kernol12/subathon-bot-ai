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
These details still require the separate actual source/config approval below.

Author and root configure/build `sqlite-work-debug` and full CTest pass53/53:
new24, unchangedold25, scaffold4 including Qt probe. Reviewer independently reported
new24/full53 passes, but durable code/config approval is still pending. Root checked
all7 reviewed artifact hashes unchanged fromc60. Linux x86-64, GNU16.2.1,
Qt6.12.0, CMake4.4.5, Ninja1.13.2, Python3.14.8. No new dependency. Author honestly
records an initial wrong test-selection regex; it selected no cases and establishes
no red/pass. Corrected actual24 and full53 executions pass. No fixture was weakened.

[CI assessment](storage-010-ci.md): stable job/archive/action pins and exact
compiled registration guards remain. New24 now run unconditionally on PR/main/
manual with normal failures. Independent source/config review and observed actual
Ubuntu acceptance/latest-head checks remain required before done/merge. Current
root command is complete; no owner question. Root owns shared CMake/presets/
workflow/state/HTML/map/checkpoint; source author and reviewer have isolated
storage010-implementation/review worktrees and separate evidence ownership.

No application schema/migration/real repository adapter, domain encoding, effect/
aggregate/job/application bundle, recovery authorization, hard execution deadline,
power-loss/live provider or supported-platform release verification is supplied.
All seven integrations, three rate providers and full release scope remain. After
merge refresh main and select STORAGETEST-011 with its own contract/test/red gate.
