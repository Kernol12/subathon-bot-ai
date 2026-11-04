# LOCALETEST-001 independent fixture/probe/test review

**Current verdict: APPROVED actual complete test source and harness at ad0dd23.**
The corrected 43 Python methods resolve T1/T2; the accepted seven-message fixtures,
Qt-only probe and strengthened CI guard remain unchanged. This is test/harness
approval before the first project red, not a recorded failing baseline or approval
of future production code. Earlier partial verdicts and correction history follow.

Reviewer service_reviewer, different from author codec_author and shared harness
integrator root,2026-11-04. This note begins with the intermediate fixture/Qt-only
probe gate. It does not review or approve Python behavior tests not yet authored.

## Initial actual fixture/probe candidate: changes required

Actual authora9e6eaa/root4744c63 plus shared registrationf45a262. Authority:
specification v0.45 section12/I18N-04, source contract51383296 sections3/4/6 and
TC01/02/06/07/08/14, APIacb90189. Exact inspected artifacts:

| Artifact | Git blob |
| --- | --- |
| source.json | 78a130843f79f48838f4a0a568f4f8e4423805cc |
| manifest.json | b211341b1f67aee32e2769dc22c5a30b05bf18fd |
| README | 0761f97aa842c7320d1824c530f4eafa803ef271 |
| Qt en golden | 1157fae9cc2c06b3db55a410ba82d392f6d593e5 |
| Qt de golden | 550614cc6c844a7c1a9e642999897e8d4326bef5 |
| Browser en golden | 68653ce7cc432f680a3c3493a4381b947375f276 |
| Browser de golden | e01a9b944c2d432dfacb463db4361bc124466a02 |
| Qt probe | a33933fa924ac16c51bbffdeddea7cca2547fa74 |
| Qt registration | 0ec255b2b03baa47b8b5623c8bfd7d274899c3b9 |

Independently read all six complete synthetic messages, four complete literal
XML/JSON goldens, manifest/README, full probe and registration. A stdlib-only
independent document-derived reference walk verified all four golden bytes, sorted
message/parameter/form order, exact metadata/source correspondence, canonical empty
context/indentation/tag order, XML entities/CR reference, raw LF/TAB/combining/emoji,
JSON security escapes/final LF and every manifest SHA256. No project API, renderer,
generator, main, marker or Qt probe was invoked by that review script. It verifies
handwritten oracles rather than obtaining expectations from the implementation.

Two bounded changes are required before foundation execution:

1. **R1 — literal percent under plural witness.** The current percent/braces case
   is ordinary and uses n=-1. It cannot distinguish a raw direct lookup from an
   accidental numerus wrapper that substitutes percent-n only for n>=0. Add one
   independently specified seventh message combining TC06's exact literal text
   with TC14's invisible integer selector, and require raw witness1/2 preservation.
   Existing six message objects/forms/assertions must remain exact. This tests the
   shared grammar/direct-QTranslator requirement in sections3/4/6; no new product
   policy or formatter is introduced. Update the manifest count and complete
   canonical outputs additively, then obtain actual revised-artifact review.
2. **R2 — actual missing English-base load.** The final absent-base assertion
   reuses the failed selected translator. Use a fresh translator, actually attempt
   loading a missing English-base filename, require failure and null raw lookup.
   Keep existing successful English loading and ordered fallback assertions. The
   test label/evidence must match the distinct path actually exercised; this does
   not implement the future adapter's catalog_unavailable handling.

Coordinator explicitly agreed/released these additive changes while preserving
all original assertions/oracles. The candidate was not run to force a failure and
no assertion is weakened to obtain green. No future revised fixture/probe or
Python feature test is prospectively approved.

## Static harness and limited build checks

The Qt-only probe links QtCore/public translation APIs and its own literal expected
strings, not the Python module or an expected-output generator. foundation copies
frozen TS, runs the actual supplied lrelease into two fresh directories, loads
actual QMs, verifies raw literals and compares complete repeated byte streams.
verify uses the same independent literals against future generated QM. Neither
build/discovery nor this probe calls a new project feature entrypoint.

