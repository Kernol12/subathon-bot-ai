# LOCALETEST-001 independent minimal source and build review

Reviewer service_reviewer, different from Python author service_author and shared
harness author root,2026-11-04. **APPROVED — actual minimal stub/configuration
candidate1f666b0 ONLY**, source authoref4974c/roota9e60e5. This approves no future
fixture, test oracle, generator implementation, product acceptance or baseline red.

| Actual inspected artifact | Git blob |
| --- | --- |
| [Minimal Python module](../../tools/translation_catalog.py) | 23089e6bd58c6118835ced754580002002360212 |
| [Stub author note](localetest-001-stub-author.md) | 3f28223720a54c40e523539d30bce65bd4e92695 |
| Root CMake | d0eb64e31789f5d3beed41597aaf90cb6019b3cd |
| Presets/profile22 | 2d4a00ec0f6f973df9479c8ab399b934b8208180 |
| Translation workflow | f6ef3a69b4d6c15970b5e0dfebfd51bf78aafaa6 |

The actual approved source contract5138329642d112423431d0314a6f9f9a78c7795d,
source authora203ad4bbba2978bcdcea276de603ee8a75d7d04, API document
acb90189c371ab4aa8979cbb2ac8d117b2e8ab0e and API author
1cfe205bad45324023bcbb8496f8ddccaf52d432 remain byte-identical. Earlier actual
[document](localetest-001-contract-review.md) and
[API](localetest-001-api-review.md) approvals/reopened mapping history are preserved.

## Actual source audit

Read the complete157-line Python source and author note. Nine frozen-slots
classes expose the exact declared tuple/str/bytes DTO shape and inert120000 timeout
default. ParameterKind has the two exact strings; CatalogErrorCode has the15
approved values. CatalogFailure owns declared metadata and fixed sanitized text;
constructors validate neither catalog nor options and grant no capability.
TranslationCatalogNotImplemented is distinct from CatalogFailure.

Every one of validate_catalog, render_text, generate_catalog, check_catalog and
main consists of precisely one immediate named raise. There is no preflight,
source/path read, rendering, subprocess, canned help/version or generated output.
The guarded __main__ bridge catches ONLY the distinct marker, writes its exact
fixed JSON line/final LF to stderr and exits86. Inert import does not enter it.
The script/marker/feature methods were NOT invoked during review.

A read-only AST check parsed Python3.10 grammar, inspected each raise shape and
then imported under a non-main name to inspect dataclass/enum/default metadata.
All passed. No feature/main/marker constructor, CatalogFailure constructor or DTO
constructor was called; no catalog/source translation file was created. An actual
Python3.10 interpreter was not run, so grammar compatibility is distinguished from
runtime-version evidence.

## Actual shared build and CI audit

Root adds one opt-in profile inheriting the existing Qt probe. Configure requires
actual LinguistTools6.12.0. Its ALL target performs py_compile and a runpy non-main
inert import with build-local Python cache. Neither configure/build nor existing
CTest discovery calls a feature/main/script guard; no generated-file dependency
attempts to consume an unimplemented generator. Existing presets/targets remain.

The new workflow has pinned checkout, contents-read permissions, no credential
persistence and ordinary PR/main/manual events. Four exact official Base/ICU/Tools/
Declarative archives are SHA256-checked before extraction. ICU goes under prefix/lib.
Actual lrelease-version and ldd require selected Qt/ICU closure, then the same preset
builds syntax/inert imports and executes ONLY four existing infrastructure/probe
checks. Actual ELF RUNPATHs resolve the prefix; no hidden feature call is needed.
This lane introduces no project catalog test or compiled-QM foundation yet.

## Independent observed checks

```sh
cmake --preset translation-catalog-debug -DCMAKE_PREFIX_PATH=/tmp/subathon-locale-qttools-6.12.0/prefix
cmake --build --preset translation-catalog-debug
ctest --preset translation-catalog-debug --verbose
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Own fresh configure/build11steps PASS, zero compiler warnings. CTest4/4 PASS0.63s:
three existing infrastructure checks and one Qt event-loop executable with3PASS/
0FAIL/0skip. Existing Python tooling tests19PASS inside infrastructure, not catalog
product coverage. GNU16.2.1/Python3.14.8 host, verified official Qt6.12.0/ICU73 prefix.
Canonical LastTest SHA256d329ee048cc21ec1cf6de3b0e6d0e707c85ce60784e57653f2bea7e849b0ec15.
Project396/122/local links/presets/currentHTML/render/diff checks PASS.

Also independently inspected the coordinator's actual completed automatic
[CI run41792990086](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41792990086)
log at reviewed1f666b0, job126450445079. Raw SHA256
4d7fcd0bbc10295ccf4ddb4c7e3da8707f7433f7950e31891fa6cd48ea5e57fc matches its
preserved file. Fresh Ubuntu24.04/Python3.12.3 checks all four archive hashes,
selected lrelease6.12.0 and Qt/ICU prefix closure, syntax/non-main import build and
all4 existing CTests PASS. No project feature function or missing marker runs in
that workflow. This actual same-artifact environment observation is not a later
feature-head foundation or a project baseline result.

## Next gate

Coordinator may release independent literal fixture/test preparation from these
exact approved source/configuration artifacts. Actual authored fixtures/tests need
separate review; the complete future feature revision needs its own selected
literal-Qt compiled-QM/raw-lookup/reproduction foundation green before the FIRST
intended project marker-red. Missing tool/dependency/import/compilation, setup
failure or a skipped test is not that red. All five entrypoints remain intentionally
unimplemented. LOCALETEST-001 and production LOCALE-001 are unfinished; real native/
browser adapters/preferences/formatting/themes/layouts/packaging/platform/full
release requirements remain explicit and mandatory.
