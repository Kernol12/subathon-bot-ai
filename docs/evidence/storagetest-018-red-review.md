# STORAGETEST-018 independent CI and red-baseline attribution

5 November 2026. Reviewer `/root/capture_reviewer`, different from feature-test
author `/root/capture_codec_tests` and runtime coordinator `/root`. Isolated branch
`task/STORAGETEST-018-red-attribution-review`, worktree
`/tmp/subathon-storagetest018-red-attribution-review`, based on actual revised
baseline head `6332885f00c0799146058aaa5b4bab26757901c0`.

**APPROVED: the actual27-codec FIRST red at550 and revised31-case/51-scenario
SQL intended red at633 establish the complete reviewed missing-feature baseline.**
The initial SQL run at550 is invalid and remains separately preserved. No new
feature/setup/marker body was rerun by this reviewer; these findings come from
actual raw logs/JUnit, exact Git artifacts, existing review evidence and GitHub
metadata/full workflow logs. Final completion metadata, latest-head CI and PR
merge gates remain; this is not implementation acceptance or authority to begin
production before this baseline is integrated.

## Exact accepted inputs and chronology

- Supplier/API/physical review0411 and minimal-interface/stub reviewa257 remain
  valid. Immediate marker code blobs are unchanged5445276d/3372f37d; no production
  code changed between550 and633.
- Codec author367/importf7e4653, CPPc207e423 and literal53da620d were separately
  reviewed11e07c9. That source/literal is unchanged by the SQL correction.
- Fixture final JSON878005d, helper82cbf17b and diagnostic00dc1f31 received
  actual program/correction reviews11e07c9/1c6ac77. The full92 schema, both exact
  keys, complete A/B closure, constraints and whole rollback were then observed
  on host Qt and both selected official CI runs. The initial failed diagnostic
  oracle is retained separately; it is never counted as feature red.
- Actual final pre-red SQL sourceea7b53c5 at550 received separate full review
  4247210 before execution. Its metadata was imported asf098 only afterward to
  keep the actual selected-CI/red source head clean550. Import timing is not
  falsely substituted for review timing.
- Actual setup correctiona8a440e/importac04 gives final SQL CPP
  `58b22366b2f853125fd3a79fe9146d2283ccd1ec`; different correction reviewce2cc6b/
  importb13 precedes revised633 publication/CI/red. Entire delta is old setup
  zeroblob100→92 plus a strict actual one-row/one-cell/non-null QByteArray92
  assertion. Existing contract charges8+payload=100; expected later result_limit,
  physical whole cleanup and every feature assertion remain.
- CMakef3987b4/workflowed141178 mechanically register all31 SQL methods, with
  actual separate configuration approval240e046/import55046aa. Codec27 and SQL31
  remain enabled explicit intentional-red acceptance; ordinary CI executes the
  mandatory old-only diagnostic and exact compiled discovery, not feature bodies.

Original exact head: `55046aa8a88d17c9913dba3f166991da9a189f76`.
Revised exact head: `6332885f00c0799146058aaa5b4bab26757901c0`.
Reviewer observed clean633 before/after the actual revised root run. Git diff
between these heads shows no server/core/fixture/helper/codec changes; only the
independently reviewed SQL setup fragment and attribution metadata differ.

## Actual selected official CI

Independently queried actual GitHub run metadata and downloaded full logs:

| Run / exact head | Observed required foundation | Discovery | Full-log SHA256 |
| --- | --- | --- | --- |
| [42090598539](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42090598539) /550 | four slots,6Qt PASS/0skip267ms,19:08:49Z | exact27 codec +31 SQL | 0ecdc1709420e8837f03f6a4f66da286999b68e7518425827064d8530e350f3f |
| [42092102634](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42092102634) /633 | four slots,6Qt PASS/0skip231ms,19:17:44Z | exact27 codec +31 SQL | 74dc09fc7e6e58b6ee80f5a3ede3a02b850d6f43b3f15a94499274d872d67222 |

Both exact-head runs completed SUCCESS. Full logs verify pinned Boost1.83,
selected Qt6.12 archive checks and actual driver SQLite3.53.4 reported by old
diagnostics, complete required new foundation and old SQL lanes. Neither invokes
a new missing-codec/store marker or feature body. No hidden failure inversion,
disabled test or skipped baseline supplies the green CI. Exact550 also had all17
actual checkruns SUCCESS when queried; no blanket all-check claim is transferred
to another revision. Temporary independent log copies are
`/tmp/capture-review-official-42090598539.log` and
`/tmp/capture-review-official-42092102634.log`.

## Actual feature-run validity matrix

Root used separate ordinary CTest label selections, `--verbose` and JUnit output;
both original runs were on the same clean550 after its selected CI success.
Only SQL was executed again on clean633 after its revised selected CI success.

| Run | Actual failures | Init/cleanup | Classification |
| --- | --- | --- | --- |
| Codec FIRST550,19:11:12Z | 27cases,27 exact MISSING_REFERENCE_CODEC markers | 54PASS | VALID codec baseline; preserve and do not repeat |
| SQL initial550,19:11:17Z | 31cases/51rows:50 exact MISSING_REFERENCE_STORE markers +1 setup error | 62PASS | INVALID complete SQL baseline |
| SQL revised633,19:18:32Z | 31cases/51rows:51 exact MISSING_REFERENCE_STORE markers | 62PASS | VALID first complete revised SQL baseline |

All three direct CTest exits are8. All JUnit registrations are enabled, with
0disabled/0skipped; raw prefixed Qt output has no skipped/warning/fatal/crash or
dependency/compilation failure. Independent parsing counts only numeric-prefixed
CTest lines so verbose replay does not double-count failures. XML timestamps are
local Europe/Zurich UTC+1, consistent with the UTC chronology above. Revised SQL
took2.40s and its formerly failing result-bytes row now reaches the exact named
store marker after its strict old-primitive setup assertion.

| Preserved raw record | SHA256 |
| --- | --- |
| [Codec FIRST](storagetest-018-codec-red-ctest.txt) | 8bd2f2b9889c775941ff4f4fe1d39cedd2d8c13f9571cfdcbb0e17f3f9f59d9d |
| [SQL initial invalid](storagetest-018-sql-initial-invalid-ctest.txt) | e13621f40fd74aa36ea68baca186e5374658eb14d942f9fd431f942badc372bf |
| Revised SQL, /tmp/storagetest018-sql-revised-red.txt | 390b28b3b916687ca05df187bc115d788d138e9002e5a211f9f90039f6bb447d |

The invalid row was `cumulativeSqlBudgetsRequireActualWholeRollback(result-bytes)`
with `SQLite work operation failed` BEFORE the new store, because old result
accounting requires108 for payload100. That observation is neither erased nor
relabelled as an intended feature failure. The corrected92 payload fits100 and
preserves every later assertion. Codecs were not rerun for this SQL-only defect.

Most assertions after the first marker in each reached scenario remain
unreachable until implementation. These results establish missing behavior,
not their eventual pass or source safety. Current stable309/foundation checks
remain distinct from58 new CTest methods and78 total codec/SQL scenario failures.
Full source/config review and actual passing relevant acceptance are required for
STORAGE018; full product authority/provider/FX/recovery/platform/UI/package/live
release gates remain outside this baseline.