Shared CMake registers one scaffold.translation_catalog_environment using the
actual imported Qt6::lrelease; there is no CMake generation step. Configured the
actual selected official prefix and built the actual CMake target/profile:

```sh
cmake --preset translation-catalog-debug -DCMAKE_PREFIX_PATH=/tmp/subathon-locale-qttools-6.12.0/prefix
cmake --build --preset translation-catalog-debug
```

Both pass; six effective build steps after regeneration, zero compiler warnings.
The build performs only existing compilation plus inert Python syntax/import.
NO CTest/probe/feature/main/marker execution was performed at this pending gate.
Actual fixture/probe acceptance must precede the first Qt-only foundation invocation.

Later complete Python behavior tests require their own actual independent review,
complete selected-head foundation green and FIRST valid project marker-red. This
partial fixture/probe gate cannot finish LOCALETEST-001 or approve implementation,
runtime formatter/preferences/UI/packaging/platform/full-release capability.

## Corrected actual Qt-only fixture/foundation gate: approved

**APPROVED narrow stage**, authorbbdeefb/root e1779a3, independent service_reviewer.
Only the actual fixture/probe/foundation gate is approved here; full Python behavior
tests remain unapproved. Exact accepted artifacts:

| Artifact | Git blob |
| --- | --- |
| source.json | 2717ee772aaebe438e9c78ce292afeded92fed0c |
| manifest.json | a467706f2a7ce1edfa2f26d52980bcbab291b530 |
| README | 87359836dd07942f1825e081b53d7096db12b23f |
| Qt en golden | 7b8c80e8cd29876f9e06e6cbe52285c1eb14709c |
| Qt de golden | 3d29d718124a2694a4cd83b628222b16bfea6691 |
| Browser en golden | 3f57a67032262293384d6cbfd8ca5ad4688f81c2 |
| Browser de golden | 0ec5cd3554045b79adc0cb40715eb99f8f00e09e |
| Qt probe | f033074da5e43ab61e7603d3279fb7402228b066 |
| Qt registration | 9ff756219213f1f5f1b31f4368e8273fe225a0ed |

R1 is resolved by an additive seventh fixture.literal_plural using the exact TC06
literal-percent/brace text in both forms/languages and TC14's invisible integer
selector. The actual probe now checks raw en/de witnesses1 AND2, so numerus percent
replacement cannot pass under the ordinary-message n=-1 escape. R2 uses a fresh
missing_base translator, actual missing-English filename load refusal and null
lookup; successful actual English load/fallback checks remain.

Independent mechanical comparison with a9e6eaa proves all previous six source
objects, browser message objects and complete Qt message fragments remain exact.
Only the seventh entry is added; all old assertions remain. Manifest explicitly
contains7 and every file hash matches. Prior independent complete-byte verification
plus the actual canonical additive block retains XML/JSON order/escaping/scalars/
metadata. No UUT output constructs any expected data.

Actual selected-prefix configure/build passes5 effective steps after regeneration,
zero compiler warnings. It compiles the revised probe/test syntax and performs only
the already-approved inert import. No Qt probe/CTest or project behavior was invoked
before this approval. CMake's actual scaffold.translation_catalog_environment calls
ONLY the supplied real Qt tool and this independent literal verifier; no project
generator is linked/called.

Coordinator may now run ONLY the Qt-only foundation and existing non-intentional-
red checks/ordinary CI. This releases no execution of the43 project feature methods,
main/script or missing marker. Complete actual Python-test/harness review and its
own selected feature-head foundation green remain required before FIRST project red.

## Actual complete Python candidate: two focused additions required

