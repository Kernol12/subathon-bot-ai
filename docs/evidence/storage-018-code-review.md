# STORAGE-018 independent SQL implementation review

Reviewer: `capture_reviewer`, different from the SQL source author and additive
regression author. Review scope is the actual caller-session SQL implementation,
its selected closure/resource/error behavior, and bounded test registration. This
note does not approve unwritten fixes or infer correctness from passing tests.

## Initial candidate and findings

Reviewed root `c06e6f03fdf4c6301cd41707df1092793bc2b016`, authored source
`ce8d4d7`/import `54967ed`, SQL CPP blob
`46c0e254d140bc5176f74cede141236d6705e533`. The existing 31 SQL methods/51
scenarios passing and the old 337 checks passing did not establish source approval.
The original fixture, literal codecs, API and schema contracts remain the oracles.

The audit follows initial-capture contract sections 2, 4 and 5, consumer section 2,
and the existing receipt/contribution/application-plan contracts. Confirmed defects
in the initial source were:

1. A present unindexed request with noncanonical `0600000/1` reached candidate
   comparison and returned `source_mismatch`; its own malformed structure requires
   `invalid_stored_data` and actual whole-session cleanup first.
2. Intrinsically invalid plan valuation (resolved quote without actual event money,
   or missing-historical valuation with a dependent request) similarly reached
   comparison instead of stored-data failure.
3. Applying nonempty requirements to every imported reference rejected valid
   present-empty rule/parent bytes. Imported opaque fields retain the existing
   contracts; only actual required identities/audit fields require nonempty bytes.
4. A generic healthy two-rule contiguous collection was classified as corruption.
   The agreed bounded interpretation first validates selected count/ordinal/type
   metadata, then refuses unsupported cardinality as unindexed `source_mismatch`
   (indexed mismatch is corruption). It does not own or certify unsupported generic
   child payload collections. Supported 1/0/1 still requires complete validation.
5. Adding physical SQL row sizes imposed a hidden smaller quota, including storage
   keys/ordinals and duplicated fields rather than the complete selected semantic
   DTO. Actual optional groups also require one logical presence byte each.

No correction was authorized on these findings alone. Separate regression
authorship, actual program/configuration review and reached behavior red preceded
the corresponding bounded source corrections.

## Reviewed structural regression and observed red

Separately authored `7a169d0` plus `bf92893`, root imports `27991bd` and `9507c97`,
add four methods/eight scenarios to the original 31 methods/51 scenarios without
changing the prior assertions or fixture. Reviewed actual CPP blob
`ec0f58e11669d08885d3918e1aff426f9e836c4b` and root harness `87a6901` before
execution. All scenarios use old committed intake/plan/SRX primitives and no
index for the candidate. They cover malformed versus healthy different canonical
requests, both intrinsic valuation defects, empty rule/parent references, and
healthy two-child versus corrupt ordinal shape. Semantic failures check whole
cleanup inside the catch and an independent connection's actual `BEGIN`; healthy
refusals check the caller's marker remains active before host rollback.

The CMake registration and compiled-function guard were 35, enabled and still
`acceptance;draft-red`; ordinary CI did not invoke the new SQL bodies. These
program/configuration gates were explicitly approved before root's targeted run.

Independently read root's raw `/tmp/storage018-structure-red.txt`, SHA-256
`a4944334a59e8acb40ccf00b186de833969135367ec58c1bb47e7b5400abef97`.
At clean `87a6901` with held CPP `46c0e254...`, root observed direct exit 8:
six reached wrong-behavior failures, two already-correct controls, eight
init/cleanup passes, zero skipped/setup/stub failures. CTest's repeated failure
transcript is not a second execution. This is a valid new behavior regression red,
separate from the preserved historical FIRST-red baseline.

Structural correction `8f5a7d7`, imported root `98c252e`, actual CPP
`8702843f4a52b8ef6c662cf78c53a117408ef1e1`, was subsequently inspected. It adds
required-reference roles, canonical decoding before comparison, actual
header-receipt valuation checks and bounded child metadata classification. The
author's passing 35-method/59-scenario run is recorded by the author; final source
approval remains pending the resource and scalar preownership issues below.

## Independent semantic resource derivation and observed red

Consumer section 2 counts declared field/copy bytes, enum/bool 1, 64-bit scalar 8,
one optional presence byte plus its contents, and exact canonical component bytes.
Independent fixture-A arithmetic from public core/server DTO declarations is:

| Declared value | B |
| --- | ---: |
| ReceiptIdentity | 25 |
| ReceiptFacts: event type 30 + schema 4 + six absent optional tags | 40 |
| ReceiptArrival: UTC 8 + Q/C/X | 11 |
| PlanEvent: V, absent parent, V, receipt, identity/facts/arrival, present CID0 | 96 |
| ContributionState including complete conditions | 12 |
| PlanValuation with absent quote | 2 |
| One RuleSelection | 33 |
| One RequestedEffect | 70 |
| CapturedPlanRevision: revision 8 + absent parent + capture/rules + absent override | 223 |
| SqlitePlanRevisionSnapshot: plan_ref P once + revision | 224 |

