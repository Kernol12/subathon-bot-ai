# STORAGE-016 independent native validation

2026-11-03, coordinator `/root`, separate from both runtime and pure-value authors.
Task branch `task/STORAGE-016-durable-holds`, integrated source at b32ba1d plus
the five concrete root configuration/comment edits below. Actual code was built
and exercised, rather than inferred from author results.

| Artifact | Git blob |
| --- | --- |
| Runtime CPP | 8d82aa2268b39f19b8ed640c03383c99f640d6e3 |
| Pure hold values | ec78b69702ce0e1da3f78b9367dc10170c284bfc |
| Private bundle access | 7107d0ee88d4d51be8642b55b1bceadb1758489a |
| Factored old bundle | 24759a20a8c9e1de5f577bf36d16f829a171b03f |
| Unchanged old bundle values | b0ca921cb27068ecc33228d136b82506fcb6e0a3 |
| Public header, obsolete comment removed only | b9043716536063d7dc47ca97b2874ba1ccc80586 |
| Reviewed revised test | 82980fdba7d538e90c66608afd4e2fe45f229cff |
| Qt CMake | 54cd5a1f80afb50c93a0110aec0d21fff088f47c |
| SQLite workflow | 14ff57de00d8f920cdbb1b57db6153ede87e811a |
| Root CMake | c30fa709fb6c6ca6a44f3e890b66bad87fc4a53c |
| Presets | 62acfe5717243f36103c656cc3f3b48f921ed1f7 |

At 14:46–14:47Z, actual commands:

```sh
cmake --preset sqlite-hold-debug
cmake --build --preset sqlite-hold-debug --parallel 2
ctest --preset sqlite-hold-debug --verbose
```

Configure succeeded; incremental build completed29 steps, ZERO warnings. Full264
CTest passed in19.73s, including all35 hold slots/268 scenarios and every unchanged
old case. All261 Qt process summaries total1058 PASS/0FAIL/0skip/0blacklist.
Native environment is local Arch/GNU16.1/Boost1.92/Qt6.12/SQLite3.54; this does not
replace the selected official Ubuntu/GNU13.3/Boost1.83/Qt6.12 CI gate.

Ignored raw evidence and SHA256:

- `local-evidence/storage016-root-configure.txt`:
  af40acaf69236b273fa58f228029cacbf37ac8fcdccf989fb2175d7201a21db2.
- `local-evidence/storage016-root-build.txt`:
  883eeb49916532ba07e6ec2b8487c0a989cc0c2334b432af81a58180614d63a3.
- `local-evidence/storage016-root-full264.txt`:
  f7ddbe8664e85621799f94eaf60f658e5f62f843e61d58a15d762d8798be50ee.

Independent [baseline revision review](storage-016-baseline-revision-review.md)
and [actual red](storage-016-baseline-revision.md) preceded the two source fixes.
Original stub-red and frozen fixtures remain preserved. This passing run does not
measure allocation order; actual source review remains required.

Root promotes only the35 newly passing hold cases: remove their `draft-red` label,
update option/preset wording, remove obsolete factory-stub comment, and execute
`ctest --preset sqlite-hold-debug -L '^sqlite-hold$'` unconditionally in ordinary CI
after exact registration/discovery and mandatory old-primitive foundation. No
disabled test, failure inversion, continue-on-error, manual toggle or fixture edit.
All old workflow steps, dependency pins and public types/signatures remain fixed.

Full independent source/config review, the specific conditional token-presence
phase audit, own promoted official CI and final merge gates remain pending.
No provider, policy, application orchestrator, live/platform/release verification.
