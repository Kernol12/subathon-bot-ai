# STORAGETEST-002 feature baseline authoring

Author: `/root/plan_fixture_author`, 2026-11-02. **AUTHORED, not independently
approved, compiled, observed red or implemented.** This author ran no new bundle
entrypoint, Qt test body, native suite or historical red. Root owns compilation,
registration/CI, full independent review and any eventual first-red capture.

## Scope, authority and inputs

Dedicated clean `task/STORAGETEST-002-feature-author` worktree started at
`47ebdcae836a795a25ef1c27bebbae26bb16ce48`. Only the NEW
[feature test file](../../tests/qt/sqlite_application_bundle_tests.cpp) and this
NEW author evidence are owned. No support header was necessary. Production/API/
schema/contracts/fixtures/old assertions and root build/progress files are unchanged.
Latest owner scope is **finish existing PR42, then STOP**; STORAGE-002 production
implementation is not assigned after baseline integration.

Read root/tests/server instructions, workflow, task brief and actual handoff/review.
Source inputs are the accepted logical2f6e9a8e, physicalf13fca6d, header8e24f062,
seven-stub source83e9a0a7 and independent JSONe8b6c77e/helperb0a5c72f. Actual API
approvalef0ba764 and fixture approval5cf5dc2e plus selected official run41531137766
allowed AUTHORING only. Existing fixture/engine proof remains in
[fixture evidence](storagetest-002-fixture-author.md) and
[actual review](storagetest-002-review.md); it is not repeated here.

During drafting, the clock-only rejection category/cleanup was found unspecified.
Before adding its assertion I read the actual NEW12-line precision80f8ee65869a27c6b1b7b6d3819a0065777958b7
at e6e0be3 and actual separate approval42f8ef5354e068d45694c753088093506582ca4c,
then integrated by root70f0258. It explicitly requires post-SQL `source_mismatch`
after actual whole-unit rollback for a well-shaped supplied before snapshot that
violates the matched healthy persisted head's binding/clock-only conservation.
No unknown-cleanup truth is converted into a known error. Root owns that contract
integration; no shared contract was edited by this author.

## Oracles, setup and failure visibility

Expected owned DTOs are constructed from independently frozen INSERT literals and
explicit specification mutations, never new manifest/codec/store output. Helpers
compare ALL recorded Intent/three snapshots/contribution/effect/component/job/
transition/result/audit/reversal fields. Complete current contribution and support
records include activity identity AND verification provenance. Supplied cap/debt/
refund/timer/rounding outcomes are storage oracles, not an evaluator/permission/
provider or unresolved prior-debt refund-allocation implementation.

Qt init loads literals and establishes schema/intake/state through ONLY the existing
writer and real acknowledged BEGIN/COMMIT/normal completion. No new manifest/read/
stage function supplies setup or expected values. Real prior intake COMMIT precedes
application units. Extra65-row progress fixture commits its new synthetic source
records BEFORE its complete raw historical bundles. Test-only independent observer
mutations explicitly disable its own foreign keys/CHECKs for feasible corruptions;
the controlled writer keeps its existing ownership/SQL rules.

Every eventual body catches `SqliteApplicationBundleNotImplemented` explicitly.
Work wrappers retain that marker and rethrow it to the Qt assertion layer, rather
than treating it as a generic expected domain/storage error or crashing. Intentional
caller rollback requires completed `not_committed` (or explicit `uncommitted` for
abandonment), absent payload/typed errors/problem; an internal assertion exception
cannot masquerade as the desired rollback. Known local failures require an actual
independent BEGIN IMMEDIATE inside the catch and exact typed database preservation.

## Focused case/spec mapping

Authority: specification v0.45 §§3.3–3.5,4,8,16; DB-01/NUM-01–04; accepted AT01–21
and SB01–15. Exact32 slot names are in the source and were sent to root for its
owned `sqlite-bundle.` registration. The following groups cover all32 slots:

