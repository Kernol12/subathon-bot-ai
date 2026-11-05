# STORAGETEST-018 independent minimal source and build-only harness review

2026-11-05. Reviewer `/root/selected_boost`, different from minimal source author
`/root/access_reviewer` and harness author `/root`. Actual source freeze
`760f89b7f51e8d0d19f0c1dff0d5f6451ae750d4`, root source/harness candidate
`95748b9f0c06ed2b5feb6ecf2e39cf9f96273224`, final formatting-only harness candidate
`08592b7966194ccd51e9184caad91f02c034a143`. Isolated
`task/STORAGETEST-018-minimal-review`, `/tmp/subathon-storagetest018-minimal-review`.
Own ONLY this NEW review note. Source/config/fixtures/contracts/shared records are
unchanged by this reviewer.

**APPROVED for the actual minimal declarations/immediate markers and this actual
compile/link/show-only harness.** No blocking defect found. This completes the
separate minimal-source/harness gate; it is not a passing018 behavior baseline,
observed feature red, implementation/schema/SQL verification or approval of any
unwritten diagnostic/fixture/test program. Root may perform its conditional OLD308
regression check only after recording this approval; no such old test/native SQL
body was invoked by this reviewer. Every NEW018 entrypoint/marker remains uninvoked.

## Actual artifacts and boundary

| File | Git blob |
| --- | --- |
| reference_capture_values.hpp | e68bf4830eed617a938e620def63d322389fcda8 |
| reference_capture_values.cpp | 5445276db1897d7edb8ca24dedb69e8e31f5093c |
| sqlite_reference_capture.hpp | 86ad17dc2417915c05b8dfb7aac67aee6064c652 |
| sqlite_reference_capture.cpp | 3372f37d2013196a535510db9a9e2e13d61038bd |
| tests/scaffolding/reference_capture_interface.cpp, final format | 6aefa3575b4cc15e8c711bb5cd218f37e1cc81fd |
| root CMake | 3a3d7697b17290acc35ead9bd704cecb25a419ae |
| presets | 5bee6c3dbd7b8c37623b5c853db2cdbdf71b247f |
| server CMake | 8d58b48bdedf36e0fa20fca9203398f713736b26 |
| test CMake | 94bc3b2c9b48b069eb35b1267e1274d670d476c8 |
| selected SQL workflow | 547acc66fa53e6c99b0ed4eb689384054e2a1838 |

Read root/server/tests instructions, workflow/task018, accepted consumer/supplier/
physical documents, actual document review0411b7d and minimal release365be94.
Inspected all four new source files, author note, complete CMake/preset/workflow
change and link-only source. Actual source and author-note blobs equal760f89b at
both integrated revisions; no reviewed old source/header/assertion/fixture or
immutable migration1–6 changes. Final085 interface change is whitespace-only;
whitespace-stripped source tokens equal the independently built957 harness.

The nine shared/store DTO member/default lists compare exactly to accepted014/018
text after comments/whitespace removal. Existing ABI types are imported from real
headers, not duplicated: ReceiptIdentity/IDs, TimerConfig, ApplicationPrincipalBinding,
PlanKey/Envelope and actual session/manifest. In particular raw ReferenceRule
retains imported principal default account0; requiring automation1 is future
validation, not a silently authorized raw default. Enums have declared uint8/order,
six uint64 adapter limits and three qsizetype store limits retain their exact values.
No admission/evaluation/source/clock/current-authority/fresh-reference producer is
implemented or smuggled into the shared declaration ownership.

ReferenceInputFailure construction/getter/what are fixed allocation-free std::exception
mechanics with enum-only state. Fixed storage/marker error constructors use the
reviewed bounded literal messages and inert enum state; they do no native work.
Facade construction only moves/copies the weak revocable session and limits, with
no validation, construction-thread probe, BEGIN/adopt/commit/cleanup or SQL. Default
copy/destruction cannot extend the underlying work/owner lifetime. All EIGHT codec
bodies contain SOLELY `throw ReferenceCaptureValuesNotImplemented{};`; manifest,
find and retain SOLELY `throw SqliteReferenceCaptureNotImplemented{};`. No prior
cast/validation/helper, successful value, real prefix/manifest/DDL, parsing, number
formatting or session/native call exists. Markers remain distinct from expected
input/storage failures for later genuine red attribution.

