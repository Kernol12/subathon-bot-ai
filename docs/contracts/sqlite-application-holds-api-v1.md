# SQLite application holds — minimal API proposal revision 1

STORAGETEST-016 MINIMAL API/STUB phase after actual concrete approvalc0/701c710f
and final physical approval696d7f9/83e96362. Bind [concrete](sqlite-application-holds-v1.md)
H01–H16 and [physical](sqlite-application-holds-schema-v1.md) EXACT step6/full89,
F89890/SHA2569f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a.
Earlier972/e094 drafts are superseded. Header: sqlite_application_holds.hpp.
All SIX new callable feature entries are currently immediate unconditional throws
of ONE unique SqliteApplicationHoldNotImplemented marker. Constructor/copy/destructor
are inert. NO validation, equality algorithm, SQL, DDL/catalog content, reference
resolution, mutation or cleanup is implemented. Actual API/configuration review
precedes independent fixtures/tests; first red/production remain later gates.

## Concrete public boundary and reused values

Use existing ApplicationOperationKey, ApplicationPlanKey, ApplicationEnvelope,
ApplicationComponentKind, typed ReceiptId/ContributionId, RequestedUnit, Exact,
RefundDeficitPolicy/TimeReversalDisposition and SqliteWorkSession/MigrationManifest.
Known historical state is validated using existing ApplicationStateSnapshot semantics,
not a new timer/state DTO. It is referenced, not duplicated into this return layout.
No existing header/interface/renderer/core/provider schema is changed.

ApplicationHoldSubject.id is a value variant: alternative0 ContributionId maps
existing ApplicationSubjectKind::contribution0; alternative1 ReceiptId maps receipt1.
This is ONE logical tag plus ONE typed ID, not a duplicated enum/raw scalar pair.
CID0..MAX is valid; receipt1..MAX is required. There is no operation scope, fallback
receipt or absent-subject default inferred from a missing CID. Fixed selector types
from017 have different semantics and are not reused as a subject namespace.

| Store call (all const) | Concrete owned result / exact normal presence |
| --- | --- |
| find(const string& hold_ref) | optional<ApplicationHoldView>; absent only if anchor absent; current view after full selected closure |
| active(const ApplicationHoldSubject&) | ApplicationHoldActiveSet{subject,holds}; ALL current active views, or error; legitimate empty set has holds.empty() |
| history(const ApplicationHoldHistoryQuery&) | ApplicationHoldHistoryResult{disposition,page}; ready0 has a page (possibly empty at tail); missing_hold1/stale_history2/cursor_conflict3 have NO page |
| record_detection(const ApplicationHoldDetection&) | ApplicationHoldMutationResult; recorded/already_recorded owns one HISTORICAL produced active version, not today's current view |
| stage_resolution(const ApplicationHoldResolutionBatch&) | same result; recorded/already_recorded owns each entry's HISTORICAL resolved version, immediately followed by that entry's replacement initial active version if any |
| application_hold_repository_manifest() | future pure MigrationManifest factory: exact unchanged prefix1–5 plus approved12-statement/full89 step6; currently immediate marker, no catalog/DDL data supplied |

Mutation result disposition is recorded0, already_recorded1, mutation_conflict2,
hold_conflict3, stale_active_set4, stale_version5, version_conflict6, missing_hold7,
closed_hold8 or replacement_conflict9. Outcome is present EXACTLY for0/1 and owns
mutation_ref, actual mutation kind and ordered produced_versions. Other dispositions
carry no historical payload/current view. Detection can return0/1/2/3/5/6/8;
resolution can return0/1/2/4/5/6/7/8/9. All healthy refusals make no own writes/cleanup.

History query owns hold_ref, signed64 after_position (initial0), optional checked
VersionRef and positive page_count (default64). Actual checked-head mismatch gives
stale_history BEFORE nonzero cursor existence; a nonexistent cursor gives cursor_conflict.
Ready page owns hold_ref, actual checked_version_ref, ordered entries, optional
next_position and has_more. next_position is present EXACTLY when has_more and
names the last returned actual position; no hidden continuation/truncated-clear.