| Source cases | Contract oracle / observable boundary |
| --- | --- |
| `manifestMatchesFrozenStepFiveAndPrefixes` | Exact step5 SQL/full66/F62465 and unchanged prefixes; literal expected payloads |
| `initialStateAndEmptyAbsenceUseRealCallerUnit` | Initial empty/insert/duplicate/conflict; real caller rollback/commit; inert facade |
| `readInputsResolvesCurrentAndHistoricalProvenance` | Full selected capture/rate/source/current15dim/support activity provenance; bounded absence |
| `stateAndRecordedValuesOwnBytesAfterReopen` | Owned/reopened historical values ignore today's head/current-state changes |
| `explicitReceiptSubjectAndContributionZeroStayDistinct` | Healthy revision1 with actual Receipt/CIDMAX, correct live target patch; explicit receipt/CIDNULL role |
| `originalStageCommitsCompleteBundleAndPatchesOnlyApplication` | AT01/B08/SB01 whole records; all typed immutable prefix bytes/schema preserved except asserted CID0 application0→1 |
| `capDebtAndDeficitOutcomesPreserveSuppliedConservation` | AT01/06–10/SB03 signed partition, above-cap retention, three deficit policies and MIN without signed negation |
| `sameUnitReadDuplicateAndClaimRollbackOrCommit` | AT21/SB02 own provisional reads/duplicate; fresh-state-b subject claim; outside pending; abandon versus exactly one commit |
| `existingIntentComparisonIgnoresFreshComputedOutcome` | AT03/04 complete Intent versus a VALID changed computed outcome; no recomputation/upsert |
| `operationSourceAndReversalIdentityConflictsDoNotWrite` | Actual source/role and scoped reversal identity guards before freshness; healthy no-own-write outcomes |
| `staleStateContributionPlanAndTokenRebindRefuse` | SB07 all15 CURRENT cells, token/plan freshness, identical old-token reuse, applied-without-claim unavailable |
| `clockAdvancedBeforeIsDistinctFromPersistedHead` | Three-snapshot400→0→300 distinction; actually approved80f8 source mismatch/whole rollback |
| `sourceAndSelectedClosureFailuresRollbackBeforeCatch` | Typed retained/source/support references and missing actual basis; copied unit/target source mismatch |
| `fullLookupRejectsMissingCountsGroupsAndCorruptSelectedBodies` | SB12 complete result/audit/claim/history/children/count/group/body validation without repair; SB11 equivalent noncanonical Exact and class→size→grammar priority |
| `typedReferenceClassesNullsAndSparseMaxAreStrict` | SB08/11 actual classes/NULL groups/domain; sparse count3/headMAX explicitly invalid |
| `returnedAndInputBoundsHaveExactOneOverCategories` | Positive options, count/byte/combined bounds, selected-only returned vectors; no truncation |
| `invalidInputPriorityPreservesHealthyUnit` | Exact preflight priority before expired/foreign ticket; prior staged caller writes remain committable |
| `signedExactResidualAndAggregateConservationAreExact` | SB11 exact residuals, fitting >4096-bit storage, one-over component, aggregate overflow without wrap |
| `multiBasisRefundsUseOneCurrentRequestAndIndependentProgress` | SB04/13 one request/two originals;100/200 quarter/full;100/50→-30; repeated SAME basis at two current occurrences in one bundle |
| `zeroIncrementFullRefundAndStaleProgressStayDistinct` | SB05 cumulative0/1/1, increments0/-1/0/residuals, final zero/full provenance, stale position AND prior cumulative |
| `reversalBasisReadReturnsActualCreditAndTerminalKey` | SB15 real zero progress versus missing; complete selected credit/unit/source/target/last key; NUM-03/SB05 capped request600 versus actual credit120 and over-original130 refusal; unsupported debt-allocation conflict |
| `selectedSourceMetadataDoesNotCertifyUnselectedBodies` | Narrow source metadata ignores ONLY unselected job/Exact bodies; full lookup rejects them; missing sibling still fails |
| `pagedMetadataBeyondSixtyFourHasNoMaterializationQuota` | Actual contiguous65 job, plan and per-basis progress rows; strict selected return caps; malformed65th metadata cannot hide |
| `progressHeadPathsAndSparseSequenceAreValidated` | Both last-head paths/cumulative/head absence; two-row position1/MAX is INVALID before exhaustion |
| `directAmountMustMatchCopiedRequestEvenWhenDeltaBalances` | SB14 immutable request-20 versus component-1000 despite consistent100→-900 aggregate |
| `callerCommitAckLossKeepsUnknownWithPhysicalWholeBundle` | AT17 real control seam after physical COMMIT; whole durable data may exist, no normal payload/replay |
| `cleanupAckLossOutranksSelectedFailure` | AT18 actual rollback acknowledgement loss overrides selected cause; closed writer/no replay |
| `postCommitBodyFailurePreservesAccountingWithoutPayload` | Confirmed COMMIT then body failure versus later response failure; durable retrieval never reapplies |
| `genuineMidstageJobFaultRollsBackWholeCallerUnit` | SB09/B08 approved idle-observer trigger, actual conditional write prerequisites, inside-catch lock/snapshot rollback and identical fault-off commit |
| `cumulativeWriterBudgetsAbortObservedStaging` | Observed new refund staging, then explicit old-session600k read leaves too little of cumulative1MiB for required full lookup; whole rollback/no reset |
| `borrowedStoreLifetimeAndCapabilityErrorsStayTruthful` | Borrowed copies/destructor, actual owner/terminal/stale tickets, pure-before-thread, owner abandonment |
| `presentEmptyAndOpaqueEnvelopeFieldsAreLossless` | Owned BLOB/NULL/present-empty and BOM/NUL/FF operation/state/group/audit/document bytes |

