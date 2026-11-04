# LOCALETEST-001 independent fixture/probe/test review

**Current partial verdict: APPROVED corrected Qt-only fixtures/probe/foundation.**
The full43 Python feature methods require the focused T1/T2 additions below;
the actual strengthened discovery CI guard6fdd985 is accepted.
No project feature/main/script/marker execution is released by this partial verdict.

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
