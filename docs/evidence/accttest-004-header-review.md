# ACCTTEST-004 minimal interface and build review

**APPROVED — MINIMAL INTERFACE/STUB/BUILD ONLY**, 2026-11-03,
independent reviewer bundle_reviewer. No future fixture/test/implementation or
observed service red is approved or claimed.

Inspected the complete actual author revisions
2b96ae14ee89fedfc74ae84979acd4602f1ddb3c and
fe816735ade581f4cccfcb7c7999c755b3945314, plus root configuration5df5977,
against the [approved concrete document](../contracts/application-service-api-v1.md)
8fa0c76ad90ad046712463dfc95206e2a99b4e40 and its author note608acc1.
The [document approval](accttest-004-api-review.md) remains unchanged.

| Actual artifact | Reviewed Git blob |
| --- | --- |
| [Public header](../../server/include/subathon/server/application_service.hpp) | 03a747a70ab7decd49227396466ff12a32c31864 |
| [Stub source](../../server/src/application_service.cpp) | ebbea2388b7f4fb6474940b53cc14b95853969cb |
| [Header author evidence](accttest-004-header-author.md) | a863a1d9fd2088cb40f8df5e76499ee87086c98b |
| [Root CMake](../../CMakeLists.txt) | 166cb1f80cac29d576261a28a59b49476452482e |
| [Core CMake](../../core/CMakeLists.txt) | 1f40148a08033f7dad09c382a08d7f367677c273 |
| [Server CMake](../../server/CMakeLists.txt) | f07cd0d031e074a6ae1f0939cdd0bba6b68bd04a |
| [Presets](../../CMakePresets.json) | aeeca7a703346e403f63bcffeca1e4b54e3437f1 |
| [Selected workflow](../../.github/workflows/sqlite-writer.yml) | e507c083196fa0e97ff49a5c1102a4d63ba6774a |

All command, principal, limits, policy/clock/proposal/action/reference, response,
failure and completion members represent the agreed finite owned surface. Enums
retain their specified values; source IDs, absent/present values and exact bytes
remain typed. Required ports are pure virtual, not permissive defaults. Their
only persistence-facing argument is the finite read view, whose weak callback
state is private with no public Session/SQL/adoption constructor. Prepared body
and matching completion are wrap-accessible. Lifetime comments cover unsubmitted
pairs. The exact owned response has Q_DECLARE_METATYPE; no generic QVariant
coercion, registration/extraction bridge or alternate result channel is implemented.

The five actual feature definitions are single unconditional throws: encode and
decode have their distinct CommandEncode/CommandDecodeNotImplemented classes,
find_retained has ApplicationReferenceReadNotImplemented, and make_work/enqueue
share OriginalApplicationServiceNotImplemented. The constructor only retains
borrowed writer/ports and limits; trivial errors/accessors/types add no validation,
authority, queue, codec, SQL, cleanup, numeric or publication behavior.

The new flag implies the existing hold/bundle chain; profile21 inherits the
existing hold profile. Existing exact/timer/debt libraries are built under the
service flag without enabling unrelated GoogleTest suites. The new static target
links those existing targets. Independent byte comparison proves the workflow
diff is EXACTLY49 old-preset token replacements. All old preset entries, ordinary
guards/commands, required fixture diagnostics, mandatory hold35, dependency/action
pins, permissions and status name are preserved. No new service test exists.

Actual independent commands in the isolated review tree:

- `cmake --preset application-service-debug` — PASS.
- `cmake --build --preset application-service-debug --target subathon_original_application_service --parallel 2`
  — fresh34-step archive/dependency build PASS, zero warnings.
- `ctest --preset application-service-debug --show-only=json-v1` — PASS;
  parsed output confirms exactly264 existing registrations and no service test.
- Static source inspection confirms all five complete single-throw bodies;
  workflow/preset comparison and exact artifact hashes PASS.
- `python3 tools/check_project.py`, `python3 tools/progress.py render --check`
  and `git diff --check` — PASS396 tasks/122 decisions/current HTML.

Linux; GNU16.2.1, QtCore/Sql6.12.0, Boost1.92.0, CMake4.4.5,
Ninja1.13.2, Python3.14.8. Initial isolated configuration-copy/log-directory
setup errors were corrected before these successful checks; they are not red
feature evidence. The five coordinator configuration copies remain uncommitted
for verification only; this review file is the only reviewer-authored change.

Separately inspected root's actual header-CI record and old264 raw result,
SHA256 d35b2f9fc9737a8c74d0418b0f86ec89cf9fede547502d6f04080f07561b42b9:
root fresh118-step build/zero warnings and all old264 PASS20.47s. This is root
regression evidence, not a reviewer replay or service acceptance. No new feature
method, marker, codec, view or factory was invoked by either check path.

The complete API candidate's own selected official environment remains a separate
required gate before independent fixtures/tests. Their actual review, selected
foundation green, compiled registration and first genuine same-clean intended
marker red must precede production. All authority, policy, mixed/post-finish,
continuation, provider/platform/live and full release obligations remain intact.

## Completed own-head environment addendum

Subsequently read the actual completed API run41673443963/status JSON and
downloaded log for head8d742239616aec1b8e9be65e972a42b4fcf2ae7a,
2026-11-03T16:21:58–16:24:36Z, conclusion SUCCESS. Exact log SHA256
8cd6edea68ab01f2409d11c9e9621be1d66f4f9be18fb068ed13b260e96ac4e8.
It compiles the new service archive in a fresh118-step/zero-warning build using
GNU13.3, pinned Boost108300 and verified Qt6.12/SQLite3.53.4; required old hold
foundation and unconditional35 acceptance pass, including exact compiled guard.
The captured checkout API record identifies568fc520, parents efe1c7e+8d74223,
whose tree df709b23f3a20be0a60407026b2335de585165bb equals the candidate tree.
This completes that selected environment gate for the unchanged reviewed API;
it is neither service behavior proof nor approval of future fixture/test bytes.