## Finite ownership and historical versus current data

The header's members are the complete public layout; no borrowed buffers/pointers
or shared ownership/lifetime extension is involved. ApplicationEnvelope documents
must be non-null owned bytes. Optional absence differs from present-empty DATA.
Required keys/format/FK refs are nonempty; optional key/FK refs are nonempty when
present. Optional event/action/version/rule/target/explanation/reason fields retain
present-empty where the physical domains permit it. No optional flag grants authority.

- Cause owns source/command/first observation/plan evidence and kind-specific data.
  Plan evidence owns ProposedOccurrence vector. CapturedRevision/FailedCapture
  matrices and extent/stopped_for/numeric-stage rules are EXACT701/83e, not selected
  by defaults. Failed finalized amounts are optional and stay absent; ProposedOccurrence
  deliberately is NOT RequestedEffect. Timer composition is optional in the C++ DTO
  but REQUIRED explicitly in every valid timer group; absence is invalid, never a
  default global defer/award choice. Numeric fixed RequireReview/RequiresReview and
  component4 have their reviewed meanings, not an authorization capability.
- A qualified Detection input is Cause plus DetectionData{mutation_ref, expected/
  proposed versions, attempt observation,audit}. The stored first_detection_ref is
  the creating mutation's ref, not a mutable field copied from a later detection.
- ResolutionBatch owns subject, expected_active VersionLocators and entries; each
  optional replacement owns one full Detection/cause. RecordedResolution owns that
  full request plus its produced_versions. It NEVER owns sibling/current HoldViews.
- HoldView owns Cause, first DetectionData, first_version, current_version and an
  optional current_event. This avoids repeating the entire cause merely to qualify
  a same-hold detection. current_event is ABSENT exactly when current is the first
  produced version; both returned versions must still fully agree. Otherwise its
  variant alternative0 is current DetectionData qualified by this view's cause,
  or alternative1 is the full finite RecordedResolution. The actual current version
  must equal this producer's matching historical produced version. Metadata locators
  are scalar; no view→batch→sibling-view recursion or current-head substitution.
- HistoryEntry owns its selected historical Version plus event variant0 full
  Detection/cause or1 RecordedResolution. A page can therefore repeat a cause or
  event's owned data; those ACTUAL repeated members count EACH time. Resolution
  output/version duplication also counts each owning copy, not a hash/ref deduction.
- MutationOutcome owns ONLY historical produced versions/ref/kind. The adapter must
  validate the complete stored actual-kind request before retry comparison even
  though that complete request is not echoed in this small normal result. A replay
  can return an old active produced version after the hold is currently resolved;
  it never reports CURRENTly active, reappends or reactivates it.

## Exact physical mapping and branch groups

Enum values are explicit storage values, never C++ sizeof or enum-name spelling:

| C++ enum | Mapping |
| --- | --- |
| ApplicationHoldCauseKind | numeric_overflow0, timer_deficit1, unresolved_policy2 |
| ApplicationHoldPlanKind | captured_revision0, failed_capture1 |
| ApplicationHoldExtent | captured_complete0, proposed_complete1, known_prefix2 |
| ApplicationHoldStoppedFor | numeric_conversion0, unresolved_capture_policy1 |
| ApplicationHoldFailureStage | final_conversion0, aggregate_after_finalization1 |
| ApplicationHoldComposition | defer_whole_input0, beside_complete_application1 |
| ApplicationHoldMutationKind /State | detection0,resolution1 / active0,resolved1 |
| ApplicationHoldApplicationLinkKind | whole_input0,timer_correction1 |

Existing unit0/1/2, deficit policy2/disposition1 and unapplied component4 remain
unchanged. Version.position/parent/state/producer/replacement map the8 physical
version columns. Position1 has no parent; subsequent parent is actual previous
VersionRef; no version follows resolved. Heads/counts are validated metadata and
are not an extra owned public authority/count field.

