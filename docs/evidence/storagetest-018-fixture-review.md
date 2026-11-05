# STORAGETEST-018 independent fixture and diagnostic review

5 November 2026. Reviewer `/root/capture_reviewer`, different from fixture author
`/root/capture_fixture_author` and harness author `/root`. Isolated branch
`task/STORAGETEST-018-foundation-codec-review`, worktree
`/tmp/subathon-storagetest018-foundation-review`.

**APPROVED for actual foundation program/helper/literals and CMake/CI harness at
`f7c3127014e7f80a3703640454260d2ef21ac126`.** This permits its first diagnostic
invocation. No new diagnostic, schema, codec or feature body was invoked during
this review. Selected-driver observations, fixture freeze, SQL feature tests,
their independent review and genuine intended red remain separate gates.

Actual author `3dc3e5c` was integrated as `d677`; the reviewed correction
`e35deae` was integrated as `f7c3127`, after actual harness `0cec1d8`.

| Inspected artifact | Exact blob |
| --- | --- |
| tests/fixtures/sqlite_reference_capture_vectors.json | 0c7bbc44db87d4ed11f24eb60370f0c940cedbe0 |
| tests/qt/support/sqlite_reference_capture_fixture.hpp | 82cbf17b5265381e89bd5c32c1076eed734ead2e |
| tests/qt/sqlite_reference_capture_environment.cpp | 00dc1f312a400e1d71b2aa934b4fd63f8208aefe |

Read root/tests instructions, concrete/physical018 and accepted consumer014
contracts, relevant DB-01/API-02/NUM-04 requirements, prior accepted document and
minimal-interface evidence, all new fixture/helper/diagnostic source, root harness
diff and relevant existing migration/writer/fixture APIs.

## Finding resolved before invocation

The actual first candidate still asserted `WorkError::none` after explicit
`session.rollback()`, despite the author's handoff saying otherwise. Existing
writer contract/code and `sqlite_writer_work_tests.cpp` distinguish explicit
rollback as `not_committed`, with `rolled_back` and no owned normal value.
Requested correction before any native invocation. Actual `e35deae` changes only
that error category and author evidence; all physical row, rollback, prior intake,
outside visibility and subsequent BEGIN/COMMIT assertions remain. This is an
independently reviewed diagnostic defect correction, not weakened018 acceptance
or a missing-behavior red run. Corrected actual blob above was reread.

## Independent literal and source audit

Independent standard-library F-framing reproduced every old per-step size/hash
from existing frozen manifests, and new step7: one1977-byte DDL, full92 tuples,
71601 payload bytes, SHA256
`efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218`.
All89 old tuples remain verbatim; exactly one table/two SQLNULL generated indexes
are added. New DDL exactly equals the accepted physical document. No factory or
SQLite engine produced these expectations.

Independently scanned all four complete A/B documents using fixed grammar:
SRC1 rule/cap/timer/five facts, SRA1 embedded exact SRC1/arrival/source/payload,
SRX1 complete IDs/plan/rule/installation and SRI1 all thirteen members. A uses
policyW/Grace120000/Fresh, absent cap/deadline, complete facts; separate codec
fixture's policy p/Immediate is a different valid structural fixture. A key26/
record55/document99/envelope136 and B exact E+NUL/MAX IDs are consistent.
SQL bindings were checked against actual immutable source/support/captured-plan
schemas and the contract: real CID0/MAX, positive receipt7/MAX, coherent allocator
high-water marks, absent activity/money/proof, captured support conditions,
revision1/no parent/override, event=root, one selected rule, zero modifiers and
one canonical600000/1 request with exact explanations/reversal provenance.

Helper builds independent literal1–7 manifest and invokes the accepted actual
migration runner; no raw catalog/history writes or new manifest/codecs/store are
used for setup. Every fixture SQL statement remains explicit trusted test data.
The diagnostic links only old holds/migration/writer dependencies. Its four slots
cover exact full schema/history/index columns and literal frames; complete A/B
typed closure observations;23 class/ref/FK refusals plus PK and receipt uniqueness;
and same-session provisional visibility/whole plan-SRX-index rollback preserving
earlier intake/support. Refused statements require an actual synchronous
`sql_failure` and terminal known rollback. No foreign-keys-off setup, secondary
authoritative writer, implementation-generated expectation or fabricated success.

Root CMake registers one enabled foundation executable with AUTOMOC/QtTest and
only old library linkage. Ordinary selected CI requires exactly all four compiled
slot names and runs this foundation unconditionally. Existing pins/check names/
guards/suites remain. The separate27 codec cases stay enabled explicit
intentional-red acceptance, discovered but not executed in passing ordinary CI.
No DISABLED/WILL_FAIL/failure inversion/manual suppression was introduced.

## Actual checks

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug
ctest --preset sqlite-reference-capture-debug --show-only=json-v1
python3 /tmp/reviewer_capture_audit.py
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0. Fresh141-step build;336 enabled registrations = old308 + one new
foundation +27 codec cases. `-functions` discovery yields exact4/27 compiled slots;
no body invocation. Workflow YAML and all23 embedded Python blocks parse.
Independent text audit reproduces all7 canonical hashes and complete fixture
grammar. Host GNU16.2.1, Qt6.12.0, selected Boost1.83.0, CMake4.4.5/Python3.14.9.

This approves the actual diagnostic and its harness, not runtime schema92 or
fixture pass, SQL capture behavior, authority/evaluator/provider/recovery or full
release acceptance. Observe selected Qt/SQLite before freezing the fixture.

## Observed diagnostic defect and reviewed dual-key correction

Root's first approved host invocation at `f7c3127` failed the A+B closure slot:
the original index observation filtered only installation I and therefore correctly
selected both installed A/B rows while expecting one. This reviewer missed that
predicate defect in the initial audit. Root's log
`/tmp/storagetest018-foundation.txt`, SHA256
`742e43f7b6dfff68aa6e4caeffc3060b456ba83b47ce9d2456492dd8c76576da`,
records exit8,5PASS/1FAIL/0skip including init/cleanup,195ms. Schema/full92,
constraints and whole rollback slots passed; the foundation as a whole was NOT
green. This was a diagnostic oracle defect, not a missing018-feature red run.

Actual author correction `3f5082e66431de086f52f7af6a329df20c10ac61`, root import
`4fa8c00`, supplies all five exact BLOB predicates, distinguishing opaque45 and
4500. All original13 typed expected fields and one-row assertions remain.
Reviewer also requested the physical contract's second installation/receipt
projection. Actual additive `dd661f005fc4ec53b5c40b1c1969d78b75fddcc5`, root import
`21c212de1dadcb56a2883661d511e279163a3790`, adds that independent exact BLOB8
receipt query for each A/B with the same13 typed field expectations.

**APPROVED before corrected invocation:** actual final JSON blob
`878005d95a0c12d69df6b836ac09cd59ba5168ba`, at root `21c212d`; helper/diagnostic/
harness/codec blobs unchanged. Both lookup predicates now select exactly the
intended complete record; no row-count relaxation, fixture weakening, skipped
check or implementation change. Approval was sent before root's next invocation.
Corrected runtime/selected-CI result and fixture freeze remain observations to
record separately; this note does not fabricate them.
