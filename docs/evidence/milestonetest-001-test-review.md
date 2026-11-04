# MILESTONETEST-001 independent complete fixture/test review

**Verdict: APPROVED actual 53-case fixture/test/harness candidate fa098e2.**
Reviewer service_reviewer, different from test author service_author and shared
integrator root, 2026-11-04. Owns this new note after the separately committed
[foundation review](milestonetest-001-foundation-review.md). No milestone feature
body/marker has run in this review worktree. This approves authored tests, not an
observed failing baseline or future implementation. Root must obtain current
selected CI/foundation green and integrate this verdict before FIRST feature red.

## Exact candidate and authorities

Actual author 9f74e77923cdf8d66df7d2dcd625178713306bb6/import643de5c plus root
harness fa098e2 follows the reviewed logical model/API and source gate a2f5d358.
Read root/core/tests instructions, specification v0.45 section8/MILE-01/NUM-04,
the complete frozen contracts and all actual fixture/test/harness artifacts.

| Artifact | Git blob |
| --- | --- |
| Literal fixture | 409be19d57b2cba4025e236a67a539af63b08be4 |
| Acceptance source | 019973844dceb289aa82ec8aabd3d9274faef98e |
| Author evidence | a54088836e54f28e4744378d136f37f485d6bdbd |
| Primitive foundation | 7f8c3f9466a0e3b313aa7e3f492b7ee82b91513d |
| Test CMake | c4457f5f06b6d50190b104f2194139cc12d24fc2 |
| Selected CI workflow | 4b64584ac40cc9c9931bf8f45eae13b16b15c6d0 |
| Root CMake | 8ab796040345500b7f6f0aceec121d9e45fc8558 |
| Core CMake | 1d9eb9e498375ac77ffda60a959598f13559604b |
| Presets | 94ef52deb3716a96c1c898a51b649c6e1c2f0552 |
| Frozen logical model | 0115a5cc3cdc4fb1ebb8c9739716a27cb3379933 |
| Frozen API | 58b8e129815c0525176cc6c1b6830a7e6f0a9fdb |
| Unchanged header | 9a1678137ba6f85cbfbb545630f08d36a261790b |
| Unchanged immediate stub | 4e3291f0e832237470a8cd0bc349e0106ad8fd77 |

No blocking fixture, oracle, source/harness or requirement defect was found. No
assertion, fixture, contract or source correction is requested to obtain a result.

## Independent oracles and semantic review

Read all 730 acceptance-source lines, the complete fixture and all 20 independent
MP rows. The fixture includes only standard-library literal data, with no project
include or calculator. Its raw input builders copy declared fields; neither a new
feature nor parser/factory supplies setup or expected results. Large input builders
produce bounded unique IDs/ascending target inputs, not an expected progression.
Expected fractions use direct unsigned integer/Boost literals. No implementation
under test generates a reference history, result, projection, fraction or budget.

The 20 independently named parameter rows cover MP01–18 and MP21–22. They inspect
all returned context/score/history/new-suffix/projection fields, current deficit
targets, captured historical target/score/revision/context, optionals and normalized
Exact components. Actual score observations are not manufactured threshold samples.
Current target corrections change projection while old capture bytes remain intact.
Refunds produce deficits rather than reset/reward replay. Empty versus all-completed,
before-first absent interval, equality advancing first and final absent next/
remaining/interval follow the frozen model. No first numeric origin is invented.

Manual arithmetic agrees with all literal distances/fractions, including negative/
zero targets, 1/2, 1/10, 1/3, overlapping deficits using the last rather than their
sum, and full signed extrema. INT64_MAX minus INT64_MIN is UINT64_MAX; the extreme
interval numerator is 9223372036854775808 with denominator 18446744073709551615,
coprime, so a signed64 projection refusal is incorrect. Endpoint cases also require
actual captured score at MIN equality and MAX crossing both extremes. No rounding,
float, score aggregation or unsafe cast supplies those expected values.

The 33 ordinary cases comprise three identity, two endpoint, six validation,
18 resource and four ownership tests. Opaque NUL/high bytes remain distinct and
input order follows increasing target values, not lexical IDs. Duplicate identity
is distinguished from nonascending target values and invalid prefix identity/order/
length. Every required reference class is checked. Historical capture targets and
scores may be nonascending/insufficient; they are data, not current validation
targets or permission. Whole input is checked unchanged on table/error scenarios.

## Resource/error and ownership checks