| Relation / projection | Exact member source and constraints |
| --- | --- |
| causes | Cause.hold_ref/source/command/kind/supersedes; PlanEvidence kind/plan/extent/stopped_for/failed_capture/occurrences; optional FailureEvidence/TimerEvidence/missing_policy_reason. contribution_id is derived typed subject projection; first_detection_ref derives from actual creating mutation. Neither is an independently writable duplicate input |
| cause_observations | Cause.first_observation fields, including optional known persisted_state_token, individual actual refs/times and required annotation |
| occurrences | ordinal plus optional action/action_version/rule/target/unit/attempted Exact/finalized signed64/explanation; implicit parent HoldRef is not duplicated as an owned field in every occurrence |
| mutations/detections/detection_observations | DetectionData and parent Cause provide raw shared mutation_ref/kind0/subject and counts0/1; attempt observation/audit required. Missing/extra opposite-kind counterpart is selected corruption, not another namespace |
| mutation resolution group | ResolutionBatch mutation_ref/kind1/subject, expected_active/entry counts, required nonempty actor_ref/decision_ref, non-null reason DATA (empty allowed), options envelope and optional application link |
| resolution_expected/entries | exact vectors in strict unsigned HoldRef order; entries own expected/proposed versions, decision and optional replacement. Replacement SQL pair derives from replacement.cause.hold_ref and replacement.request.mutation_ref; no separately editable redundant pair |
| versions/heads | full produced Version data and exact head/position/count/parent/producer metadata; replacement locator is a scalar pair, never another owning Detection/View |
| optional application group | ApplicationLink.kind/operation/receipt_id/plan are a whole group; actual complete bundle matches these typed fields. WholeInput has no held_origin; TimerCorrection requires HeldOrigin.version/request/component4 plus optional known remaining_units |

FailureEvidence numeric stage/known failing ordinal/target/Exact/annotation is a
WHOLE optional group: required for numeric overflow, absent for timer/policy causes.
TimerEvidence is required ONLY for timer kind, absent otherwise; full negative
request/unapplied values, explicit policy/disposition/composition/decision required.
Beside composition requires associated OperationKey/request pair (same held request
ordinal); Defer composition forbids it. UnresolvedPolicy requires non-null
missing_policy_reason DATA and no numeric/timer/associated group. FailedCapture
requires stopped_for/evidence envelope and proposed_complete or known_prefix; a
present optional prior PlanKey still receives FULL historical source/plan closure.
CapturedRevision requires actual positive PlanKey, captured_complete and known
observation token. No permissive optional plan circumvents required alternative
evidence or manufactures finalized amount0/state/head/plan revision0.

Application-link kind0 is REFUSED if ANY closed timer cause was BesideCompleteApplication,
even when the old associated bundle is fully valid and shares its subject/source.
Kind1 requires a closed timer entry's exact expected version/held occurrence and
DISTINCT complete original receipt-subject/no-CID correction key/source, not replay
of the associated original. All source/context/support/plan/state/link crosspaths
and incoming missing_reference/source_mismatch versus selected invalid_stored_data
follow83e96362. remaining_units is optional signed DATA; storage does not calculate
entitlement or choose partial/full continuation/reject/defer policy.

## Bounds and pure input order

SqliteHoldLimits.references composes the ACTUAL public SqliteBundleLimits, not a
fake getter or owning adapter. Its reference_bytes4096/value_bytes262144/
component_bytes16384 ALSO bound corresponding hold fields/Exact components. All
its options, including dormant old count options, are validated FIRST. Its other
per-vector/result/owned ceilings bound selected OLD reference materialization:
plan children, snapshot balances and complete linked application bodies. Those
internal checked reference values do NOT add fictional hold result items/bytes;
references.input_items is NOT applied to historical read bodies or hold inputs.
Metadata64/65 does not acquire any lifetime quota from either profile.