## Harness, preservation and no-execution audit

The new opt-in flag enables the existing application-service dependencies; new
preset inherits its exact old profile. Two static targets import existing public
service/hold dependencies and selected Qt/Boost headers, with no new dependency,
connection/native provider or global initializer. The interface harness uses only
constructibility/decltype assertions and volatile function/member-pointer addresses
for all11 feature symbols. Its main contains no invocation and is NEVER launched
or registered with CTest. No fake successful feature smoke test or new acceptance
registration is introduced by merely linking those declarations.

Independently normalized the new workflow's preset name back to the old one and
removed its ONE new artifact-existence step: the parsed workflow then equals the
actual365be94 workflow EXACTLY. All old source-derived inventories, counts, native
commands, guards/pins/permissions and no-skipping conditions remain; the new step
only checks the two archive files and executable existence. Existing stable job
name remains. New artifacts compile/link, but no new behavior/manifest is run by
this workflow. Actual new-workflow CI remains root's later publication observation,
not a result supplied by this static review.

Both new archives' nm output has exactly8/3 feature definitions, no authored
_GLOBAL__sub_I initializer and no session execute/commit/cleanup reference. Linked
interface symbols resolve all11 entrypoints; its executable was not invoked.
Show-only discovery has308 OLD enabled/uninverted cases and no new entrypoint or
interface executable registered. This is compile/registration evidence, not an
old308 pass or018 test. Existing Qt/GTest enumeration performed by discovery does
not invoke test slots/feature bodies.

## Actual independent checks

Host C++20/GNU16.2.1, CMake4.4.5, Ninja1.13.2, Python3.14.9, actual Qt6.12.0 and
explicit isolated selected Boost1.83.0. Executed in the review tree:

```sh
cmake --preset sqlite-reference-capture-debug \
  -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 \
  -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug
ctest --preset sqlite-reference-capture-debug --show-only=json-v1
clang-format --dry-run --Werror server/include/subathon/server/reference_capture_values.hpp server/include/subathon/server/sqlite_reference_capture.hpp server/src/reference_capture_values.cpp server/src/sqlite_reference_capture.cpp tests/scaffolding/reference_capture_interface.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Configure/build133 steps/link/show-only exit0. No native program/test/marker was
launched other than permitted compiler/build/discovery enumeration. A read-only
optional harness-note lookup was absent before the build; no configure/build/setup
failure or intended red is claimed from that missing note. Final format085 has
identical semantic tokens, so an unnecessary full133 rebuild was not repeated.
Clang-format/project/render/whitespace all exit0:419tasks/122decisions/links/presets/
current HTML. Text-only eleven-sole-throw and nine-DTO/default comparisons PASS.
YAML parsing, all37 shell bodies bash syntax and21 embedded Python compilations
PASS; those workflow bodies were NOT executed by this reviewer. An initial review-
helper comparison serialized YAML's boolean key through JSON and falsely differed;
normalizing text before parsing preserves the key and proves exact workflow equality.
No repository assertion/behavior changed to fix that helper-only comparison.

Discovery JSON SHA256
36a7e045aa5423380fc54775e83e44f09b9781914661cc899c71ae602549da08;
retained discovery/stderr/nm evidence under `/tmp/storagetest018-minimal-review/`.
The no-argument CTest/native old308 command was NOT run. No new codec/store/factory/
constructor/marker, interface main, SQLite/schema/probe/fixture/program or input
generator body was invoked. Compiler/native build probes and permitted test-list
enumeration are distinguished from executing acceptance/diagnostics.

## Next gates

Root owns the authorized old308 regression and actual current selected-workflow
observation after this approval. Independently authored selected Qt schema/primitive
diagnostic and literal fixtures require their actual source/harness review before
invocation/freeze; new tests then require actual independent review, complete build
and genuine missing-behavior red. The new manifest factory remains an immediate
marker, so setup must use independent reviewed literal step7/prefix with the old
runner rather than counting setup failure as red. Coordinate7 is assigned for the
proposal by root but no migration/DDL/prefix implementation or runtime schema is
provided by these source files. STORAGETEST018 remains unfinished; STORAGE018 waits
for its integrated reviewed/red baseline. No producer/API011/authority/rules/HTTP/
provider/FX/recovery/platform/UI/package/license/live capability is approved here.
