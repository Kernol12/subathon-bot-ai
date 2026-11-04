# LOCALETEST-001 independent fixture/probe/test review

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
