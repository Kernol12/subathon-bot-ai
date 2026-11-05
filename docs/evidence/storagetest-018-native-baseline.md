# STORAGETEST-018 native baseline integration

Coordinator /root, 5 November 2026. Baseline incomplete; no production authorized.

## Build and first diagnostic

Authored codec367f758/importf7e4653, fixture3dc3e5c/importd677c8c plus
rollback-oracle correctione35deae/importf7c3127; root harness0cec1d8. Separate
capture_reviewer source/fixture/harness approval preceded first invocation.
Configure/build sqlite-reference-capture-debug with the existing selected Boost1.83
paths succeeds; CTest show-only336 = prior308 + codec27 + one foundation executable.
All enabled, zero failure inversion. No feature entrypoint had been invoked.

First diagnostic command atf7c3127:

```sh
ctest --preset sqlite-reference-capture-debug -R '^scaffold.sqlite_reference_capture_environment$' --verbose --output-junit /tmp/storagetest018-foundation.xml
```

Output was redirected to /tmp/storagetest018-foundation.txt. Actual exit8:
5 Qt passes/1 failure/0 skips including init/cleanup,195ms. Schema/full92 projection,
all constraints and whole rollback passed. A+B closure check failed because each
literal index-row query scoped only installationI and selected BOTH healthy rows.
This is a diagnostic oracle defect, NOT an intended missing-feature red. Neither
new codecs, store nor manifest was invoked. Preserve this observed failure.
Author correction3f5082e adds all five exact source-key predicates; all13 typed
expected members and one-row assertion remain. Different actual correction review
and new diagnostic invocation are required; no fixture freeze from this failed run.

Initial output SHA256: 742e43f7b6dfff68aa6e4caeffc3060b456ba83b47ce9d2456492dd8c76576da

## Remaining gates

Reviewed corrected foundation must pass natively and in selected official Qt CI.
Codec test review is complete. SQL source/config review, selected-driver proof
and genuine intended red remain distinct. Existing308 passing history does not establish018 acceptance.

## Corrected reviewed foundation observation

Different capture_reviewer approved actual source21c212d, additive dual-key query
assertionsdd661f0 and final JSON878005d95a0c12d69df6b836ac09cd59ba5168ba
BEFORE the second invocation. Independent notes11e07c9/1c6ac77 retain both
findings and approvals. Same diagnostic command with output suffix
foundation-corrected ran at21c212d, exit0: all4 diagnostic slots/6 Qt passes
including init/cleanup,0fail/skip,192ms. Host Qt6.12.0; selected official driver remains a separate CI gate. The actual
92-object projection and full seven history identities match independent literals;
both key projections, constraints, A/B closure and whole rollback pass. New
codec/store/manifest entrypoints remain uninvoked. Selected official CI remains
a separate prerequisite to final fixture freeze and FIRST red.

Corrected output SHA256: b5e6c97fff3654cbbe0566ebe92d5083c502d74cdaaa9fe248c6041f8a7a425e

Stable integration command after corrected foundation approval:

```sh
ctest --preset sqlite-reference-capture-debug -LE '^draft-red$' --no-tests=error --output-junit /tmp/storagetest018-stable309.xml
```

Actual exit0,309/309 PASS (old308 plus the new foundation),25.40s, no failures
or disabled/skipped cases. Raw output /tmp/storagetest018-stable309.txt. This
explicitly excludes enabled intentional-red feature baselines and is not a full
product suite pass. New58 cases require actual review/discovery/FIRST red.

## Actual selected CI and first feature observations

[Official42090598539](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42090598539)
completed SUCCESS at exact55046aa8a88d17c9913dba3f166991da9a189f76 before
FIRST feature execution. Root and different reviewer inspected actual full logs:
selected Qt6.12.0/SQLite3.53.4/pinnedBoost1.83, required4-slot foundation6QtPASS
0skip267ms, exact27+31 enabled compiled registration and all old SQL lanes PASS.
All17 exact-head PR checks SUCCESS. Final fixture JSON878005d is now observed
on the selected driver. Full CI logSHA256:
0ecdc1709420e8837f03f6a4f66da286999b68e7518425827064d8530e350f3f

Actual SQL source/harness review4247210 approved exact final CPPea7b53c5 at550.
Review support branch existed before execution; imported only afterward so root
remained same clean CI head. No new production changes existed.

At19:11:12Z root FIRST codec command, same clean550:

```sh
ctest --preset sqlite-reference-capture-debug -L '^reference-capture-values$' --verbose --output-junit /tmp/storagetest018-codec-first-red.xml
```

Direct exit8,27/27 cases fail ONLY exact MISSING_REFERENCE_CODEC marker;54
init/cleanup passes,0skip/disabled/setup failure. This valid codec FIRST red is
preserved in [raw output](storagetest-018-codec-red-ctest.txt), SHA256
8bd2f2b9889c775941ff4f4fe1d39cedd2d8c13f9571cfdcbb0e17f3f9f59d9d.
Do not repeat or replace this completed gate. Most later assertions remain
unreachable behind stubs; this is no implementation acceptance pass.

At19:11:17Z root FIRST SQL command, same clean550:

```sh
ctest --preset sqlite-reference-capture-debug -L '^sqlite-reference-capture$' --verbose --output-junit /tmp/storagetest018-sql-first-red.xml
```

Direct exit8,31 cases/51 scenarios:50 exact store markers plus ONE real setup
failure in cumulativeSqlBudgetsRequireActualWholeRollback(result-bytes), with
62init/cleanup passes/0skip. [Initial raw](storagetest-018-sql-initial-invalid-ctest.txt),
SHA256 e13621f40fd74aa36ea68baca186e5374658eb14d942f9fd431f942badc372bf.
This is INVALID as a complete SQL baseline, not51 intended feature failures.
Existing writer result accounting charges8 bytes per value plus BLOB payload;
zeroblob100 exceeds the configured100 before the new store can run. Bounded
author correction/review must use payload92 to consume EXACT100 and preserve
the expected later result_limit/whole rollback. Selected new-head CI and revised
SQL red remain mandatory. No production implementation is authorized yet.

Store implementation note, independently agreed by author and SQL reviewer:
retained SQL stores documents, not outer format refs. Fixed known SRC/SRA/SRX
role metadata is not an actual selected reference under store reference ceiling26.
Public codecs still bound supplied format refs; a private borrowed body path must
preflight all actual nested refs under26 before ownership. Any owned synthetic
envelope still counts fixed-format bytes in its complete owned budget. Widening
a public codec profile then checking actual refs after decode is forbidden.

## Valid revised SQL baseline

Actual different correction reviewce2cc6b/importb13 accepts source58b22366.
Selected [42092102634](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42092102634)
at exact clean633 completedSUCCESS: Qt6.12/SQLite3.53.4, required4/6QtPASS0skip
231ms and both compiled guards, no feature body. Full logSHA74dc09fc7e6e58b6ee80f5a3ede3a02b850d6f43b3f15a94499274d872d67222.
Only then root revised SQL command at SAMEclean633,19:18:32Z, directexit8:
31 cases/51 exact missing-store markers,62initcleanupPASS,0other failures/skip/disabled,
2.40s. [Valid revised raw](storagetest-018-sql-red-ctest.txt),
SHA390b28b3b916687ca05df187bc115d788d138e9002e5a211f9f90039f6bb447d.
Different capture_reviewer independently corroborated raw/JUnit/CI/chronology
without replaying either completed gate. Codec FIRST remains original550.
Full baseline is now observed red; implementation and latest publication/merge
remain separate. See [baseline handoff](storagetest-018-handoff.md).
