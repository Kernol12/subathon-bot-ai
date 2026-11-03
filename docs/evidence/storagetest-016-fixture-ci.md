# STORAGETEST-016 fixture harness and environment evidence

The independent fixture foundation is authored and passes the native harness.
Full independent review has passed; this new harness's own selected official Qt CI
remains pending. Acceptance tests have not been assigned. All six hold entrypoints remain
uninvoked unconditional stubs; these checks use raw SQL, existing repositories and
construction of declared DTO values only.

The prerequisite [API review](storagetest-016-api-review.md) and
[API-source official gate](storagetest-016-api-ci.md) passed before the independent
fixture assignment. They do not supply the new fixture's official gate.

## Actual candidate and harness

Fixture author commits `6d8422bb32c89e3afa695c7d5489e43010f29636` and
`2508658aa2ad41437fc8a615a8b4835f1ff34667` are integrated as `1f5112a` and
`fc581602ec041e056948d6578735db83a67446cd`. See the
[author's evidence](storagetest-016-fixture-author.md).

| Artifact | Git blob |
| --- | --- |
| Frozen JSON | `63de09c58d07e1f9c95837cbc89ebd4e2c63b0c9` |
| Fixture helper | `7ca946f2f5569fafd95d47f073cbc836ff38d737` |
| Environment CPP | `a17312fe5700a179fb03a262d0662aef8d0da3b9` |
| Author evidence | `216244ced14a81d14a9e7072d8e1ccf305c642d4` |
| Qt CMake harness | `ec36e1327d89dd91c30adaf2221e456845e7dca2` |
| Required CI diagnostic | `559133e8bc81c613d107f986e8ac9271c5464c51` |

The two root harness files were uncommitted verification inputs during the runs
below. CMake adds one environment executable linked to the existing bundle library
and Qt Test, with seven explicit fixture paths. The workflow appends an exact
three-slot compiled-discovery guard and unconditional verbose environment run after
the preserved original-claim acceptance step. It changes no old step, dependency
pin, permission or assertion. There is no hold feature-test target yet.

## Actual native checks

Host: Linux x86-64, GNU16.2.1/C++20, Boost1.92, Qt6.12.0, SQLite3.54.0.

```sh
cmake --preset sqlite-hold-debug
cmake --build --preset sqlite-hold-debug --parallel 2
ctest --preset sqlite-hold-debug -R '^scaffold.sqlite_application_hold_environment$' --verbose
build/sqlite-hold-debug/tests/qt/server_sqlite_application_hold_environment -functions
ctest --preset sqlite-hold-debug --show-only=json-v1
ctest --preset sqlite-hold-debug --verbose
```

Configure/build pass; the new environment compiled and linked in four steps with
zero warnings/errors. Final fixture-only run passes: five Qt passes including
init/cleanup, zero failures/skips/blacklists, 82ms. Compiled discovery contains
exactly these three cases:

- `frozenCatalogAndConstraintFoundationIsIndependent`
- `pendingSourcesAndContiguousHistoryContainNoAppliedSubset`
- `processedTimerAndDistinctCorrectionHaveCompleteOldClosure`

Global discovery has 229 registrations: the existing 228 and this one environment.
The complete native run passes 229/229 in 7.99s, actual exit0; 226 Qt summaries total
720 passes, zero failures/skips/blacklists. These are foundation/old-behavior checks,
not passing hold acceptance or an intended red.

The new diagnostics verify all 89 catalog objects, exact old prefix payloads and
the 89890-byte step6 frame, actual SQL constraints/FKs, coherent pending sources
and contiguous history, and complete old timer/original and distinct correction
closure. Synthetic DTO evidence grants no application policy or producer authority.

Raw files remain under ignored `local-evidence/`:

| File | SHA256 |
| --- | --- |
| `storagetest016-fixture-configure.txt` | `af40acaf69236b273fa58f228029cacbf37ac8fcdccf989fb2175d7201a21db2` |
| `storagetest016-fixture-build.txt` | `aa90280572bac8710790d7e53f29ca314593abecaa7347c1d4bf7ccec2de10ca` |
| `storagetest016-fixture-final-green.txt` | `d1cb71e629e14b4a1b5f47651f19a9e6cae02f409fb5243b09f5761f21e3a338` |
| `storagetest016-fixture-all229.txt` | `66cf8ad0f44f564ef9f52a69e299e383041126fe2d1275a45bedeeca212bd2ab` |
| `storagetest016-fixture-LastTest.log` | `b854a7f104fbb7790b25c85fe525b4076fb8b1cf62e9d85bf4cb97b2a6e2c3a6` |

Actual separate [fixture/harness approval](storagetest-016-fixture-review.md)
`e6246145996733aa6a6ab2730a494c7c3e76ab6e`, integrated `a96367e`, accepts the
exact six artifacts above at clean `e31c5d4`. No correction was required. Independent
raw catalog/source/history/DTO audit and all35 B/I oracles agree; reviewer native
foundation passes5 Qt cases/zero skips76ms after four warning-free build steps.

Next gates: own new selected official
Qt6.12 foundation green; only then acceptance-test authoring. Actual test review,
compiler/discovery and same approved candidate environment precede the first
missing-feature red. STORAGE-016 remains a separate later implementation task.