Hold-specific defaults are occurrences1024 per cause, active_holds_per_result256,
resolution_entries64, history_versions_per_page64, input_items4096,
result_items8192 and owned_bytes1048576. active_holds_per_result also bounds a
submitted complete expected_active vector and full selected resolution expected-set
body; resolution_entries bounds each input/selected full batch; historical metadata
producer counts are audited independently of those full-body caps. Each replacement
is an optional single Detection; its own occurrence cap applies independently.
No actual active set can truncate to its ceiling or be mistaken for clear.

Validate ALL composed options and hold count ceilings before input/ticket access.
Reserve n+1 below qsizetype/qint64 and EVERY relevant concrete vector.max_size:
occurrences→ProposedOccurrence; active ceiling→HoldView AND VersionLocator;
entries→ResolutionEntry; page ceiling→HistoryEntry. Checked2*resolution_entries+1
also fits the produced Version vector/qt/signed ranges; it bounds the maximum
entry+replacement output count. Combined input/result totals fit checked size_t,
without inventing a shared aggregate vector capacity. owned_bytes is positive.
A positive page_count must fit its page ceiling; invalid range is collection_limit.

Pure priority: options; nonempty required/optional-known keys and known typed
subject/positive source ReceiptId (invalid_key/invalid_subject); ALL per-vector/
combined counts, including nested replacement occurrences and page_count; required
scalar/envelope/optional-group and known-enum shape; cause/plan/failure/observation
matrix and both Exact component bounds; detection/resolution/version ordering and
local link/replacement shape; aggregate owned INPUT bytes. Negative after_position
is invalid_cursor; checked-head ref is an optional nonempty key. No SQL/ticket/
cleanup on any pure error, preserving earlier caller staging and preceding actual
foreign/stale/closed capability errors. This is proposal semantics; no validator
is implemented in the stub.

For isolated malformed inputs, use these specific category mappings after the
earlier options/key/subject/count checks:

- A null `DetectionData.audit.document` is `invalid_detection`.
- Missing required numeric `FailureEvidence` is `invalid_failure`.
- An illegal PlanEvidence kind/extent/stopped_for branch for a known cause is
  `invalid_plan_evidence`; a valid branch with an invalid numeric failure stage or
  failing ordinal is `invalid_failure`.
- An Observation-specific shape defect, including a null annotation document,
  is `invalid_observation` in BOTH `first_observation` and `request.observation`.
  A cause/plan matrix requirement for a known state token remains a plan-evidence
  requirement, rather than making every absent observation token invalid.

Required format/reference keys still use the earlier `invalid_key` tier. A
non-null empty document is legitimate DATA. These mappings do not impose a total
order among unrelated defects within one preflight stage; category examples
isolate the relevant defect and preserve all previously specified precedence.

After pure checks the exact701/83e selected actual-kind retry→closed/current state→
active expected/version/token sequence applies. Resolution compares COMPLETE actual
active set before per-entry existence/closed/expected/token checks. Corrupt full
selected identity fails storage before a healthy mutation_conflict. Incoming reference
absence/inconsistency uses missing_reference/source_mismatch AFTER actual rollback;
the same defect in selected stored closure is invalid_stored_data. Well-typed selected
overage is result_limit; wrong class/NULL/group/chain/noncanonical data is
invalid_stored_data, with both Exact classes→both sizes→grammar order preserved.

Public SqliteHoldInputError and SqliteHoldStorageReason have exactly the categories
in701, with simple tagged exception accessors only. Known source-store busy/sql/
resource/result errors retain their matching hold reason AFTER known rollback;
selected missing/mismatching/corrupt old references map by the incoming/stored
rules above. Already-aborted retention/writer exceptions are not cleaned a second
time. Original SqliteWorkFailure wrong_thread/stale_session/session_closed/protocol/
invalid_statement/unmapped/uncertain outcome is NEVER converted into a known hold
rollback. Shared actual SQL/binding/result budgets are never reset or transferred.

## Exact logical byte formulas