All five option maxima are literal and valid; zero, max+1 and size_t maximum reject
as invalid_options. Smaller selected limits independently test reference/target/
completed-prefix ceilings and all-or-none complete output growth. Exact default
256/257-byte references and 4096/4097 target/prefix counts are meaningful valid/
invalid inputs, without forged vectors. Bytes after embedded NUL count in full.
The 4096 historical rows remain legitimate manual/corrected data and project
current deficits; their final deficit 4096 follows score -1 and current target 4095.

Independent declared-field walks verify empty input 11/output 36 bytes, interval
input 50/output 118, and five-target input 56/unanchored output 53. The interval count
charges captured row 21, suffix scalars 16 and projection 70, including actual repeated
target/ID copies and Exact 1/3 bytes. Input/output item budgets are independently
3/2 and 2/4. Exact/one-less limits test the appropriate input or complete output
failure. The default 16384-item/8MiB ceilings are not falsely exercised by objects
already limited to smaller counts/references; smaller profiles reach those distinct
global accounting boundaries. No fitting prefix may be returned on resource failure.

The first-failure table separates invalid options, global input preflight before an
earlier empty reference, required captured/current references, duplicate IDs,
ascending current targets, prefix validation and output refusal. The malformed
prefix case fits input items 3 but would require output items 4 if wrongly repaired,
so invalid_completed_prefix must win. Expected categories follow the API phases.

Ownership cases preserve complete earlier results after caller mutation/destruction,
later calls, copy/move/assignment and refusal. Result data access uses the existing
private owned result; there is no forged successful constructor or certificate.
Caller-side test copies are outside the API's resource budget, as specified.

expect_failure records wrong exception types/messages as nonfatal ADD_FAILURE,
never a successful expected marker. Expected failure checks validate exact enum
and fixed message and then input preservation. Loop SCOPED_TRACE labels identify
field/row/option. Positive EXPECT_NO_THROW scenarios likewise fail on the distinct
marker. Direct positive calls can abort their remaining method statements at that
marker; no later assertion is claimed observed. No skip, WILL_FAIL, disabled case,
fake success or failure inversion exists. Static fixture/data-provider/PrintTo and
discovery paths invoke no new feature. Runtime resource assertions do not measure
allocation-before-growth, bounded scratch, bad_alloc or internal temporary arithmetic;
those remain mandatory later implementation source-audit obligations.

## Harness, actual checks and remaining gates

The standalone primitive foundation/link/green-lane safety was separately approved
in de0a30f09e0f2769d482989ceff23cd8eef2222d. Root subsequently reports actual
foundation 1/1 green with 25 facts on import7d7e621, raw SHA256
9a83525e34fb2ee0557715cdb97dbda45162babd3e87f291f82f60c371c14de8. That was
coordinator execution using local Boost 1.92, not new selected 1.83 CI or feature
acceptance. This reviewer did not replay the foundation.

Actual registration has 33 ordinary TESTs plus one TEST_P expanded into20 literal
rows, exactly 53 names. NO_PRETTY_VALUES retains raw labels; the actual workflow
guard verifies their exact CTest set, enabled/uninverted status, required labels
and one normal foundation. Its green command excludes intentional-red bodies,
not their compilation/discovery. It retains the existing immutable checkout,
Boost 1.83 package/header/config and explicit GoogleTest-fetch pins. Current workflow
publication/access and actual selected execution remain root-owned pending gates.

Independent commands already run against this exact candidate:

```sh
cmake --preset milestone-progression-debug
cmake --build --preset milestone-progression-debug
ctest --preset milestone-progression-debug --show-only=json-v1
ctest --preset milestone-progression-debug -LE '(intentional-red|milestone-progression-environment)' --output-on-failure
clang-format --dry-run --Werror tests/fixtures/milestone_progression_vectors.hpp tests/acceptance/milestone_progression_tests.cpp tests/acceptance/milestone_progression_environment.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

Fresh 18-step build passes with zero warnings, GNU16.2.1/Boost1.92.0/
GoogleTest1.18.0/Python3.14.8. Parsed discovery has90 total; actual inline workflow
guard passes 53. Only the old 33 numeric plus three infrastructure cases execute:
36/36 pass in 0.69s. Formatting, static name/fixture/frozen-source checks and
project 401-record/122-decision consistency, current render and whitespace checks
pass. No new milestone test/setup, marker or native foundation ran here.

The actual full test/harness review gate may pass. Root must integrate this verdict,
observe current selected CI/foundation green, then save the FIRST plain feature run
with clean revision, raw/JUnit evidence and independent per-case/row marker
attribution. Compilation/dependency failures, skips or an earlier local foundation
alone cannot replace that sequence. Production MILESTONE-001 and future source,
score/history authority, edit/reset/manual policy, atomic state/actions/presentation,
UI/provider/platform and release capabilities remain separately gated.