Read ALL43 actual methods at test blobbfec72a1ce62b82bf044c0f13a5e0423f598fd97,
inventory3c5b8830c57f7ae248a094d0a288148b49a1e506, authorbbdeefb/root e1779a3.
The17 value,20 file and6 CLI methods cover independent literal owned DTO/text
outputs, typed validation/resource boundaries, real compilation/raw-QM/reproduction,
read-only checking, aliases/inventory/source/layout changes, actual process failures,
truthful zero-exit bad-output metadata and exact CLI records/exits. Error-only helper
executables never provide a substitute successful compiler; unaffected phases exec
the real supplied Qt tool. Expected goldens/raw strings are independent; hashing
actual files checks truthful digest summaries, not a SUT-produced semantic oracle.

The distinct immediate marker cannot satisfy CatalogFailure catches or normal CLI
exit assertions. No skips/failure inversion exists. Imports/discovery do not execute
methods/setup/data providers. Runtime formatter/count display/preferences, allocation
order, unexpected internal failure and publication partial-I/O source review remain
explicit separate proof obligations, not falsely executed cases.

**CHANGES REQUIRED before final full-test approval**, from source51383296
sections3/4/5 and APIacb90189 sections2/3:

- **T1:** validation-stage precedence is GLOBAL across messages, with IDs sorted
  WITHIN each stage. Existing one-message priority and same-stage ID sorting cannot
  detect processing one entire earlier-ID message before a later higher-priority
  error. Add earlier-ID malformed-template/missing-coverage versus later invalid
  parameter cases, and earlier malformed-template versus later missing coverage.
  The higher-priority later message must win. Preserve current priority assertions.
- **T2:** browser serialization must escape U+2028/U+2029 while retaining exact
  decoded scalars and canonical raw Qt text. Neither scalar occurs in the seven
  frozen examples. Add one independent valid small render input with literal
  separator/paragraph scalars and explicit escaped browser bytes/no raw bytes,
  decoded scalar equality and raw Qt scalar checks. Preserve all seven fixtures
  and four complete goldens unchanged.

Coordinator agreed/released only additive assertions in the existing methods plus
owned author evidence. Method count remains43. No new feature implementation,
fixture weakening or retrospective baseline change is involved; no project test/
feature/main/marker has run. Actual revised tests require separate re-review.

The actual shared CI guard6fdd985, workflowd90ce3c, independently resolves the
registration coverage gap: raw43 AST definitions and43 unique names across every
top-level class, exact43 inventory/discovery, no disabled/WILL_FAIL and all three
required labels. Existing CMake properties satisfy it. Ordinary CI still executes
only the approved Qt-only foundation/non-intentional-red checks. This guard approval
does not release the43 project test bodies while T1/T2 remain unresolved.

## Corrected complete test source and harness: approved

Independent reviewer service_reviewer approves actual author
bbff4e0e3cbab4616cd4f7edad972e4b3c46dbb7, imported as ad0dd23, against source
contract5138329642d112423431d0314a6f9f9a78c7795d and concrete API
acb90189c371ab4aa8979cbb2ac8d117b2e8ab0e. The reviewed fixture/probe blobs in the
earlier narrow approval remain exact. Additional exact reviewed blobs are:

| Artifact | Git blob |
| --- | --- |
| Python test module | 6a7da9282e52ca0656284eca19732280d9d6c098 |
| Test author note | 7f794b7453ddffa5fd376397eafd6619e21dd99d |
| Case inventory | 3c5b8830c57f7ae248a094d0a288148b49a1e506 |
| Qt registration | 9ff756219213f1f5f1b31f4368e8273fe225a0ed |
| Translation CI workflow | d90ce3cc4f69aeb8e1c0dd6f88aa6419a3f95191 |
| Root CMake | 96e646c8d89cbd870c4c210952fa174a3d3466c3 |
| Presets | 2d4a00ec0f6f973df9479c8ab399b934b8208180 |
| Unimplemented Python module | 23089e6bd58c6118835ced754580002002360212 |

