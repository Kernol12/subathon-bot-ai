# Handoff — STORAGETEST-018

Coordinator /root, 5 November 2026. Complete reviewed test baseline; no capture
codec/store implementation. PR68 publication/merge remains separate from this gate.

## Scope and reviewed artifacts

Specification v0.45 §§3/5/17/18, DB-01/API-02/NUM-04. Concrete supplier/API and
physical contracts are sqlite-initial-captures-v1 and sqlite-initial-captures-schema-v1;
CONTRACT014 consumer remains unchanged. Exact minimal declarations/eleven immediate
stubs are preserved. Existing schema1–6, public APIs, assertions and fixtures unchanged.

| Artifact | Authored revision / final blob | Different actual review |
| --- | --- | --- |
| Logical/API/physical documents | 6728e6a5 | 0411b7d4 |
| Inert declarations/stubs | 760f89b7 | a257be63 |
| Codec27 tests / literal header | 367f758 / CPPc207e423 / header53da620d | 11e07c9 |
| Literal schema/closure / diagnostic | 3dc3e5c plus e35/3f508/dd661 / JSON878005d | 11e07c9 + 1c6ac77 |
| SQL31 tests/51 scenarios | 4376+b5+782+b32+a8 / CPP58b22366 | 4247210 + ce2cc6b |
| Root CMake/CI | 0cec/573/c612 / CMakef3987b4/workflowed141178 | b931371 + 240e046 |

[Codec review](storagetest-018-codec-review.md), [fixture review](storagetest-018-fixture-review.md),
[SQL review](storagetest-018-sql-test-review.md), [harness review](storagetest-018-sql-harness-review.md).
Fixture author never approves own fixture: capture_reviewer reviews that artifact;
capture_fixture_author reviews only SQL tests authored by capture_codec_tests.

Root fresh/incremental builds and independent fresh141/145-step builds pass.
Current367 registrations = old308 + foundation executable1 + codec27 + SQL31.
All enabled, zero failure inversion. Stable309 (old308 plus diagnostic) PASS25.40s.
Selected fixture freeze has actual full92 projection, immutable1–6 identities,
step7 payload71601/SHAefe4d592 and both exact unique-key projections. Setup uses
OLD migration runner and independent literal data, never new manifest/codecs.

## Actual CI and intended red chronology

1. [Official42090598539](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42090598539)
   at clean55046aa succeeds: Qt6.12.0/SQLite3.53.4/pinnedBoost1.83, mandatory new
   foundation4slots/6QtPASS0skip267ms, exact27+31 discovery and all old lanes.
   All17 PR checks at that head SUCCESS. Actual test reviews preceded execution.
2. FIRST codec at SAMEclean550,19:11:12Z: CTest exit8,27/27 exact missing-codec
   markers,54initcleanupPASS/0skip/disabled/setup failures. [Raw](storagetest-018-codec-red-ctest.txt),
   SHA8bd2f2b9889c775941ff4f4fe1d39cedd2d8c13f9571cfdcbb0e17f3f9f59d9d.
   This completed gate was never replayed.
3. Initial SQL at SAMEclean550,19:11:17Z:50markers + ONE actual setup failure,
   62initcleanupPASS/0skip. [Invalid initial raw](storagetest-018-sql-initial-invalid-ctest.txt),
   SHAe13621f40fd74aa36ea68baca186e5374658eb14d942f9fd431f942badc372bf.
   Not a complete intended-red baseline. Writer charges8+payload; BLOB100 cost108
   against100 before the feature. Bounded a8 correction changes only setup to92
   and strictly checks the actual returned92-byte value, preserving budget100,
   expected result_limit and whole rollback. Separate actual reviewce2/b13 accepts.
4. [Official42092102634](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42092102634)
   at clean6332885 succeeds before revised SQL: same selected pins/driver, required
   foundation4/6QtPASS0skip231ms,27+31 guards and all old lanes. Root and different
   reviewer read full log SHA74dc09fc7e6e58b6ee80f5a3ede3a02b850d6f43b3f15a94499274d872d67222.
5. FIRST complete valid SQL at SAMEclean633,19:18:32Z: CTest exit8,31 cases/
   51 scenarios ALL exact missing-store markers,62initcleanupPASS/0skip/disabled/
   other failures,2.40s. [Raw](storagetest-018-sql-red-ctest.txt),
   SHA390b28b3b916687ca05df187bc115d788d138e9002e5a211f9f90039f6bb447d.

Actual commands and initial diagnostic defect history are in
[native integration](storagetest-018-native-baseline.md). Configure/build preset
sqlite-reference-capture-debug with selected Boost1.83; stable selection `-LE '^draft-red$'`.
Historical codec label `^reference-capture-values$`, SQL `^sqlite-reference-capture$`.
Do not rerun FIRST red to replace historical receipts. CI currently compiles/discovers
new intentional-red suites separately and runs mandatory foundation/old acceptance;
implementation must promote only actually passing reviewed suites to ordinary CI.

## Remaining scope and next work

This proves authored/reviewed tests and genuine missing-behavior red, not passing
capture acceptance. Most assertions after the marker still await implementation.
Source audit must establish allocation/preflight, bounded indexed metadata-first
reads, fixed scratch and truthful cleanup beyond these black-box outcomes.

STORAGE-018 remains planned until this baseline integrates on main. The concrete
contract permits splitting pure codec implementation before dependent SQL execution;
keep all public contracts/assertions and every release requirement. Actual authority,
rule/clock/application producers, providers/FX/recovery/UI/platform/package/live/full
release remain owning tasks. No live operation or credential data was used.