No tiny sparse fixture certifies complete MAX history/position exhaustion. Full
financial policy/whole-count refund eligibility, semantic authorization, real clock/
job delivery/recovery/platform/live product gates remain their owning tasks; this
storage baseline preserves supplied provenance and never claims those gates complete.

## Independent thresholds and preliminary corrections

`python3 /tmp/storagetest002-owned-byte-oracles.py` read frozen literal bytes and
public logical weights ONLY (no SQL or product invocation). Input byte table:
Intent145 +advanced-before122 +after121 +advance41 +effects/components216 +jobs116
+result36 +audit41 = **838**; the test checks838/837. Returned current `start` state
is **112** =57identity/envelope/config +27timer/debt +28balances; check112/111.
Returned item counts are independently confirmed **20** lookup, **9** inputs,
**10** Main basis; invisible internal validation DTOs are not charged as returned
vectors. Source script SHA256 `2fed5831789272246fde86f9a86eb0e525d8f37af1fdcdde9eb14eb18476b5ed`.

Six material preliminary read-only findings were corrected before source commit:

1. MAX expected CURRENT state now comes from its OWN frozen row plus the explicit
   money update; it no longer copies CID0's different15 dimensions. Actual MAX
   application patch and untouched CID0 are separately asserted.
2. Expected caller rollback now rejects work_exception/any hidden assertion problem.
3. NULL decoder coverage uses an ACTUALLY nullable required quote-group member;
   CHECK-off does not pretend to disable receipt_id NOTNULL.
4. Complete current activity and support verification fields are asserted.
5. The captured prefix snapshot is actually compared, normalizing ONLY the separately
   asserted CID0 application delta; whole schema equality is also checked.
6. Optional presence uses explicit `.has_value()` where a bool parameter requires it.

These are draft test corrections, not specification changes, weakened reviewed
assertions or new observed reds. The actual complete candidate still needs different
full review; early feedback cannot approve later source by implication.

## Actual checks and remaining gates