The other selected fixture-A DTOs are at most 186 bytes, including the retained
role-envelope payloads. The old sum of header/receipt/support/revision/rule/request
physical rows was 279; it cannot replace the declared snapshot projection. Budget
240 fits the complete selected DTOs; 222 is below even the mandatory revision 223.
No implementation output was used to derive these expected values.

Reviewed separate author candidate `aea8e7c`, CPP
`6d832b975a4c248acd21b0520f040feec54c6510`, and actual root harness `807e1d3`
before execution. It adds one two-row indexed healthy-A test (36 methods/61
scenarios total), preserves all 35 prior methods, and checks exact find plus
`already_present`/normal commit at 240, versus known `result_limit`/actual whole
cleanup at 222. Configuration only adds its enabled draft-red registration and
changes the compiled-function guard to 36; no automatic SQL feature-body lane.

Independently read root's raw `/tmp/storage018-budget-red.txt`, SHA-256
`3d114490a862719f56610f6af740a8ff5354bb6b7d0afadc938e5791b067f7b2`.
Root's actual clean `807e1d3`/held CPP `46c0e254...` run at 20:43:58Z exited 8:
the healthy 240 row reached a storage refusal, the 222 overage control passed,
plus two init/cleanup passes, zero skipped/setup/stub failures (98 ms). This is
valid behavior red for the hidden quota. Only afterward was field-by-field typed
resource correction authorized. The new formula is not yet approved here.

## Remaining scalar preownership finding

At actual structural candidate `98c252e`, `children()` selects raw
`min(ordinal)`, `max(ordinal)` and, for requests,
`max(requires_historical_valuation)` alongside a malformed-type count. A corrupt
TEXT/BLOB may therefore be copied into Qt before the type count is examined.
The metadata path must return only bounded fixed scalars even for malformed
storage; a writer result-byte refusal cannot substitute for selected known-shape
corruption. The raw ordinal aggregate existed initially; the dependency aggregate
is new in the structural correction. Root and source author were notified.

A separate focused test/review/reached-red gate is required before correcting
these aggregate expressions. No final source/configuration approval is given by
this interim note. The historical reviewed baselines, valid review records and
FIRST-red evidence remain valid and must not be replayed or rewritten.

## Reviewed metadata regression, reached red and resource delta

Separately authored metadata candidate `d75738b`, CPP
`35c0bacbb4b22d156d7e9ea38d317a3d92df4b5f`, adds one two-row method while
preserving the earlier 36 methods/61 scenarios. The actual program and root
configuration `45533d52f251131abf143304b391eee11666385b` were independently
approved BEFORE invocation. Configuration adds only the new method and changes
the compiled-function count 36 to 37; enabled draft-red/no inversion/no ordinary
SQL-body execution remain intact.

Each row first proves healthy complete A find/normal commit under the SAME generous
65536-byte writer result profile. The existing disposable Observer alone then
enables ignored CHECK constraints, writes one literal 262144-byte BLOB into either
rule ordinal or request Boolean, restores CHECK enforcement immediately (including
the UPDATE failure path), and asserts restoration. Metadata-only typeof/length
queries establish the fault without returning its payload. After reopening with
the identical profile, the NEW call must report selected corruption and actual
whole cleanup before publication.

Independently read raw `/tmp/storage018-aggregate-red.txt` and its XML, SHA-256
`6949ac096d92904ea204c6b242c0a769f24fae27ec29ab52bffd974493f8884f`.
Root confirms clean `45533d5`, CPP `8702843...`, direct exit 8 at 21:02:22Z:
two reached wrong-known-reason assertions, two init/cleanup passes, zero skipped/
setup/stub failures, 109 ms. Both healthy controls and fault validation completed
before those assertions. The raw log does not print the actual enum value; the
source's raw aggregate and writer bounds identify the preownership problem. Only
after this reviewed program/configuration and reached red was bounded CASE-guard
correction authorized. Every aggregate argument must remain fixed INTEGER/NULL,
while malformed counting still examines all rows, including mixed valid/bad rows.

Actual resource correction `da96727`, CPP `27fa987...`, was independently audited
as a bounded delta from `98c252e`. Its checked field-by-field projection matches
the public declarations: receipt's six logical optionals; support's full actual
original receipt plus kind/evidence/verified-link/conditions/associated CID;
retained kind/key/document/optional configuration; plan-ref once plus the complete
captured revision with actual header-receipt identity/facts/arrival, state,
valuation, rule, request, parent and override presence. It derives fixture-A 224
without hardcoded fixture sizes or arbitrary subtraction. Supported 1/0/1 gathers
all metadata and checks complete snapshot B before plan header/revision/child
payload ownership. Per-row SQL buffer checks do not impose a smaller bound: their
storage keys/scalars are dominated by the corresponding full semantic projection.
This resource delta is acceptable; it deliberately leaves raw aggregates unchanged
for the separate gate and is not approval of the complete source yet.