These are requirements for later implementation/tests, NOT new serializer code.
For owned string or byte array x, `S(x)` is byte length (no terminator/framing).
`E(x)` is numerator decimal ASCII length INCLUDING sign plus positive denominator
ASCII length in canonical reduced form (zero0/1). `O(x)=0` when absent, otherwise
its member formula; engagement adds NO invented boolean byte. An explicit enum/
bool is1, signed64/uint64/position/count is8. Source schema_version would be4 if
returned, but none of these DTOs echoes receipt facts. C++ padding/heap/vector
capacity/length prefixes are not counted. Distinct equal owning members each count.

`B(Op)=S(scope)+S(ref)`, `B(PlanKey)=S(plan_ref)+8`,
`B(Envelope)=S(format_ref)+S(document)`, `B(Subject)=1+8=9`.
The variant subject tag is its ONE logical existing SubjectKind; ID is typed8.
A runtime event variant adds1 for its actual MutationKind alternative; a concrete
fixed Detection/Resolution type has no extra phantom kind field of its own.

| Owned value | B(value) |
| --- | --- |
| Source |9+8+O(event_ref)+O(parent_event_ref)+O(root_event_ref)+S(configuration_ref)+S(arrival_context_ref) |
| Command |B(operation)+B(intent) |
| Observation |O(persisted_state_token)+B(annotation)+O(policy_revision_ref)+O(permission_revision_ref)+O(actor_ref)+O(decision_ref)+O(observed_utc_ms)+O(elapsed_ms) |
| ProposedOccurrence |8+O(action_ref)+O(action_version_ref)+O(rule_ref)+O(target_ref)+O(unit)+O(attempted_units)+O(finalized_units)+O(explanation) |
| PlanEvidence |1+O(plan)+1+O(stopped_for)+O(failed_capture)+SUM B(occurrence) |
| FailureEvidence |1+O(failing_ordinal)+O(failing_target_ref)+O(attempted_units)+B(annotation) |
| AssociatedApplication |B(operation)+8 |
| TimerEvidence |8+1+8+1+1+8+1(required explicit composition)+B(composition_decision)+O(associated) |
| Cause |S(hold_ref)+B(source)+B(command)+B(first_observation)+B(plan_evidence)+1+O(failure)+O(timer)+O(missing_policy_reason)+O(supersedes_hold_ref) |
| DetectionData |S(mutation_ref)+O(expected_version_ref)+S(proposed_version_ref)+B(observation)+B(audit) |
| Detection |B(cause)+B(request) |
| VersionLocator /ReplacementLocator |S(hold_ref)+S(version_ref) / S(hold_ref)+S(detection_ref) |
| Version |B(locator)+8+O(parent_version_ref)+1+S(producer_mutation_ref)+O(replacement) |
| HeldOrigin |B(version locator)+8+1+O(remaining_units) |
| ApplicationLink |1+B(operation)+8+B(plan)+O(held_origin) |
| ResolutionEntry |S(hold_ref)+S(expected_version_ref)+S(proposed_version_ref)+B(decision)+O(replacement Detection) |
| ResolutionBatch |S(mutation_ref)+9+SUM B(expected locator)+SUM B(entry)+S(actor_ref)+S(decision_ref)+S(reason)+B(options)+O(application link) |
| RecordedResolution |B(request)+SUM B(produced version) |
| CurrentEvent |1+B(active DetectionData OR RecordedResolution alternative) |
| HoldView |B(cause)+B(first_detection)+B(first_version)+B(current_version)+O(current_event) |
| ActiveSet |9+SUM B(hold view) |
| HistoryEvent |1+B(active Detection OR RecordedResolution alternative) |
| HistoryEntry |B(version)+B(event) |
| HistoryQuery |S(hold_ref)+8+O(checked_version_ref)+8(page_count) |
| HistoryPage |S(hold_ref)+S(checked_version_ref)+SUM B(entry)+O(next_position)+1(has_more) |
| HistoryResult |1(disposition)+O(page) |
| MutationOutcome |S(mutation_ref)+1(kind)+SUM B(produced version) |
| MutationResult |1(disposition)+O(outcome) |

