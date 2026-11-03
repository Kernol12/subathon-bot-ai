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
Native environment is local Arch/GNU16.2.1/Boost1.92/Qt6.12/SQLite3.54; this does not
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

## Final269-row candidate after captured-token correction

Independent test review001896f and actual PUBLIC red on CLEANf67ae73 preceded
helper correction18cc890, integrated617ecdd. Final helper Git blob
13828cc3c72548791dc4299b7fd4c813c36b59f3 removes ONLY premature captured-token
checks; existing matrix checks remain. Final test Git blob
d740a1757699d73d84cfe7b0f88aecdadb4f3091 adds ONE focused row, no old assertion
or fixture changes. All other code/config pins above remain exact.

Root actual `cmake --build --preset sqlite-hold-debug --parallel 2` and
`ctest --preset sqlite-hold-debug --verbose` completed14:56–14:57Z:3-step build
ZERO warnings, actual CTestexit0/full264 PASS19.97s, all35hold slots/269scenarios
PASS.261Qtprocess summaries sum1059PASS/0FAIL/0skip/0blacklist.

- Final build raw `local-evidence/storage016-final-root-build.txt` SHA256
  fc241a711dc587ee3e20078f5b8b785cccf0470417a632501daf207b781c9286.
- Final full raw `local-evidence/storage016-final-root-full264.txt` SHA256
  6777936f6ca1e833bd4377f5edd6cac70cdcf4547441d532c333239e9a0492df.

This supersedes the earlier268-row result for final acceptance. Actual different
full source/config review and own promoted official CI remain pending; task is
not done or merged. No extra repeated original stub baseline.

## Final270-row candidate after retained-query seam correction

Separate regression approvalce7c61a and actual PUBLIC red/GDB at CLEAN707cb3a
preceded runtime fixef1ca070→root8f5afbb. Final runtime Git blob
44376762d972cd22a23ff0e635827c7c27d42094 adds ONLY3 lines at retained-query
catch/reuses the old mapper; no extra cleanup, unchanged default Work truth.
Final test0e172a161b8bb06efe2a35c1bce20b905e4eb541 adds ONE public source-seam row.
Helper13828, all frozen fixtures/config/public-type/old-bundle pins remain exact.

Root actual build3steps/ZEROwarnings and full264 CTestexit0/PASS19.94s completed
15:15–15:16Z:35hold slots/270scenarios,261Qtprocesses1060PASS/0FAIL/skip/blacklist.
Actual commands remain the same build plus verbose CTest preset; no old stub replay.

- Build `local-evidence/storage016-final270-root-build.txt` SHA256
  fc241a711dc587ee3e20078f5b8b785cccf0470417a632501daf207b781c9286.
- Full `local-evidence/storage016-final270-root-full264.txt` SHA256
  6f9a43da09675092c945c9cffbc1d053babd71c7dffe0c198c93e40dee335e6c.

Final independent source/config approval and this270-row source candidate's own
updated promoted official CI remain pending. Earlier269green stays attributed to
its earlier exact source, not the corrected source.