## Intrinsic activity/anchor finding and program review

The initial/structural candidate checks optional activity groups and their nonempty
members but reaches narrow-profile absence comparison before enforcing the old
contribution contract's intrinsic origin and original-support compatibility rules.
The contract requires any present activity identity to have its ACTUAL original
receipt origin, and the linked original support's complete optional verified key
must match the contribution's optional identity. These are already required by
old `validate_observation`, `read_observation`, `contribution`, `compatible` and
`anchor_matches`; no current mutable state or additional reader is needed.

Separately authored actual `2a07637`, CPP
`5c4dd48162d224220cae5c0a95813e7dc9608136`, adds a compact four-row method,
preserving all earlier 37 methods/63 scenarios byte-for-byte (38/67 total):

- Both tuples match actual receipt origin: healthy generic `source_mismatch`,
  caller remains active until host rollback.
- Both tuples match each other but have the wrong actual origin: corruption,
  independently testing origin even with a compatible anchor key.
- Support's origin alone is wrong: corruption.
- Both origins match actual receipt but the activity IDs differ: corruption,
  independently testing full anchor key compatibility.

Both tuple groups and proof are literal complete nonempty BLOBs. OLD setup joins
verify actual original/observation receipt7 and both real receipt origins, support
kind0 and evidence3. No index is present. Healthy refusal checks provisional marker
visibility and actual host rollback; malformed rows require exact stored-data
failure and independent lock/marker cleanup INSIDE catch. This program was
approved before invocation. Actual root registration/reached red and the subsequent
bounded source correction remain pending in this entry. No source author has
permission here to correct unobserved behavior or weaken these assertions.

## Origin/anchor reached red and final source approval

Actual root `d18e15c0caf75d3c172649c0f33c8f56fe2751c6` contains the exact
approved four-row program and 38-case configuration. The independently reviewed
configuration changes only the method registration/compiled count 37 to 38 while
retaining enabled draft-red behavior. Public header changes are comments only,
accurately describing the pure manifest and provisional caller-session results.

Root's targeted run at clean `d18e15c`, source `27fa987e1cd25366e2538077b5c822d44fb51c96`,
21:06:38Z exited 8. Independently read raw `/tmp/storage018-origin-red.txt` and XML:
SHA-256 `f97639410d5189d6a94c91b5e7b02275aef5a3febc5d76d263fdfb5edfa6ea72`.
The healthy matching-key/origin control passed; three corrupt cases returned
without the required stored-data failure and reached the `caught` assertions.
Two init/cleanup passes, zero skipped/setup/stub failures, 198 ms. This is valid
new regression red after actual program/configuration approval, before the bounded
origin/anchor correction. It does not replace any preserved historical FIRST red.

Actual aggregate correction `1d54acc`/root `cce251e`, CPP
`f8013f49b2aa3f6a5a71e2c88639c5fd3e52fe8d`, separately passed source inspection:
all min/max ordinal and dependency aggregate arguments are now CASE-guarded to
fixed INTEGER/NULL, while the independent malformed count still covers all rows.
Domain-invalid integers remain fixed scalars and are rejected by that count;
mixed malformed/valid rows cannot hide behind a valid extreme or dependency.

**Final source APPROVED:** authored
`90eeca7b9054fd369dd576127970a458d87e394e`, root import `c0ff1ec`, SQL CPP
`ccdc4f616529aedc310ffc75259a79f7690239b5`. The final 15-line delta checks both
optional key presences, both origins against the ACTUAL original support receipt,
and all four identity components before narrow-profile comparison. When the
contribution's original receipt differs from the proposed receipt it uses the
already validated actual target, not candidate identity. Support receipt/CID
association and structural groups/domains are validated first. Healthy matching
generic activity remains a caller-preserving `source_mismatch`.

The complete source audit also confirms exact indexed source5/receipt2 ordering,
bounded2-row ambiguity checks, original-only receipt/support reads, retained known
roles/private decoders, revision1 without current HEAD/history/dimensions, canonical
exact components, checked semantic projections before relevant ownership, guarded
payload queries, one immutable INSERT, and honest actual rollback/uncertainty/
capability/allocation behavior. No owning repository, secondary writer, hidden
BEGIN/adopt/COMMIT/retry, authority/evaluator/provider or repair capability is added.