T1 is resolved by three actual two-message cases. Invalid parameter declarations
on z.last precede either malformed text or absent German coverage on a.first;
missing German coverage on z.last precedes malformed text on a.first. Each checks
the specific error category and z.last identity. The reversed input order and
earlier lexical ID make a per-message validation loop observably wrong. The old
same-message priority and within-stage ID-order assertions remain. Separate
subTest blocks let each input reach the immediate marker independently; that
exception remains an ordinary ERROR rather than a successful expected failure.

T2 is resolved by one independent source containing A/U+2028/B/U+2029/C in both
languages and four complete literal expected artifacts. Browser expectations use
lowercase escaped sequences, explicitly reject raw separator bytes and decode to
the original scalars. Both complete TS expectations retain those scalars raw in
source and translation, with the approved empty-context serialization. Expected
bytes are literal constants; no project renderer or escaping helper produces them.
The original frozen-source render invocation and assertions retain their own
subTest. No frozen message, golden, manifest or native assertion changes here.

Independent review checks on this actual candidate:

- Read the complete correction diff and author handoff. A stdlib AST comparison
  against bbdeefb verifies all 145 previous assertion/failure-call nodes remain
  identical and in source order. Syntax parses with the Python3.10 grammar.
- Static raw method enumeration finds exactly 43 unique qualified definitions,
  exactly matching the unchanged inventory. Inert runpy import and unittest loader
  discovery find 17 value, 20 filesystem and 6 CLI methods. No setup or test method
  runs, and the imported module is this worktree's reviewed immediate stub.
- Independently extract only the four literal T2 expectation constants using
  ast.literal_eval/string encoding. Stdlib JSON/XML parsing verifies their exact
  decoded scalars, escaped browser bytes, raw TS scalars and complete artifact
  names. An initial review script compared unqualified method names with the
  qualified inventory and failed its own assertion; qualifying that comparison
  fixes the review script without any test/source change. The corrected audit
  passes in full.
- python3 tools/check_project.py passes: 396 task records, 122 decisions, links,
  presets and current HTML. python3 tools/progress.py render --check and
  git diff --check pass. These are infrastructure checks, not feature acceptance.

The complete earlier review covers the remaining methods and their independent
oracles: exact owned DTO/text bytes, categories and global priorities, input/resource
bounds, filesystem/source/compiler aliases and ownership, read-only check behavior,
real Qt generation/reproducibility, bounded timeout/output and zero-exit bad-output
metadata, and exact CLI records/exits. Successful compilation authority always
comes from the selected real Qt tool. Negative child scripts only exercise actual
failure paths; no injected successful compiler, skip, failure inversion or marker
catch can satisfy these tests. The exact CI guard checks raw and unique counts,
inventory/discovery equality, enabled tests, absent WILL_FAIL and all three labels.
Ordinary CI excludes intentional-red feature bodies explicitly.

The coordinator ran the previously approved Qt-only foundation after its separate
approval: 100 assertions and two fresh QM byte comparisons, as recorded in
[foundation evidence](localetest-001-foundation.md). That completed gate was not
replayed by this review. The coordinator separately reports ordinary CI41799087660
on ad0dd23 successful with the same enabled 43-case discovery and five ordinary
checks; its reported raw SHA256 is
d503efe39c9e385d897fdeeabf96b39509041dd6c94363955350c906267f52e1.
That is coordinator-observed infrastructure evidence, not a Python feature result
independently observed by this reviewer.

No project feature, main, script or missing marker has been invoked by this reviewer
before this verdict. With the coordinator's actual selected-environment foundation
and current CI gates satisfied, the actual reviewed 43 methods may now receive their
FIRST ordinary stub run. Preserve raw failures, reached method/subcase attribution
and exact candidate hashes; missing dependencies, compilation errors, skips or
inverted failures cannot establish red. LOCALETEST-001 cannot be marked complete
until that evidence and applicable checks are recorded. Generator implementation,
allocation/streaming/publication source review, later runtime named formatting,
preferences, UI and platform/release verification remain separate work.
