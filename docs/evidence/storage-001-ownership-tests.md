# STORAGE-001 — owned admission regression tests

Author: `/root/storage_test_author`, 2026-11-01.
Branch `task/STORAGE-001-ownership-tests`, based on coordinator hold `5790bd5`.
Scope: strengthen the existing owned-input guarantee in
[SQLite writer contract revision1](../contracts/sqlite-writer-v1.md), §Admission,
and [event/state boundaries revision1](../contracts/event-state-boundaries-v1.md),
§Authoritative queue. Specification v0.45 §2/DB-01 remains authoritative.

## Reason and independently derived expectations

The separate [implementation review](storage-001-code-review.md) reported that
Qt implicitly shared copies of `QString::fromRawData`/`QByteArray::fromRawData`
retain borrowed external storage. Admission must snapshot command values rather
than allow later caller mutations to change already admitted SQL or bindings.
These tests exercise that existing guarantee; they add no new product restriction
or contract/interface revision. Author did not run the reviewer's temporary probe.

The new executable contains three focused cases:

| Slot | Input after successful admission, before dispatch | Literal expected result |
| --- | --- | --- |
| ownsRawBackedSqlAtAdmission | Raw QChar storage for SELECT1 becomes SELECT9 | Completed SQL returns exactly one result set, one row, integer1 |
| ownsRawBackedTextBindingAtAdmission | Raw QChar binding abc becomes zbc | Independent fixture SQL reads one persisted row with text abc |
| ownsRawBackedBlobBindingAtAdmission | Raw byte binding 616263 becomes7a6263 | Independent fixture SQL reads one persisted row, SQLite type blob, hex616263 |

Each backing array remains alive through Qt completion. A nested scope destroys
the caller's raw-data view and `WriteBatch` before event-loop dispatch. The test
also verifies that the caller view sees the mutation, so normal Qt copy-on-write
detachment cannot accidentally turn this into the old owned-container case.
No dangling memory, concurrent mutation, clocks, sleeps or implementation-generated
expected values are involved. `!completed` verifies that mutation precedes dispatch;
QTRY waits only after all mutation/view destruction is complete.

The fixture uses a disposable SQLite file, real Qt SQL, and a separate owner-thread
connection. Its SQL only creates a synthetic table and reads committed literals.
No helper in the fixture implements admission, snapshots or transaction policy.
Qt SQL/thread/connection-lifetime warnings are configured as test failures.

## Actual compile and discovery

Environment: GNU C++16.2.1, Qt Core/Test/Sql6.12.0, CMake4.4.5 and Ninja.
The coordinator owns the repository CMake target, CTest registration and CI guards.
Author used an untracked disposable CMake harness with C++20, AUTOMOC, the repository
`subathon_target_options`, actual writer source and Qt6::Core/Sql/Test.

Commands from this isolated worktree:

```sh
cmake -S /tmp/subathon-storage001-ownership-harness -B build/storage001-ownership-compile -G Ninja -DSUBATHON_SOURCE="$PWD"
cmake --build build/storage001-ownership-compile
build/storage001-ownership-compile/server_sqlite_writer_ownership_regression -functions
git diff --check
```

All exited0. The executable compiled and discovered exactly the three slots above.
No test body, CTest suite or intended red execution was run by this author. Separate
review of the actual candidate must precede the observed regression red and fix.

Unchanged reviewed artifacts were checked with `git hash-object`:

- Existing22 tests: `f8418e83dbd88d74505c047e6b3e2c30304ade69`.
- Header: `11d204aab3340bc670df08716ae1c80cae0c337f`.
- Contract: `4d280461966f3fd9fcb13ab6d054c88228ca05b8`.
- Unfixed source: `a8e5c8e80ebb9bde8321d01f7fbc1b1d826c2811`.

## Handoff and remaining gates

Only `tests/qt/sqlite_writer_ownership_tests.cpp` and this note were authored.
Coordinator owns progress/HTML/checkpoint, build/CTest/CI registration and PR25.
This support branch is not pushed. Independent test approval, actual regression red
against the unfixed source, focused production correction, separate code review
and passing original22 plus new3 cases remain required before merge. The original
reviewed stub-red and all valid reviews remain preserved; this is not a baseline
restart or a change to the existing assertions.

Passing these cases will establish raw-backed input snapshot behavior only. It does
not establish app-level transaction semantics, crash/backup/recovery guarantees,
hard SQL deadlines, provider/platform support or a running production server.
