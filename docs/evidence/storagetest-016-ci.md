# STORAGETEST-016 feature compilation and first-red gates

Current phase: the initial complete reviewed candidate passed its own official gate,
but its first execution did NOT establish a complete intended red:261 scenarios
reached named stubs, while four setup queries failed first. See the preserved
[initial attempt](storagetest-016-initial-attempt.md). A narrow actual-schema
correction is now separately approved; its fresh complete-head official gate and
revised full intended red are pending. No production behavior is implemented.
Historical compiler/review gates below remain valid for their exact revisions.

The [reviewed fixture foundation](storagetest-016-fixture-review.md) and its
[own selected official gate](storagetest-016-fixture-ci.md) passed before test
authoring. API prose is now `46c70297`, with the
[separately reviewed category clarification](storagetest-016-error-category-review.md).
These prerequisite results do not replace the complete feature candidate's future
review, own environment gate or first actual missing-feature run.

## Actual source and root harness

Author commits `0c9f304cdf403279205a7981ea72e152a56d4ea4` and
`b3659bd93b72f9ac18478c6a1b067c8e5b8e37a8` are integrated as `828bbd0` and
`553310a`; focused correction `f0d7da7` is integrated `5dd44d0`. Final source blob:
`816441205e4a2ff5c2b69dee535818ad648b1e40` (2353 lines). The item addition
distinguishes input items from two produced historical/new
replacement versions: result_items2 fits, result_items1 requires post-SQL rollback.
Actual [author evidence](storagetest-016-author.md) `92ab22d`/`aa17214` is integrated
`70243f1`/`d5e056e`; final note blob `fc167da2131655f9c9cb1a3f8a842c3f5b8088bc`.

Independent review found a genuine pre-baseline test defect: an UPDATE-only fault
trigger would couple head advancement to a SQL verb absent from the contract. The
four-line correction retains that trigger and every assertion, adding a restricted
BEFORE INSERT guard on the same head. Author and reviewer independently demonstrated
the actual two trigger strings abort UPDATE/INSERT/REPLACE/UPSERT for that head while
permitting the unrelated head. These raw SQL diagnostics are not feature execution.

Root Qt CMake `ea1e3cabc0de800d05ee8c8aa8b08f116c233cd4` adds the new executable,
existing hold library/Qt Test links, seven fixture paths and 35 explicit enabled
CTest registrations labelled `sqlite-hold;acceptance;qt;server;draft-red`.
Workflow `b9529c189b33399bd895ca7fdfcee8dd57c1330a` preserves the entire previous
workflow prefix and appends compiled/CTest discovery consistency checks only.
The existing mandatory three-case old-primitive hold diagnostic still executes.
New feature cases remain separate intentional red; no failure inversion, disabled
test, ignored failure or automatic passing-product claim is introduced.

All fixture/header/stub artifacts remain unchanged. In particular, source
`5b10c0e0` still has six immediate unconditional unique-marker throws. No production
implementation is present.

## Actual native compilation and discovery

Host: Linux x86-64, GNU16.2.1/C++20, Boost1.92, Qt6.12.0, SQLite3.54.0.

```sh
cmake --preset sqlite-hold-debug
cmake --build --preset sqlite-hold-debug --target server_sqlite_application_hold_acceptance --parallel 2
build/sqlite-hold-debug/tests/qt/server_sqlite_application_hold_acceptance -functions
ctest --preset sqlite-hold-debug -L '^sqlite-hold$' --show-only=json-v1
ctest --preset sqlite-hold-debug --show-only=json-v1
clang-format --dry-run --Werror tests/qt/sqlite_application_hold_tests.cpp
```

Configure and initial four-step build pass. After the author's final source update,
the three-step rebuild passes with zero warnings/errors; the corrected source also
passes its three-step rebuild. Compiled discovery has
exactly35 unique feature slots; CTest has the same35 names, enabled with no WILL_FAIL.
Global264 registrations are the unchanged229 names plus35 new cases. Formatting
and diff checks pass. No whole-profile or feature-body execution occurred.

Raw files under ignored `local-evidence/`:

