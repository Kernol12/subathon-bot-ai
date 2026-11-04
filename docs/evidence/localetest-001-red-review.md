# LOCALETEST-001 independent first-red attribution

**Verdict: ACCEPTED intended failing baseline at
9765647f0283031516e45626940fdee5e445b0bd.** Reviewer service_reviewer, different
from test author codec_author, stub author service_author and coordinator root,
2026-11-04. This is a read-only audit of the first saved execution, with no replay.
It approves the observed red attribution, not an implementation or passing feature.

## Reviewed inputs and chronology

Authority is specification v0.45 section12/I18N-04, source contract blob
5138329642d112423431d0314a6f9f9a78c7795d and API blob
acb90189c371ab4aa8979cbb2ac8d117b2e8ab0e. The complete actual test/harness approval
eee87e1d9290d92c4363de7c272bce1c89f41099 was imported as the execution commit
9765647f0283031516e45626940fdee5e445b0bd. See the preserved
[test review](localetest-001-test-review.md), including earlier fixture/probe
corrections, narrow Qt approval and actual T1/T2 resolution.

Git comparison independently verifies identical source, tests, inventory, all seven
fixtures/four goldens, native probe, contracts and build/CI configuration between
the final review, source-CI commit ad0dd2389757221f94fbf82985aa984b78edbb64 and the
first-red commit. In particular:

| Artifact | Git blob at first red |
| --- | --- |
| Immediate Python stub | 23089e6bd58c6118835ced754580002002360212 |
| Python tests | 6a7da9282e52ca0656284eca19732280d9d6c098 |
| Inventory | 3c5b8830c57f7ae248a094d0a288148b49a1e506 |
| Native literal probe | f033074da5e43ab61e7603d3279fb7402228b066 |
| Qt registration | 9ff756219213f1f5f1b31f4368e8273fe225a0ed |
| CI guard/workflow | d90ce3cc4f69aeb8e1c0dd6f88aa6419a3f95191 |

The actual saved locale CI job41799087660 log independently shows selected
lrelease6.12.0/runtime closure, exact43 enabled/uninverted discovery and five
ordinary checks green at 08:23:46 UTC, before this execution. Its Qt-only foundation
completed100 assertions and two fresh real-QM comparisons with the accepted
en/de hashes. Ordinary CI explicitly excludes intentional-red cases. Raw
corrected-tests-41799087660.log SHA256 is
d503efe39c9e385d897fdeeabf96b39509041dd6c94363955350c906267f52e1.
The coordinator separately reports all12 applicable checks successful; this audit
directly inspected the saved locale job, not every other job. Earlier selected
local foundation evidence is preserved in
[foundation evidence](localetest-001-foundation.md). No dependency, compilation,
native-tool failure or skipped test is substituted for feature red.

## Actual command and preserved files

The coordinator ran the following command in the root checkout, not this isolated
review worktree:

```sh
ctest --preset translation-catalog-debug -L '^translation-catalog$' --verbose --output-junit /home/archnoob/ProgrammingProjects/SubathonProject/subathon-bot-ai/local-evidence/localetest001-ci/first-red-junit.xml
```

Observed coordinator result: exit8; CTest43/43 failed, zero disabled/skipped,
2.47 seconds (metadata2.475). Each invocation uses /usr/bin/python3.14, its exact
qualified reviewed method, the root tests/localization working directory, selected
official lrelease and compiled native probe paths. The 60-second CTest timeout did
not cause any failure. The coordinator metadata records a clean checkout before
and after; this reviewer also observed the root clean at the execution commit.

Saved ignored files are under the root checkout's
local-evidence/localetest001-ci, inspected by absolute path. Independently computed
SHA256 values:

