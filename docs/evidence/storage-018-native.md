# STORAGE-018 native integration and regression evidence

Coordinator /root, 5 November 2026. Synthetic fixtures and disposable state only.
Historical FIRST stub reds are preserved in the baseline handoff, never replayed.

## Original baseline and source review

Initial author ce8d4d7/import54967ed, source46c0e25, passed all unchanged31 methods/
51 scenarios and old337 checks. Passing those tests did not establish source
correctness. Separate reviewer capture_reviewer found the gaps below. Independent
author capture_codec_tests added seven methods/16 scenarios, preserving every old
assertion and fixture. Each actual program and harness was separately approved
before its intended behavior red; only then was that correction authorized.

All commands below used `ctest --preset sqlite-reference-capture-debug`,
`--no-tests=error --verbose --output-junit /tmp/storage018-<case>-red.xml` and the
exact method filter shown. A reached assertion failure is distinct from a stub,
setup error or compilation failure. Every run below had zero skips/setup failures.

| Finding and filter | Clean root / source / UTC | Actual red | Correction |
| --- | --- | --- | --- |
| `-R '^sqlite-reference-capture.unindexed'` (four new structure methods) | 87a6901 /46c0e25 /20:37:03 | exit8,6 failures+2 correct controls,8 init/cleanup passes | 8f5a7d7/import98c252e |
| `-R '^sqlite-reference-capture.indexedSelectedPlanOwnedBudgetUsesSemanticDtoBytes$'` | 807e1d3 /46c0e25 /20:43:58 | exit8,240-byte case wrongly refused;222 correct refusal;2 init/cleanup passes | da96727/import6735dfc |
| `-R '^sqlite-reference-capture.indexedChildMetadataRejectsMalformedScalarsBeforePayload$'` | 45533d5 /8702843 /21:02:22 | exit8,2 wrong-reason failures;2 init/cleanup passes | 1d54acc/importcce251e |
| `-R '^sqlite-reference-capture.unindexedActivityOriginsValidateAgainstActualReceipts$'` | d18e15c /27fa987 /21:06:38 | exit8,3 corruption failures+1 healthy mismatch control;2 init/cleanup passes | 90eeca7/importc0ff1ec |

Raw records and SHA-256:

- [Structure](storage-018-structure-red-ctest.txt): `a4944334a59e8acb40ccf00b186de833969135367ec58c1bb47e7b5400abef97`.
- [Semantic budget](storage-018-budget-red-ctest.txt): `3d114490a862719f56610f6af740a8ff5354bb6b7d0afadc938e5791b067f7b2`.
- [Metadata aggregates](storage-018-aggregate-red-ctest.txt): `6949ac096d92904ea204c6b242c0a769f24fae27ec29ab52bffd974493f8884f`.
- [Actual origins/full anchor](storage-018-origin-red-ctest.txt): `f97639410d5189d6a94c91b5e7b02275aef5a3febc5d76d263fdfb5edfa6ea72`.

Structural validation precedes profile comparisons: canonical exact values, actual
receipt valuation coupling, required versus opaque empty references and generic
child metadata. Off-profile cardinality is refused after bounded type/count/ordinal
checks without reading or certifying all unsupported generic payloads. Supported
1/0/1 reads validate full selected payloads before meaning comparisons.

The independently derived frozen plan is223 semantic DTO bytes, snapshot224
(225 with outer optional);240 safely fits,222 does not. The old279 physical-row
sum wrongly charged duplicate foreign keys and ordinals. The correction projects
actual declared DTO fields dynamically before payload ownership, with separate raw
SQL eight-byte-value/payload guards; no fixture constant or arbitrary subtraction.
CASE-guarded aggregates return only fixed INTEGER/NULL before Qt ownership; an
all-row bad-type/domain sum still rejects malformed mixed rows. Original support
activity keys must be compatible and both origins match the actual original receipt;
verified observation origins match actual observation receipts. Healthy off-profile
activity remains a mismatch preserving the caller session, corruption requires
known whole rollback or honest uncertainty.

## Combined passing candidate

Author90eeca7/rootc0ff1ec SQL CPP `ccdc4f616529aedc310ffc75259a79f7690239b5`;
SQL test CPP `5c4dd48162d224220cae5c0a95813e7dc9608136`, literal fixture
`878005d95a0c12d69df6b836ac09cd59ba5168ba`. Selected source/includes are unchanged
by later CI/metadata work. Existing public signatures and codec implementation stay
unchanged; SQL header comments now describe implementation rather than stubs.

Host GNU16.2/Qt6.12, selected Boost1.83 from disposable SDK paths:

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug -j4
ctest --preset sqlite-reference-capture-debug --output-junit /tmp/storage018-root-final.xml
```

Root configure/build exit0, zero compiler warnings; full375 PASS28.29s (old337
plus SQL38;67 SQL scenarios), zero failed/disabled/skipped. SQL selection3.37s.
Raw `/tmp/storage018-root-final-ctest.txt` SHA-256
`36263a03210604644105726fed1e9b31a520f994efd4ba39e75ae874f7967355`;
build `/tmp/storage018-root-final-build.txt` SHA-256
`d780de2d3012f4ae11345ff31860704e5b6a335021492bbe7ac7334f2a26d86c`.
Author full375 PASS29.19s. Independent reviewer fresh145-step build/rebuild and
full375 PASS28.32s, zero failures/disabled/skips/warnings; detailed commands,
source/resource audit and configuration review belong to the separate review note.
Ordinary selected Qt CI and final publication/merge are separate gates.
