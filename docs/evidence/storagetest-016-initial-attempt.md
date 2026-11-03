# STORAGETEST-016 initial execution — incomplete intended red

The initial execution does **not** establish a valid complete missing-feature
baseline. Four setup-query failures occurred before a hold call; 261 other
scenarios reached the exact named unimplemented marker. Production remains gated.
The original run and valid prior reviews are preserved; no assertion is weakened.

Actual clean head: `af310e7d68dfe6c239719bcdf69569a06c4ba526`. Actual complete
source/harness approval `adb0404`/`41aa858` preceded own selected official
[SQL41637913237](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41637913237),
successful12:42:01–12:44:17Z: fresh110, zero warnings, GNU13.3/pinned Boost108300/
verified Qt6.12/SQLite3.53.4, new required three-case foundation5PASS/0skip186ms,
exact35 compiled registrations, existing stable acceptance and all11 checks.
Official raw log SHA256c565c760b6935299e68d13d074f363a6495ca08e429d12c0e019a78645f3367c.

Only then, at the SAME clean head, root ran:

```sh
ctest --preset sqlite-hold-debug -L '^sqlite-hold$' --verbose
```

Start2026-11-03T12:45:01.970Z; end12:45:07.795Z; wall5.825s; actual CTest exit8.
35 slots fail. The per-invocation LastTest contains35 Qt summaries:70 init/cleanup
passes,265 scenario failures, zero skips/blacklists. Exactly261 unique scenario
rows report `HOLD-MISSING` with the exact unique stub message; four report
`HOLD-ASSERT: Observer query` before any hold entry. HEAD/worktree remained clean.

The preset's outputOnFailure plus verbose output repeats failed output in stdout.
The first recorder therefore counted522 marker occurrences/70 summaries and
rejected the run. Use the retained LastTest for invocation counts; do not treat
duplicate printed output as extra execution or ignore the four genuine failures.

## Actual setup defects requiring separate correction review

- Three `timerApplicationLinksAreDataAndCannotReplayBesideOriginal` kind1 rows
  select `effect_count` from `subathon_app_operations`. The frozen physical
  schema declares that column on `subathon_app_results`. Correct the query using
  the real operation/result relationship while preserving the before/after count
  and identity assertion.
- `incomingReferenceAbsenceAndMismatchActuallyRollback/wrong-retained-context`
  inserts positional context values as reference/config/document. The actual
  retained-context schema orders reference/document/configuration_ref, so that
  setup supplies an invalid empty known config reference. Use explicit named
  columns and a valid separately retained context before asserting source mismatch.

Both defects are in newly authored test setup SQL, not a provider uncertainty or
new storage implementation. Independent reviewer confirmed them against the actual
schema after execution. Author and reviewer are explicitly reassigned to narrow
corrections and native OLD-primitive setup diagnostics. No changed feature body
may run before actual correction approval and the revised complete candidate's own
selected CI. Preserve all fixtures, requirements, counts and assertions; retain
this attempt when documenting the revised baseline. No valid first-red replay or
production implementation is authorized by this incomplete attempt.

## Retained raw evidence

- [Original CTest output](storagetest-016-initial-attempt-ctest.txt), SHA256
  `4c2dd57f6f6480f3c6bd21bf957e737daedd5de281a81180d61af49ea7907be0`.
- [Original per-invocation LastTest](storagetest-016-initial-attempt-last-test.txt),
  SHA256 `6d4398039fafa4bdf0823d058a3667f5685e440b785245cdc809e7909dfa19fa`.

Original ignored recorder/analysis/API result files remain under `local-evidence/`.
All six production entrypoints remain unconditional marker stubs; no hold behavior
or service/policy/live/platform capability is reported implemented or passing.


Subsequent outcome: the [separately approved setup revision](storagetest-016-setup-correction-review.md)
and NEW complete-head official gate precede the [valid revised baseline](storagetest-016-final-review.md).
This original attempt remains invalid and unchanged; see the current
[handoff](storagetest-016-handoff.md) for task status.
