# STORAGE-015 independent SQL NULL oracle correction review

Reviewer: `/root/contribution_reviewer`, 2026-11-02. **APPROVED: actual narrow
test-defect correction** `61f424243a428d2f7e96b7249ffe350f919a0ff7`.
This is separate from production source review and makes no implementation or
passing feature-acceptance claim.

| Artifact | Git blob |
| --- | --- |
| Original complete feature test | `0f8e625ff7c238776602fd1be102959f8c43b073` |
| Revised complete feature test | `a805674e014c0c7ae325474460bc64d6b10de770` |
| [Author correction evidence](storage-015-null-baseline-correction.md) | `2b47f5066d635c3e9b3bfbdd77e8f11c6e938409` |

The actual changed-source failure logs contain exactly one failed slot,
revisionsAppendContiguouslyWithoutRewritingInitial, at original line1003. Its
initial raw authority is printed as QVariant(QString,empty), while the expected
value is QVariant(Invalid). The [logical contract](../contracts/sqlite-application-plans-v1.md)
and [physical role CHECK](../contracts/sqlite-application-plans-schema-v1.md)
require revision1 with NULL parent/override and later authorized INTEGER2 audit;
neither requires SQL NULL to have the Invalid C++ metatype. Pretty-print alone
cannot distinguish typed NULL from an actual empty string, so it was insufficient
proof by itself. The prior baseline review missed this unsupported metatype oracle.

I read the complete minimal diff and author evidence. Independently verified every
byte outside the delegated slot is unchanged. The complete-history/saved-input/
source-mutation and immutable-original-row comparisons remain exact. All six
original data columns and every revision/parent/actor/reason/decision byte literal
remain selected/checked. New checks require exactly3 rows of7 columns, initial raw
authority.isNull(), both later raw authorities equal QVariant(qint64(2)), and SQL
types null/integer/integer. Removing the already independently checked raw column
from a local comparison copy preserves all original byte checks. Empty TEXT,
INTEGER0, wrongly typed authority or changed audit bytes cannot satisfy this oracle.
No producer cast, default, skipped assertion, fixture or requirement was changed.

I inspected `/tmp/storage015-null-variant-probe.cpp`, SHA256
`a96168023a63ad5278add864963e0d250162044824b206e13e20924b2707d66d`,
and independently rebuilt it as `/tmp/storage015-null-variant-probe-review` with
C++20/-Wall/-Wextra/-Wpedantic/-Wconversion/-Wshadow/-Werror/-fPIC and pkg-config
Qt6Core/Qt6Sql flags/libraries only. It includes no project header or fixture and
calls no new codec/factory/store. Actual build/run exit0, zero warnings, GNU16.2.1,
Qt6.12.0 and SQLite3.54.0:

```text
Initial: Qt type=QString valid=1 null=1 equalsInvalid=0 SQL type=null
Later: Qt type=qlonglong null=0 value=2 SQL type=integer
```

That disposable CREATE/INSERT/SELECT probe explicitly checks SQL typeof and raw
isNull/exact value, establishing a genuine driver-representation mismatch instead
of inferring NULL from an empty pretty-print. My exact compiler argv/output is
`/tmp/storage015-null-variant-probe-review-build.log`; runtime output is
`/tmp/storage015-null-variant-probe-review.log`. This is an old/raw Qt diagnostic,
not a new plan feature test, changed-source acceptance or selected official proof.

Actual static checks passed: outside-slot byte comparison, exact revised blob,
unchanged source/API/fixtures/config/other tests, `git diff --check`,
`python3 tools/check_project.py` (391 tasks/122 decisions) and
`python3 tools/progress.py render --check` (current HTML).

Original red blob `ae3e0e25956b16b0d3bc26065c71247263bca421` remains immutable
historical baseline evidence. No old/new feature, stub or missing-behavior red was
run during this correction review, and no replacement red is invented. Earlier
full approval8b537cdb remains applicable to untouched assertions; this verdict
separately approves the actual revised assertion and its written contract basis.

Root may integrate this correction and record the revised baseline hash/reason,
then rerun actual changed-source29/full173. Separate concrete production source/
configuration review, actual automatic selected official29 acceptance, final
metadata/latest-head CI/discussions/rules/normal-merge gates remain required. No
implementation or full application/provider/FX/recovery/UI/platform release scope
is approved or reduced by this test-oracle correction.
