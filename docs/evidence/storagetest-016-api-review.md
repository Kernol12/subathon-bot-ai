# STORAGETEST-016 — independent minimal API/stub/configuration review

Reviewer: bundle_reviewer, 2026-11-03. **APPROVED actual minimal API/stub/config**
candidate8dbde1e92a0b7587dbcd705de263d8f0d091f1c7 after complete interface/formula
review, independent compilation and unchanged old228 verification. This does not
approve hold behavior, future fixtures/tests, first red or production. The SIX
feature bodies remain immediate unconditional named stubs and were never invoked.

## Exact reviewed artifacts and preservation

| Artifact | Git blob |
| --- | --- |
| API document,324 lines |421acf29012f31345cc41cbee239bcf95000ad62 |
| Public owned-value header,331 lines |fe50fb10e6f5a7198df27358666734ad02fb47d1 |
| Immediate-stub CPP,30 lines |5b10c0e0b4e7057af4042ae510af851efcd24efa |
| Actual separate author evidence,102 lines |5217ec350c52a8dcf7e9b349dbfc948af2733071 |
| Root CMake |5d763585cc6f2f6dd7aa90cd89a006d587f50821 |
| Server CMake |b55b59124e657383051d69cda9dea27e4a6a1f4f |
| Presets |053b517accc0d769cc36d0980e7f4d3ee07ad0bd |
| SQLite workflow |88f8073ec9af73d21240f3df46c10d524d071cc8 |

Actual author APIac0610f and evidence77cddef imported into the preserved isolated
task/STORAGETEST016-review; four root-owned config copies used ONLY for verification
and left uncommitted. Root owns schema order/shared configuration/state. Only this
review document is reviewer-authored tracked content. All earlier reviews and
worktree verification copies remain intact. Relevant actual files equal root8db.

Read root/server/tests instructions, STORAGE016 brief, approved concrete701c710f/
reviewc0, physical83e96362/review696 (full89/F89890/9f5b61d4), accepted service6d0,
actual existing borrowed versus owning ports and integrated017. Inspected ALL API
document/header/CPP fields, operations, mappings, presence rules, formulas, author
note and four complete shared-config diffs. No old API/source/helper/test/fixture/
DDL implementation is changed.

## Interface, shape and formulas

- Subject is one value variant of typed CID or Receipt: ONE logical tag plus8-byte
  typed ID, global scope, CID0/MAX preserved and Receipt positive. No fallback
  receipt/scope namespace or guessed absence. Source, actual command, observation,
  plan/known-occurrence/failure/timer and version/link values retain approved
  physical mappings and explicit known/unknown/NULL groups.
- FailedCapture uses optional finalized amounts, explicit mode/extent/stopped_for
  and required alternative evidence; it cannot fabricate RequestedEffect0 or plan0.
  CapturedRevision retains actual complete selected provenance. Timer composition
  is optional in the DTO but REQUIRED engaged for valid timer data: no inferred
  global defer/apply policy. H11's kind0 old-Beside alias remains forbidden and
  kind1 requires its exact held origin/distinct correction. Policy/authority/
  entitlement, producer codecs and same-unit proof are not granted by data fields.
- The ownership graph is finite. View owns Cause once and first DetectionData,
  first/current Versions and optional current producer. Later current detection
  data is qualified by the SAME owned cause, not an unqualified borrowed locator.
  Absent current_event means exactly first=current with complete equality. History
  instead owns full Detection/cause or finite RecordedResolution. Resolution owns
  its request/replacement Detections and HISTORICAL produced versions, never
  sibling/current Views. No recursive batch→view→batch ownership.
- Normal mutation result owns historical versions/ref/kind ONLY for recorded or
  already_recorded; other dispositions have no outcome. Detection returns exactly
  one historical active version; resolution returns each closed entry's version
  followed by its replacement initial version if any. Exact replay never substitutes
  today's head or implies CURRENTly active. History ready alone has a page; absence/
  stale/cursor conflicts have none. Checked-head mismatch precedes cursor existence;
  has_more and optional next_position agree.
- Composed references validates ALL actual existing bundle options first. Common
  field/Exact bounds also cover hold fields; old reference-body caps remain separate
  from returned hold totals. No references.input_items cap on historical bodies or
  hold inputs, no fictional returned source/plan/state items. Hold vector limits
  cover each concrete vector, expected active locators, derived2*entry+1 produced
  capacity and every relevant max_size/reserved range; checked totals do not invent
  a combined vector capacity. Metadata remains quota-free finite-buffer work under
  the SAME actual writer budgets.