Actual author checks: `clang-format --dry-run --Werror` on the new CPP;
`git diff --check`; static exact32-slot/no-skip/no-new-entrypoint-in-Qt-init inventory;
`python3 tools/check_project.py`; `python3 tools/progress.py render --check`. Passed:
391 tasks/122 decisions/current HTML. The Python byte-table derivation above ran;
no C++ compiler, Qt body, new entrypoint, old native suite or red ran by this author.

Source compile candidate is commit `eb5bab9423c52a3b3d1d9e0b274a1171954de259`, blob
`88e9b1b83056fb90420b7f238ebdf2f7d0a5c16d`, SHA256
`0a69e8756e16160c7be7733299a2df676a6ae9685a17138958be730b49813670`.
It was sent to root UNREVIEWED for compile/discovery while this note was completed.
Root records actual build feedback/registration/CI/red outcomes separately. Any
source revision must receive its own applicable review. Full actual review,
successful compile/exact discovery and selected official same-revision fixture
green remain before FIRST intended missing-behavior red. No implementation is
authorized by authored tests, and the owner stop boundary remains in force.

## First root compile attempt: Qt keyword setup correction

Root integrated eb5bab as8b59d78 and froze source/harness at
`9faabbec29528d69855ca402035cc80ea5afc6af`. Configure passed; the first target
build stopped in AutoMoc BEFORE C++ compilation because `Fixture::insert` used
local identifier `slots`, a Qt keyword macro. The diagnostic at source line694
was `Missing access specifier for slots`; root preserves
`local-evidence/storagetest002-feature-build.txt`. No binary, entrypoint, test body
or red ran. This is a build defect, not a feature red or changed storage oracle.

The focused correction renames that local and its uses to `placeholders`.
No assertion, field, fixture, production source or intended behavior changes.
New source blob `054acbc2d9da87b1b705fc558b4b3201a4acae98`, SHA256
`450f1141916279d642772f0d578603cf18eadc622e9e1eded59c4164cd4b8654`.
Scoped formatting/diff/project/render checks pass. Further actual compilation and
full independent review remain root gates; this author still ran no native/body.

## Actual full-review corrections before first feature execution

The different reviewer found two material defects in source054acbc2 (root's
warning-only `selected_state` rename gives source64678efb at c3e8e988) and one
selected Exact coverage gap. Root explicitly assigned these corrections within
the same two owned paths. Root reports that the preceding candidate's32 slots
compiled and registered; no new entrypoint or feature body has run. That prior
build does not verify this corrected source.

1. A600000-byte returned job does not imply its public lookup can fit a1MiB
   cumulative writer budget: the contract specifies no fixed SQL query/read cost.
   After actual observed refund staging, the test now executes exactly one old
   session SELECT of the known job and asserts its real BLOB class and all600000
   supplied bytes. This leaves at most448576 bytes of the1MiB budget. A subsequent
   full lookup must return that600000-byte body and therefore exhausts the same
   cumulative budget regardless of how it validates or materializes it. Separate
   staged/consumed/caught flags, typed `result_limit`, actual writer
   `WorkError::result_limit`, independent BEGIN inside catch and exact pre-unit
   snapshot preservation remain mandatory. No assumed first public lookup cost
   remains.
2. Renaming the original state token before its immediate foreign-key references
   was an unreachable positive setup. The corrected setup uses existing
   `add_state(head)` to commit a complete fresh opaque snapshot plus balances,
   then repoints the singleton head in another acknowledged old writer unit.
   Explicit counts retain the original immutable `start` snapshot and both
   balances. All BOM/NUL/FF, opaque key/group, owned bytes, present-empty and
   SQLNULL assertions remain; controlled-writer foreign keys stay enabled.
3. The existing full-lookup slot now independently corrupts the selected score
   effect whose actual immutable request is100. BLOB `200`/`2` must be rejected
   as noncanonical even though numerically equal. With `component_bytes=3`, a
   four-byte non-ASCII BLOB numerator plus valid denominator must return
   `result_limit` before grammar; the same oversized numerator with a TEXT
   denominator must return `invalid_stored_data` before size. The contract's
   explicit BOTH-classes/nonempty-shapes, BOTH-sizes, then grammar/gcd order is
   the oracle. All untouched literal selected plan/effect components are asserted
   to fit three bytes. Each case has its own database, feasible observer
   CHECK-off UPDATE, inside-catch rollback/lock proof and exact snapshot comparison.

