# STORAGE-017 implementation author

Author: bundle_implementer, 2026-11-03, isolated `task/STORAGE-017-source` from
actual main `a40b42fc6d6e2a419f90f3abfd246c29dfd8173c`, after root claim455258c.
Root owns shared metadata/build/CI/publication. Own source candidate
`d675eb9e16ce87a8848fd46700f11d78b4661f0b` changes only bundle.cpp, blob
`834336d34f532508cf35d9eabb58f8ed8f0e71bf`.

Authority is the approved [concrete claim contract](../contracts/sqlite-original-claims-v1.md)
(bad19c47), [reviewed baseline](storagetest-017-handoff.md), full reviewf5c6dc1
and final attributiona72992c/eb5639a. The already completed first-red at clean
c24fb6eb had18 failed registrations/35 exact missing-query markers,36 init/cleanup
passes and zero skips; preserved raw SHA25690f3c739a735c13ca4eddecf0055ce7322cf1247e6dd52bc030e0956b1ee19b0.
That baseline preparation was not repeated. This is subsequent production
implementation verification, not a new test-baseline claim.

## Behavior and preservation

The reader validates all options, then the fixed selector's role/positive receipt/
exact CID presence before touching the borrowed ticket. It selects the global
tagged subject first, only then the ORIGINAL-kind receipt source when absent.
Actually selected rows receive strict SQL class/key/fulluint64/identity decoding.
A selected dangling/corrupt subject cannot fall through. Absence does not admit
the candidate source or audit unrelated/current data. Signed original corrections
remain kind0; subject receipt differences and source fallback's different actual
subject are preserved under their respective identity checks.

A match reuses the complete existing lookup closure and its historical/count/
Exact/vector/byte validation. The existing record accounting walk is extracted
without altering its field order or charges; the new match adds exactly one
checked discriminator byte and zero vector items after full closure/identity.
There is no fixed-selector aggregate input quota, new lifetime limit, writer
budget reset, own transaction/commit/destructor cleanup or authority inference.
Known selected failures use actual whole-unit rollback; existing writer capability,
protocol and unknown-outcome errors retain their original truth.

The public headercf721e7f, helperb0ca921c, feature-test0e4f4f00, all older tests,
fixtures and schema66 are unchanged. A static reversal check removed only the
new internal/public method blocks and accounting extraction, restoring the entire
preimplementation source130e3db62b421479ec3d0a25ce2e712480482f0e byte-exact.
The extracted field walk independently matches every original stripped line.
The root will separately remove the header's obsolete stub-status comment after
integration; this author's header remains exact.

## Actual local checks

Commands ran in the isolated source worktree, using system GNU16.2.1/C++20,
Qt6.12.0, Boost1.92.0, runtime SQLite3.54.0, CMake4.4.5, Python3.14.8,
Linux x86-64. No personal paths or dependency settings were committed.

- `cmake --preset sqlite-original-claim-debug`: PASS.
- `cmake --build --preset sqlite-original-claim-debug --parallel 2`: PASS,
 100 build steps, zero warning/error diagnostics.
- `ctest --preset sqlite-original-claim-debug -L '^sqlite-original-claim$'`:
 exit0, all18 registrations PASS,0skip,0.82s.
- `ctest --preset sqlite-original-claim-debug`: exit0, all228 PASS,7.97s.
 The225 Qt processes report zero failed/skipped; old35 bundle cases remain green.
- `git diff --check`: PASS. The programmatic source-reversal/accounting-order
 comparison described above passes.
- `python3 tools/check_project.py`: PASS396 task records/122 decisions, links,
 presets and current HTML; this does not independently approve behavior.

Raw logs are retained in this worktree's ignored `local-evidence/storage017/`:

| File | SHA256 |
| --- | --- |
| configure.txt | 27253417847e55a114d0fdd5dd85e046f47218516b23c24fb609fe0a00d1e73b |
| build.txt | 071c2a8b722a2cebe96fd09018d1c8a5e7aefd137fb58f8046104af46fab4eda |
| targeted.txt | 679535cd583274dc46d04adfc953639433bce792bd0c50b3ceac9cb68889cafc |
| full.txt | 269166f7e26278c0b1dc18de70b6832364e792c1fd37dbc020f4dc247d395b7a |

These author results do not supply independent source approval or official CI.
Different actual source/config review, selected ordinary CI and latest-head merge
gates remain with the coordinator. Application permission/current policy/complete
command matching, holds016, service baseline004, provider/network behavior,
financial refunds/recovery/platform/release gates remain separate obligations.