| File | SHA256 |
| --- | --- |
| `storagetest016-feature-configure.txt` | `819235f2f6805a4e050d117dd078aa9b4641b12e934095d8412292e285f7e8b1` |
| `storagetest016-feature-portable-build.txt` | `9e40a80c3ab15875c0ceea2b41bd2ea27bca4d1a02b8e79a3709c0b748defc3c` |
| `storagetest016-feature-portable-functions.txt` | `94056a922e885d2c658367be53890097a6d8ca043fa83d2252bc4ce5fcaf965f` |
| `storagetest016-feature-portable-discovery.json` | `0e3c7891520c766be4aa6eb19921addbbf05f73b192d520b7f428d79f3a2190d` |
| `storagetest016-feature-all-discovery.json` | `002979bffae99c55006808b51d46f6eb7c2d9c4490e95be790a2fc9309e86a22` |

Actual [full approval](storagetest-016-review.md) `adb0404`/`41aa858`, integrated
`7ab0e0d`/`220dc21`, pins the complete clean candidate
`25da01e188892fc36d76930ecbda05998d70e6df` and the exact four artifacts above.
All35 slots/265 statically counted authored rows are reviewed; no body ran. The
reviewer independently rebuilt the final source without warnings and checked
exact35/global264 discovery. No blocker remains. Actual allocation order remains
a future production source-review obligation, not an observed test property.

Next gates: publish the complete approved candidate; observe
its own selected official foundation green and exact35 compilation/discovery. Only
then run the FIRST intended hold baseline and retain exact head/time/raw log/LastTest
hash and named-marker attribution. Missing dependencies, compilation/setup failures,
skips or prior green suites cannot supply that red. STORAGE-016 remains planned.


## Initial complete official gate and incomplete execution

Own selected SQL41637913237 at clean af310e7 succeeded12:42:01–12:44:17Z,
fresh110/0warnings/GNU13.3/pinned Boost108300/Qt6.12/SQLite3.53.4/required new3
foundation5PASS0skip186ms/exact35 compiled/oldstable/all11. Official raw SHA256
c565c760b6935299e68d13d074f363a6495ca08e429d12c0e019a78645f3367c.
Only then did first execution run at SAME clean af310e7,12:45:01.970–12:45:07.795Z,
exit8. The [retained attempt](storagetest-016-initial-attempt.md) records actual
per-invocation261 missing-feature rows and4 setup errors, not a valid full baseline.
Do not convert repeated verbose output into extra invocations or hide the setup errors.

Actual revised source0cce9545 (da588312→root45b3de6) fixes only two observer SELECTs
and one context INSERT against the frozen physical schema; all assertions/counts
stay intact. Author's OLD-library native Qt diagnostic validates the exact corrected
queries without executing any feature body. Actual separate correction approval,
updated author evidence and the NEW complete corrected head's own official gate
must precede the revised complete baseline. Original attempt/raw/valid reviews and
261 reached markers remain preserved; no production implementation yet.


## Actual corrected baseline revision approved

[Separate correction approval](storagetest-016-setup-correction-review.md)
743b87fe, integrated7607f8b, accepts actual test source
`0cce9545c3a3851b25df9131e6e35e97366aacee` and author note
`1a65cc9e6f61a7c6a69f012a1b596c4165f89537` at e567141. The old full review
remains valid for all unchanged tests; this review covers the genuine setup fixes.
Every assertion,35 slots/265 scenarios and all fixture/header/stub/harness pins
are retained. Exact setup SQL passed the OLD-library native Qt diagnostic, not
feature execution. Root corrected three-step compile is warning-free; exact35
compiled slots equal registration and global264 remains unchanged.

Latest ignored root files `storagetest016-feature-setup-build.txt`,
`...-setup-functions.txt`, `...-setup-discovery.json` have SHA256 respectively
9e40a80c3ab15875c0ceea2b41bd2ea27bca4d1a02b8e79a3709c0b748defc3c,
94056a922e885d2c658367be53890097a6d8ca043fa83d2252bc4ce5fcaf965f,
0e3c7891520c766be4aa6eb19921addbbf05f73b192d520b7f428d79f3a2190d.

Freeze/publish the complete corrected candidate, then observe its OWN selected
foundation green and35 compiled cases. Keep that head/worktree clean for revised
execution, preserving initial evidence. Parse the per-invocation LastTest rather
than duplicate verbose output. A valid revised baseline requires all265 unique
named missing-feature rows/35 failing slots/70 init-cleanup passes/zero setup errors
or skips. No baseline rerun has yet occurred after correction; STORAGE-016 planned.