The root `selected_state` warning rename is preserved exactly. Source for these
first three full-review corrections is
blob `682346f83893e093a2b49ea4ebf3ea67d0047fb3`, SHA256
`f3466734915136ce85b1539862cfe056b26c525283fe7a06b58443667502aee5`.

Actual author checks after these changes:

```sh
clang-format -i tests/qt/sqlite_application_bundle_tests.cpp
clang-format --dry-run --Werror tests/qt/sqlite_application_bundle_tests.cpp
git diff --check
python3 tools/check_project.py
python3 tools/progress.py render --check
```

All pass; project check remains391 tasks/122 decisions/current HTML. An inline
Python text-only inventory compared the35 Qt slot/lifecycle names against2ade878
byte-for-byte, confirmed32 feature slots, no skips/new entrypoints in Qt setup,
the preserved warning rename and all focused correction markers. No SQL, native
compiler, new entrypoint or body was invoked by this author. Different actual
full review, compilation and root's selected official green/first-red gates remain
pending for this source. No production implementation is assigned; finishPR42
then STOP remains the owner boundary.

## Fourth full-review correction: actual credited basis bound

The actual full-source reviewer found that prior ready-basis/refund examples all
had actual credit equal to requested units. A getter returning the request, or a
stage omitting the cumulative<=actual-original check, could escape those assertions.
Root explicitly assigned one final correction within the existing basis slot;
no additional slot, API, fixture or production change is involved.

The new independent no-debt original has initial Main1000, request600, actual Main
credit120, cap-limited480, cap1120 and after Main1120. The actual basis getter must
return120 while preserving its selected effect's requested600 and complete supplied
120/0/480 partition. An actual captured current refund request is separately changed
to-130 before any application unit. The proposed refund matches the fresh head,
complete current state and actual selected capture: Main1120→990, debt0, components
-130/0/0, expected prior position/cumulative0, fraction1, entitlement120, residual-10,
increment-130 and full flagfalse. Pure shape, local signed conservation, selected
request equality and exact entitlement/residual algebra remain valid. Cumulative130
is less than requested600 but exceeds the ACTUAL credited120, so it must return the
normal no-own-write `reversal_conflict`, never a pure-input/storage error or staging.
These intentionally invalid reversal amounts test the storage bound; they do not
claim valid financial refund eligibility or choose an upstream rounding policy.

The caller inserts an earlier marker, obtains the normal refusal, remains usable
and actually commits. Its exact durable BLOB bytes prove no facade cleanup erased
caller work. A subsequent acknowledged old-writer unit removes ONLY that marker;
the complete typed schema/data snapshot must then equal the pre-refusal snapshot.
Any changed application/source/progress/state byte remains observable. This proves
no own writes while preserving the successful original and its real120 basis.

Inline Python stdlib `Fraction` arithmetic independently checked:

```text
600 = 120 - 0 + 480
1000 + 120 = 1120
1120 - 130 = 990 >= 0
120 * 1 = 120; 120 - 130 = -10
-(130 - 0) = -130; 120 < 130 < 600
```

Final source blob `4b60fcaa06dec8136f0b81e7ac6d01374df94a48`, SHA256
`6f84dfa02c0a6ad2dfccf1d6c87b004cb188678ca439a43b35858a59467a0b19`.
Scoped clang-format/diff/project/render and unchanged32-slot/setup-only inventory
checks pass. Only the owned CPP and this note changed. No SQL, compiler, native
suite, new entrypoint or feature body was invoked by this author. The actual final
source still requires root compilation/discovery, different full-review verdict
and the gated first intended red. Finish existingPR42 then STOP; production
implementation remains unassigned.