Here O(string) means its byte length when present; an empty known DATA string
therefore adds0 bytes but still differs structurally from absence. Likewise a
present Envelope("f",non-null empty BLOB) adds1 while absent envelope adds0.
Exact-3/1 adds3, Exact0/1 adds2; no finalizer4096-bit policy is invoked by this
storage byte count. A Version(h1,v1,position1,parent absent,active,producerd1,no
replacement) adds2+2+8+1+2=15. If returned as BOTH first/current, those two owning
members add30: there is no equality/hash deduction. First/current matching values
and canonical whole optional groups still receive full equality/shape validation.

Input owned_bytes is applied to the ACTUAL submitted value for EVERY method:
find→S(hold_ref), active→9, history→B(query), detection→B(Detection),
resolution→B(Batch). Keys also have the independent reference_bytes bound. Fixed
query inputs have no vector items; do not echo/charge them in the result unless
an actual result member owns the same value. A factory takes no hold limits and
uses the separate MigrationManifest contract. Output owned_bytes applies to the
actual optional view/set/history/mutation result, not to internal reference decoding.
Input-byte overage is pure owned_byte_limit; post-SQL selected result overage uses
result_limit after actual rollback. No DTO merely satisfying these byte bounds is
promised to fit the actual cumulative writer binding/result budgets.

## Exact vector item formulas

`I(x)` counts ACTUAL elements of every vector owned within x, once per owning
occurrence. Strings/bytes/scalars/optionals/variants are NOT vector elements; an
engaged optional contributes its contained nested I, absent contributes0. A vector
contributes its length PLUS each element's nested I. Thus:

- I(PlanEvidence)=occurrences.size; I(Cause)=I(plan_evidence);
  I(Detection)=I(cause); DetectionData/Version/Locator/Observation have I0.
- I(ResolutionEntry)=I(optional replacement Detection).
  I(Batch)=expected_active.size+entries.size+SUM I(entry).
- I(RecordedResolution)=I(request)+produced_versions.size.
  I(CurrentEvent)=I(its actual alternative).
  I(View)=I(cause)+I(optional current_event).
- I(ActiveSet)=holds.size+SUM I(view).
  I(HistoryEvent)=I(its actual alternative);
  I(HistoryEntry)=I(event); I(Page)=entries.size+SUM I(entry);
  I(HistoryResult)=I(optional page).
- I(MutationOutcome)=produced_versions.size;
  I(MutationResult)=I(optional outcome). find's optional View has I(View) when
  present,0 when absent. All fixed query input I values are0.

Input_items applies to Detection or Batch complete I BEFORE element bodies;
result_items applies to actually returned optional View/ActiveSet/HistoryResult/
MutationResult I, including every nested replacement/event copy. Relevant per-
vector ceilings also apply independently. Actual selected declarations/counts/
metadata must be proved BEFORE reserve/materialization as83e requires; an understated
count is corruption, not permission to grow or a result_limit substitute. No
unreturned selected plan/receipt/snapshot/reference payload becomes a fictitious
returned item, and no metadata-history count becomes a lifetime materialization cap.

## Existing boundary and next gates

There is no existing public getter for arbitrary historical state token or full
selected plan body plus metadata-only history, and legacy receipt/contribution
begin/adoption transfers commit/destructor-cleanup ownership. Future hold reference
validation needs actual same-session internal strict SQL/decoder factoring, not
imagined getters, old full-history materialization, dummy PreparedBundle, reversal
basis misuse or a second reader. A link proves DATA closure only, never newly
staged THIS-unit data, shared COMMIT, current policy or replay permission.

The unique marker is the ONLY body behavior now. Minimal exception accessors and
inert storage of session/limits are plumbing, not feature validation. API/header/
stub/config need actual different review and OWN selected environment compilation/
old-suite green before independently authored fixtures. Those later tests need
full review/compiler/exact discovery and their FIRST genuine marker-red before
STORAGE-016 production. Whole overflow, timer mixed policy, FailedCapture producer,
postfinish/continuation/reject/defer/current authority remain explicit owner-service
or owning-task gates. All agreed providers/FX/UI/renderers/platform/release scope
and all previous assertions/fixtures/DDL remain unchanged.