| Saved file | Bytes | SHA256 |
| --- | ---: | --- |
| first-red-ctest.txt | 457538 | 0f238e5982a9a2f98675bf9922043158d2f67afabccdd51340d1e65f183f5d9e |
| first-red-junit.xml | 222437 | 4047f73032b1789463f049f6eda631adfe463c7c35e5d546a6045e0358835776 |
| first-red-metadata.json | 1212 | d0f7fc2f4290dce52c52f560cf1fa87f78d8de3b94ea43b5ceb763f83f97cdbe |

Metadata revision, command, reviewed commit, source-CI revision and all six recorded
Git blobs match the actual Git objects. No source/assertion/fixture adjustment
occurred between review and run. The reviewer read/parses these files; none is
rewritten or generated from another run.

## Independent failure attribution

Parsed every JUnit testcase and system-out. There are exactly43 unique names,
identical to the reviewed inventory, each with status fail and a failure element;
the suite records disabled0/skipped0. Counts below use each JUnit stream once.
Verbose CTest repeats failed output, so counting its duplicate tracebacks would
inflate the result.

| Reviewed methods | Reached ordinary failures | Actual cause |
| --- | ---: | --- |
| CatalogValues17 | 81 errors | Named immediate marker tracebacks |
| CatalogFiles20 | 28 errors | Named immediate marker tracebacks |
| CatalogCLI6 | 14 assertion failures | Script marker exit86 disagrees with required exits |

The37 direct methods contain109 error records, each ending in
tools.translation_catalog.TranslationCatalogNotImplemented with the reviewed
message. Their exact immediate bodies are reached: validate_catalog78,
render_text3, generate_catalog24 and check_catalog4. Every recorded error header
belongs to the method/subcase, with no import/setup failure or unrelated exception.
CatalogFailure catches do not catch this different exception. Filesystem setup and
any independent real-Qt preparation before these calls succeed far enough to reach
the stub; later native verification is not thereby claimed executed.

The six CLI methods launch the exact frozen Python script via sys.executable,
without a shell. The help case fails its first tuple comparison and prints the
exact unimplemented JSON/exit86 in the assertion diff. The other five methods stop
at record's return-code assertion: validate and generate each86!=0; nine independent
usage rows each86!=64; validation-error86!=65; mismatch86!=1. Those five outputs do
not print captured marker stderr. Their attribution combines the actual86 result
with the reviewed script's sole exit86 path: immediate main raises the marker,
the specific __main__ catch emits the fixed marker JSON and exits86. This is not
a claim that all six raw outputs visibly print the marker. No CLI expected normal
exit matches86, and no marker is caught or inverted into success.

T1's three new source labels each have their own named-marker error:
later-parameters-before-earlier-template,
later-parameters-before-earlier-coverage, and
later-coverage-before-earlier-template. The four original priority groups also
reach their separate calls. T2's unicode-line-and-paragraph-separators source and
the original seven-frozen-messages source independently reach render_text. Thus
the earlier marker cannot hide these additive inputs.

## Limits and handoff

All43 methods are observed red; the109 direct errors and14 CLI assertion failures
are reached failure records, not a count of all authored scenarios. Sequential
statements after an immediate error/assertion do not run. For example, the help
case does not reach version/direct-main checks; validation errors do not reach
their later expected category/identity assertions; and the first CLI invalid-JSON
failure prevents later I/O/tool rows in that method. The frozen expected outputs
remain independently reviewed oracles, not observed successful comparisons.

Independent audit commands were read-only Python3 stdlib parsing/hashing of saved
logs/JSON/XML, exact inventory and per-case summary/traceback assertions, Git blob
lookups and git diff --name-only across the reviewed/source-CI/execution revisions.
No test, setup, project function/main/script/marker, native probe or compiler was
invoked by this audit. Root remains responsible for publishing the raw evidence
and coherent task-state/checkpoint changes. With that evidence recorded, the
test-author baseline gate can complete. LOCALE-001 production implementation,
its actual passing acceptance and separate source review, runtime formatting,
preferences/UI and platform/release gates remain unfinished.
