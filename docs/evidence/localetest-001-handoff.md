# LOCALETEST-001 baseline handoff

Coordinator /root, 2026-11-04, task/LOCALETEST-001-catalog-baseline,
[draft PR53](https://github.com/Kernol12/subathon-bot-ai/pull/53).
This records reviewed tests and FIRST intended missing-feature red, not generator
implementation or a passing translation product suite. Independent red attribution
and final publication gates are pending at this record's authoring revision.

## Contract, source and independent review

Specification v0.45 §12/I18N-04 and [task brief](../tasks/EPIC-023/LOCALE-001.md).
The [source contract](../contracts/translation-catalog-v1.md) blob51383296 and
[API](../contracts/translation-catalog-api-v1.md) blobacb90189 are independently
approved. Actual minimal [stub review](localetest-001-stub-review.md) approves
inert declarations and all five immediate marker entrypoints. [Test authorship](localetest-001-test-author.md)
records independent complete literal oracles. [Full review](localetest-001-test-review.md)
commit eee87e1/import9765647 approves actual test6a7da928, exact43 methods and
all source/configuration/fixture boundaries. All145 earlier assertion/failure AST
nodes remain in source order. All seven fixtures/four complete goldens and the
Qt-only probe remain unchanged by the final two Python-only additions.

The initial source mapping failure and review findings are preserved. Actual Qt
required the explicit empty context/name wrapper. Later independent review added
a plural-percent fixture/fresh missing-English load, global cross-message priority
and Unicode line/paragraph separator oracles before any project feature execution.
No baseline assertion was removed, weakened or derived from the implementation.

## Selected green setup before first red

[Foundation evidence](localetest-001-foundation.md) records selected official
Qt6.12.0 Base/ICU/Tools/Declarative pins, root/compiler environment and actual
100-assertion Qt-only foundation with two-fresh-directory en/de QM byte comparisons.
This links no new generator. CI41797849656 passed actual build/discovery/foundation
at cafe4ad. The final corrected source also passed actual selected
[CI41799087660](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41799087660)
at ad0dd2389757221f94fbf82985aa984b78edbb64: all43 enabled/uninverted methods
discovered and all five ordinary checks passed on Ubuntu24.04/Python3.12.3.
All12 PR checks at that source revision passed before first red. Raw ignored
corrected-tests-41799087660.log SHA256
 d503efe39c9e385d897fdeeabf96b39509041dd6c94363955350c906267f52e1.

## FIRST intended red — actual command and attribution

Clean tracked root9765647f0283031516e45626940fdee5e445b0bd after importing final
independent approval; the tracked worktree was clean before and after execution. Its feature/source/
fixture/build/CI blobs match the green CI source revision; only the review note
was added. Actual command (the JUnit path was absolute under ignored local-evidence):

```sh
ctest --preset translation-catalog-debug -L '^translation-catalog$' --verbose --output-junit local-evidence/localetest001-ci/first-red-junit.xml
```

Actual CTest exit8;43/43 methods failed in2.47s. JUnit records43 failures,
zero disabled and zero skipped. There were no setUp/setUpClass errors, dependency,
import, compile, tool-start, native foundation, timeout or signal failures.
[Exact raw CTest output](localetest-001-red-ctest.txt) SHA256
0f238e5982a9a2f98675bf9922043158d2f67afabccdd51340d1e65f183f5d9e.
Ignored first-red-metadata.json and first-red-junit.xml preserve actual revision,
command, clean-state receipts and per-method output.

The37 direct API methods expose109 unittest error records from
TranslationCatalogNotImplemented; they are not expected CatalogFailure exceptions.
Six CLI methods record14 assertion failures: the help method exposes the exact
unimplemented JSON/exit86 tuple; five methods report86 versus their expected normal
exit. Those five helpers do not print captured stderr before failing, so do not
claim each raw record prints the marker name. The frozen __main__ guard has only
one exit86 path, catching the distinct marker from immediate main. Source/probe/
fixture hashes and complete failure traces establish that attribution, subject to
the separate actual red review. No error was reversed into a passing test.

Three added cross-message priority sources and the separator source each reached
the marker in their own subTest. Earlier independent input failures did not abort
these new rows. The totals are REACHED failure records, not a claim that every
later sequential assertion/branch ran. Success, process-error, publication I/O and
allocation/source-audit obligations remain for implementation and code review.

Exact blobs at first execution:

| Artifact | Git blob |
| --- | --- |
| Stub |23089e6bd58c6118835ced754580002002360212|
| Feature tests |6a7da9282e52ca0656284eca19732280d9d6c098|
| Inventory |3c5b8830c57f7ae248a094d0a288148b49a1e506|
| Qt-only probe |f033074da5e43ab61e7603d3279fb7402228b066|
| Qt registration |9ff756219213f1f5f1b31f4368e8273fe225a0ed|
| CI |d90ce3cc4f69aeb8e1c0dd6f88aa6419a3f95191|

## Next work and retained scope

Obtain the actual independent red-attribution verdict, update progress/map/checkpoint,
then complete latest-head CI/rules/discussion/approval checks and normally merge the
focused baseline PR. LOCALE-001 remains planned until integrated prerequisites;
implement only its reviewed generator scope with preserved assertions, separate
code review and real full acceptance/automatic CI. No WILL_FAIL, skip, blanket
continue-on-error, canned QM or fake successful compiler is allowed.

Ordinary baseline CI explicitly excludes only intentional-red feature execution,
while always building/discovering all43 and running the mandatory real Qt foundation.
Implementation must promote actual passing feature execution into ordinary CI.
Formatting/preferences/live switching/German layouts/packaging/platform and all
other product requirements remain separate tasks; TS/JSON-only output cannot
finish LOCALE-001. No live provider or supporter operation occurred.
