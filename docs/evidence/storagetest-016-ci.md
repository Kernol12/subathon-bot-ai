# STORAGETEST-016 — reviewed baseline compilation and observed revised red

The corrected test baseline has an independently verified intended red. This is
NOT passing hold behavior: all six production entrypoints remain immediate
unconditional marker stubs. [Final attribution](storagetest-016-final-review.md)
records the actual gate; [handoff](storagetest-016-handoff.md) lists remaining work.

## Exact reviewed candidate

Clean native execution head: **598a96e89bd836e50e45b45b15a26b4b1e2f34ef**.

| Artifact | Git blob |
| --- | --- |
| Feature tests |0cce9545c3a3851b25df9131e6e35e97366aacee |
| Author evidence |1a65cc9e6f61a7c6a69f012a1b596c4165f89537 |
| Qt CMake |ea1e3cabc0de800d05ee8c8aa8b08f116c233cd4 |
| SQLite workflow |b9529c189b33399bd895ca7fdfcee8dd57c1330a |
| Clarified API |46c70297bacddbdd1649abfe58a76b33ce9773c4 |
| Public header |fe50fb10e6f5a7198df27358666734ad02fb47d1 |
| Six-stub CPP |5b10c0e0b4e7057af4042ae510af851efcd24efa |
| Frozen JSON/helper/environment |63de09c5 /7ca946f2 /a17312fe |

The [full review](storagetest-016-review.md) adb0404/41aa858 covers all35 slots,
265 authored rows and harness. The [separate actual SQL correction](storagetest-016-setup-correction-review.md)
743b87fe establishes the corrected source/note above; all assertions/scenarios and
frozen fixtures remain intact. [Final review8d3a9bc](storagetest-016-final-review.md)
independently verifies CI ordering, source pins and the actual revised run.

Prerequisite API-source CI and independent fixture/native/selected gates are in
[API CI](storagetest-016-api-ci.md) and [fixture CI](storagetest-016-fixture-ci.md).
They occurred before the dependent fixture/test assignments and were not reused
as proof of the later complete feature candidate.

## Compilation and discovery

Actual host: Linux/GNU16.2.1/C++20/Boost1.92/Qt6.12; native setup diagnostic
QSQLITE3.54.0. Configure, warning-free final three-step build, compiled -functions,
CTest --show-only and clang-format checks pass. No feature body was run during
these compiler/discovery checks. Exactly35 compiled names match35 explicit enabled
registrations; global264=unchanged229+35. No DISABLED/WILL_FAIL/failure hiding.
Required old-primitive foundation still executes; new feature red stays separate.

```sh
cmake --preset sqlite-hold-debug
cmake --build --preset sqlite-hold-debug --target server_sqlite_application_hold_acceptance --parallel 2
build/sqlite-hold-debug/tests/qt/server_sqlite_application_hold_acceptance -functions
ctest --preset sqlite-hold-debug -L '^sqlite-hold$' --show-only=json-v1
```

Latest ignored root setup-build/functions/discovery SHA256:
9e40a80c3ab15875c0ceea2b41bd2ea27bca4d1a02b8e79a3709c0b748defc3c,
94056a922e885d2c658367be53890097a6d8ca043fa83d2252bc4ce5fcaf965f,
0e3c7891520c766be4aa6eb19921addbbf05f73b192d520b7f428d79f3a2190d.

## Own complete corrected-candidate official gate

[SQL41640390883](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41640390883)
for PR head598a96e succeeded2026-11-03T12:59:45Z–13:01:46Z, all11 checks successful.
Fresh110-step build/zero warnings; GNU13.3, compiler-selected pinned Boost108300,
verified official Qt6.12/SQLite3.53.4. Required exact3 hold diagnostic guard and
actual5PASS/0fail/0skip/0blacklist188ms pass, as do old stable acceptance and exact35
feature compilation/discovery. No hold bodies execute in that CI baseline job.

The ordinary CI checkout is synthetic merge02ad181fb2d2f99d4beed657026478ec3f7e851b
with parents mainaad1ad11 and head598a96e. GitHub commit API and local Git show
its tree148090bdf9968f094e29730181ab8c6fd498d93f equals the reviewed598 tree.
This records exact candidate content without claiming a literal branch-head checkout.
Raw `local-evidence/storagetest016-revised-official-ci.txt` SHA256
42f3066c15a44bd7b5e7833563701697cb6bb903e5a94fa81213a9efedb6d2c1.

## Actual revised complete intended red

ONLY after that actual success, root ran at SAME clean native598a96e:

```sh
ctest --preset sqlite-hold-debug -L '^sqlite-hold$' --verbose
```

Start2026-11-03T13:02:29.325Z; end13:02:35.196Z; wall5.872s; actual exit8.
Canonical LastTest: **35 failing slots,265 UNIQUE exact named HOLD-MISSING rows,
70 init/cleanup PASS,0HOLD-ASSERT/setup failures,0skip/blacklist**. Every reviewed
compiled slot is represented. HEAD and tracked worktree remained clean before and
after. Assertions beyond the immediate missing entry have NOT executed yet.

- [Raw CTest](storagetest-016-red-ctest.txt), SHA256
 59a4d6d38f08300cf372b8d461e7f3b6c4059ecae940bda094f4b75edc51da75.
- [Canonical LastTest](storagetest-016-red-last-test.txt), SHA256
 b6892801f9b234cf370e48088c692bf785b9abd73580ea603dd4864f5dfdaaa2.

Verbose/outputOnFailure duplicates stdout; invocation counts use LastTest, not
printed repetitions. The [initial af310 attempt](storagetest-016-initial-attempt.md)
had261 markers plus4 setup errors and remains explicitly INVALID as a complete
baseline. It is retained with its hashes and separately reviewed genuine correction;
no invalid run is relabeled and no valid first-red replay is needed.

Final completion-metadata latest CI/current rules/discussions/normal merge remain
publication gates. STORAGE-016 must separately implement and pass these cases,
retain all old baselines, obtain source review (including actual allocation order),
and promote ordinary selected CI. Service/producer/policy/provider/platform/full
release obligations remain unverified.


Completion checks retain byte-exact raw output. Full-PR `git diff --check` reports
only original raw error-message trailing spaces/LastTest EOF blank lines; raw logs
are not normalized or rehashed to obtain a cosmetic pass. The corresponding
source/metadata check excludes only the four archived .txt logs and passes.
No source, assertion, validator or required CI gate is weakened.