Independent verification in fresh `/tmp/subathon-storage018-final-review`:

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug -j4
ctest --preset sqlite-reference-capture-debug --show-only=json-v1
ctest --preset sqlite-reference-capture-debug --output-junit /tmp/storage018-review-final.xml
clang-format --dry-run --Werror server/src/sqlite_reference_capture.cpp
git diff --check
```

Fresh configure/145-step build at `cce251e`, then fast-forward to exact `90eeca7`
and rebuild changed source: exit0, zero compiler warnings. GNU16.2.1,
QtCore/Sql/Test6.12.0, CMake4.4.5, Python3.14.9, local Boost1.83 SDK.
Compiled/registered SQL function sets equal38; total375, zero disabled/inverted.
Actual final full run exit0: **375/375 PASS, 28.32 s**, zero failed/disabled/skipped.
Its SQL38 methods/67 scenarios total143 Qt passes (67 rows +76 init/cleanup),
zero FAIL/QWARN/skip. The complete LastTest.log also has zero FAIL/QWARN.
This is one final passing implementation run, not a historical marker-red replay.

| Independent raw artifact | SHA-256 |
| --- | --- |
| `/tmp/storage018-review-final-ctest.txt` | `85920b4f7d167fd7a77384ef16b3396d9f108045df95caedf7c0536ea6568c13` |
| `/tmp/storage018-review-final.xml` | `83cfe31a6cb70da458d6e323cf767b03cee23e614a89af95c9708cde2f431d52` |
| Final review tree `Testing/Temporary/LastTest.log` | `95e94565ec428cefd190352366efd67b033c1f565bf4c1ad683fabf207451605` |

Independent byte reconstruction removes ONLY the seven additive methods and
reproduces original SQL CPP `58b22366b2f853125fd3a79fe9146d2283ccd1ec` exactly:
all original31/51 assertions remain unchanged. Literal fixture remains
`878005d95a0c12d69df6b836ac09cd59ba5168ba`; structural codec CPP/private header
remain `fa3fc6d1775606604129522f6e071fd76318c70b` /
`9fc608d42062ad54525d47bbdc911e55bfc99028`.

## Ordinary SQL CI configuration approval

**Configuration APPROVED:** actual root `b4a33e1b2e27337420d3cde067d0481805fa43a5`,
CMake blob `89260636a602204f6412ed447ff52358bad0cab9`, workflow
`c36d0a992d5f25a0ec5f40d34f7c697e2da76842`, presets
`daab875da22ab766c785809894e941a564826e27`. It promotes exactly all38 reviewed
SQL cases by removing draft-red; preserves enabled registration/timeouts/acceptance
labels; retains compiled/registered equality38, no DISABLED/WILL_FAIL and explicit
no-draft-red guards; adds unconditional selected SQL38 CTest with normal failure
propagation. Codec28, old lanes, pins and check names remain unchanged. Preset
display text is descriptive only. Source/tests are identical to the approved
final source/program. This review follows the actual passing implementation run.

Selected official CI execution, root's implementation handoff/status metadata,
latest-head checks and normal PR merge are still separate coordinator gates. This
component approval supplies no full product/release, live provider, permission,
API adapter, recovery, power-loss, platform or deployment verification.

## Initial publication metadata review

Reviewed actual published `fe440ac` (metadata author `e303f0e`) in fresh isolated
`/tmp/subathon-storage018-metadata-review`; no production/test/fixture changes are
introduced by this metadata. Final SQL CPP remains `ccdc4f6...`, with the exact
source/configuration approvals above preserved.

Independent JSON comparison against integrated main `ba66804` confirms all422 task
IDs and all1619 dependency edges unchanged; only the STORAGE-018 task row differs.
Its status is `in_review`, local acceptance/source review passed, and selected CI
is `not_run`, consistent with the ordinary workflow still pending at this review.
DB-01 and NUM-04 remain `partial`; the document-only session prerequisite remains
planned in this branch. No other active task/review/owner/dependency was overwritten.
Architecture/test-strategy/map describe the implemented component and separate
remaining authentication/producer/authority/provider/recovery/platform/release work.
The four committed raw regression logs hash exactly to the previously reviewed
records above. `python3 tools/check_project.py` and
`python3 tools/progress.py render --check`: exit0 (422 records/current HTML).

The initial status/scope/evidence claims are acceptable, with two bounded wording
corrections requested from the coordinator: specify *mapped storage/corruption*
failures in the handoff's whole-cleanup statement (pure input/original capability
errors retain their distinct contract behavior), and call the four table entries
*regression families*, rather than four individual gaps. The structural family
contains several distinct findings. `configuration_review` may now be recorded
passed from the actual b4 approval above. The coordinator checkpoint is known
stale in this candidate and is being updated separately; it is not approved here.

No unknown selected-CI outcome, task-done status, completion metadata or latest-head
merge gate is approved by this initial review. Those require the actual receipt
and subsequent exact candidate review.
