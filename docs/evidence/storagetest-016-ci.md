# STORAGETEST-016 feature compilation and first-red gates

Current phase: complete feature source is authored, compiled and registered; actual
independent full source/harness review is active. No hold method, factory or feature
test body has run. Compilation and discovery are not an intended red.

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

Next gates: complete written different full test/harness review of corrected source
and final author note; freeze/publish the complete approved candidate; observe
its own selected official foundation green and exact35 compilation/discovery. Only
then run the FIRST intended hold baseline and retain exact head/time/raw log/LastTest
hash and named-marker attribution. Missing dependencies, compilation/setup failures,
skips or prior green suites cannot supply that red. STORAGE-016 remains planned.