- Pure options/key/subject/count/body/matrix/ordering/owned-byte priority is explicit.
  Every submitted method's actual input bytes are charged, including find key,
  active Subject9 and HistoryQuery; this new hold rule is explicit rather than
  borrowed from017's different fixed selector. Per-entry existence/closed/expected/
  token checks follow full actual-set checking after SQL. Known selected failures
  require real whole-unit rollback; terminal/capability/protocol/unknown stays
  original WorkFailure, without a second cleanup of already-aborted primitives.

Compared EVERY B/I formula with the actual header members. Optional engagement
adds0, explicit enum/bool1, integers/IDs8, Subject one tag+ID9, canonical Exact counts
signed numerator plus denominator bytes. Fixed concrete event types have no extra
phantom kind; runtime event variants add1. No physical redundant CID/first-detection
column, implicit vector length, padding or internal reference DTO is charged.
First/current Version copies count separately. Current DetectionData avoids a
second cause; full history event/replacement copies count their actual nested
members EACH time. Normal mutation outcome does not echo its validated request.

Independent symbolic literal sums (one-byte refs/formats, empty documents/optional
DATA, failed-policy context, no API call): Source19, Cause29, Detection33, initial
Version12, initial View57/one-active66, later-detection View64. A replacement with
one known-prefix occurrence gives Batch61/RecordedResolution88/closed View149/
HistoryResult108/MutationResult30; corresponding I values3/5/5/6/2. This exercises
the finite nested/repeated ownership differences. Also independently checked stated
Version(h1,v1,producerd1)=15/two owning copies30, Subject9, Exact-3/1=3 and0/1=2.
These are layout arithmetic diagnostics, not frozen fixture or semantic-producer
acceptance. No blocking interface/formula finding.

## Actual stub/configuration and unchanged-suite checks

Static inspection/checker confirms SIX exact `throw SqliteApplicationHoldNotImplemented();`
bodies: factory/find/active/history/detection/resolution. No validation, SQL, DDL/
catalog data, equality algorithm, source lookup, mutation or error mapping exists.
Constructor ONLY moves the revocable session and copies limits. Implicit copy/
destruction is inert; no owner adoption/construction-thread authority/cleanup.

Root CMake adds hold flag→existing original flag BEFORE original→bundle dependency.
Server appends only static holds library linked to existing bundle/QtCore. Preset20
inherits19; removing new preset objects leaves every old preset object exact.
Replacing workflow20 profile/directory tokens back to19 restores originalc5d152f
BYTE-EXACT: all old dependency pins, permissions/check names, compiled guards,
mandatory diagnostics and unconditional stable suites remain. No Qt test target,
hold registration, skip, WILL_FAIL or product-suite relabeling was added.

Linux/GNU16.2.1/C++20, CMake4.4.5/Ninja1.13.2, Boost1.92, Qt6.12.0, host
QSQLITE3.54.0. Actual commands in the isolated reviewer tree:

- `cmake --preset sqlite-hold-debug`: exit0.
- `cmake --build --preset sqlite-hold-debug --parallel 2`: exit0, fresh102 steps,
  ZERO warnings/errors, including compiling/linking the new inert library.
- `ctest --preset sqlite-hold-debug --show-only=json-v1`: EXACT228 OLD registrations;
  names match the previous accepted228, no new hold case/disabled/failure inversion.
- `ctest --preset sqlite-hold-debug --verbose`: **228/228 PASS,7.88s,exit0**;
 225 Qt processes/715 passes,0failed/skipped/blacklisted. No new hold method or
  factory invocation. Old tests/source remain exact; this is old stability only.
- check_project PASS396 records/122 decisions/links/presets/current HTML;
  render --check and git diff --check PASS. No old initial red was replayed.

Ignored raw files under local-evidence/storagetest016-review:

| File | SHA256 |
| --- | --- |
| api-configure.txt |4cd235b49b7c1835ae0e938f36fb8b7547d0b39742f1ce2c9e2c1d0fcc9b357b |
| api-build.txt |abeef22c6cb44a01664356d5dae444fcb6c2313f1a8b246d0caa92b4cd7662c3 |
| api-discovery.json |c698fda5648d1e4db21f66d1191e5620fb78b6eb266df7f9cedf62416992f117 |
| api-old228.txt |97b7fffb40b955872deb954826cca6e1906cfdcdd7128144f22d2eebd64dbb1a |
| api-literal-byte-review.json |34ea4b477c0006629eab54bf0576c56893a7f70b7052d86b71ba2b816532e19b |

Own API-source selected official environment compilation/old stable CI is still a
separate next gate before independent frozen fixture/required-green authoring.
Actual feature author/full review/compiler/exact discovery/own environment/FIRST
intended marker red remain required. This review does not finish STORAGETEST016 or
authorize STORAGE016 production. Existing prefix/DDL/source/tests/reviews and all
service/policy/provider/financial/recovery/UI/platform/release obligations remain.
