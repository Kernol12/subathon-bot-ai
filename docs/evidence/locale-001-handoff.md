# LOCALE-001 implementation handoff

Coordinator /root,2026-11-04, task/LOCALE-001-catalog-generator,
[PR55](https://github.com/Kernol12/subathon-bot-ai/pull/55). Component implementation
is complete after acceptance/separate source review/selected CI. Latest completion
revision CI and normal merge remain publication gates at this record's authoring.

Contributors edit localization/messages.json; actual generator validates the whole
catalog, returns canonical Qt TS/browser bytes and compiles real selected Qt6.12 QM.
Source/alias/owned-inventory checks, bounded child execution/streaming reads and
honest per-file publication/check/CLI failures implement the reviewed source/API.
The six initial product IDs cover common actions, connection status and timer text.
Runtime formatter/preferences/switching/layout/package/platform work remains separate.

[Implementation evidence](locale-001-implementation.md), [different source review](locale-001-code-review.md)
and [CI evidence](locale-001-ci.md) record actual commands/revisions/results. Baseline
[handoff](localetest-001-handoff.md)/[red review](localetest-001-red-review.md) retain
FIRST clean976 failure43methods/109marker+14CLI and all unchanged test assertions.
Source module d7985160/catalog cb392400 and four generated text outputs are reviewed
at d7d6328 by acaeb41. Subsequent CMake wording-only cleanup changes no behavior.

Actual root commands with the selected verified Qt prefix:

```sh
cmake --preset translation-catalog-debug -DCMAKE_PREFIX_PATH=/tmp/subathon-locale-qttools-6.12.0/prefix
cmake --build --preset translation-catalog-debug
ctest --preset translation-catalog-debug --verbose
```

Full48 pass6.44s; author43 pass3.753s; different reviewer43 pass6.06s plus foundation
and infrastructure. Selected fresh CI41808104501 actually runs product generation/
read-only tracked consistency, mandatory Qt100 assertion foundation, all43 and4
other checks. All12source-head checks passed. No skip/inversion/fake QM or expected
output generation from the implementation replaced a reviewed test oracle.

Normal builds generate only build-root outputs before comparing tracked text;
staleness fails the build. Explicit translation_catalog_update regenerates tracked
text. Source files are never listed as cleanable byproducts. [Contributor notes](../../localization/README.md)
explain the workflow. Binaries stay under build/<preset>/localization; source JSON
and four generated text files are committed together.

Code review covers global validation order/fidelity/bounds, process capture/reaping,
source/stat/identity/alias checks, lstat/fstat/chunk reads, staging/publication/cleanup,
read-only consistency and safe CLI. Contract remains a serialized local host with
trusted supplied compiler; no hostile concurrent actor/filesystem-wide transaction
is promised. Actual form-key taxonomy follows specific reviewed examples, not the
ambiguous general absent-text wording; tests/contracts remain unchanged.

Next: observe final-head CI, check current protection/reviews/threads/mergeability,
normally merge exact head, refresh main and resume the next dependency-ready slice.
Preserve active ratePR54 (required exact-rational bridge009 included) and blocked
CORETEST008/PR52. No live supporter/chat/provider operation occurred. Python3.10
runtime and Windows remain unverified; do not claim deployed UI or full release.
