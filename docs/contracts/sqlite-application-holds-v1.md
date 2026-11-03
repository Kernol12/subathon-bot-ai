# Durable application-review holds — concrete document proposal revision 1

STORAGETEST-016 DOCUMENT phase only. Authority: specificationv0.45 §§3.3–3.5,4,5,
NUM-02/04 and DB-01; accepted [application-service logical contract](application-service-v1.md)
blob6d0cd67f; [ADR0003](../decisions/0003-application-service-prerequisites.md);
[writer/session](writer-transactions-v1.md), [bundle](sqlite-application-bundle-v1.md)
and [global original claims](sqlite-original-claims-v1.md). This proposal supplies
owned value meanings, port operations, engineering choices and literal examples.
It supplies NO C++ signature, physical layout/DDL, migration, stub, fixture or test.
Different actual document review precedes those separately assigned artifacts.

## Scope, ownership and honest guarantees

Persist independently enforceable application-review causes on the caller's ONE
active SqliteWorkSession. No BEGIN/commit, owning contribution adapter/adoption,
second writer, destructor cleanup, event pumping, provider call or publication.
Presence/reads/staged results are provisional, including a duplicate or a referenced
application. Earlier durable intake/context/capture is a caller protocol established
by actual earlier acknowledged units, not inferred from row presence in this unit.

One database is one installation/subathon. Subject gating is GLOBAL in that
namespace: contribution/CID including0, or explicit non-support/positive ReceiptId.
No operation_scope, request key, confirmation receipt or plan revision participates
in its active-set selector. Trusted installation binding/classification belongs
the owner service, as for017; an arbitrary caller string cannot create a second
hold namespace. Equal money/names/timestamps do not establish subject identity.

The adapter writes only its future hold records/history/heads/decision links. It
NEVER changes contribution evidence/review/valuation/application/reversal/settlement,
application state/head/effects/jobs/claims/results, or a stored plan. The service's
WHOLE-input overflow unit commits only the hold and its non-success audit; earlier
fitting proposals do not survive. The adapter cannot inspect arbitrary caller SQL
and certify that another API never staged an unrelated application in this unit.
That stronger WHOLE-attempt no-success rule is the owning service protocol and
requires its own real orchestration tests. Hold storage must not present its normal
staging return as proof of that protocol or as an acknowledged durable Held result.

Known selected-data/storage failures establish actual WHOLE caller-unit rollback,
including unrelated earlier staging, before a known-rollback exception. Actual
cleanup/COMMIT uncertainty remains original WorkFailure/unknown/fail-closed/no
normal payload/no blind retry. Only actual COMMIT plus normal work completion
permits an authorized durable response. A later publication failure is separate.

## Identities and independent conditions

Use opaque nonempty bounded byte references supplied by the trusted caller:

- HoldRef identifies ONE immutable cause. No allocator, monetary correlation or
  automatic deduplication of equal-looking causes. Multiple HoldRefs may be active
  for one subject; one current version/head exists PER HOLD, not per subject.
- DetectionRef and ResolutionRef are role names for ONE shared raw-byte mutation
  key domain, unique across BOTH kinds in this database. The kind is stored DATA,
  never an extra key namespace. Equal ref bytes with opposite kinds return
  mutation_conflict BEFORE hold freshness/closed/version checks. First validate
  the selected existing record under its ACTUAL stored kind; corruption is a
  storage failure, not a healthy conflict or reinterpretation as the incoming kind.
- A DetectionRef identifies ONE immutable detection request. Exact retry returns
  that historical detection; altered payload conflicts. It never appends history
  twice or reactivates a hold. A ResolutionRef similarly identifies ONE immutable
  resolution/replacement batch and its recorded historical version results.
- VersionRef is an opaque CAS token, unique for the lifetime of its HoldRef. Never
  rebind/reuse a version, including after resolution or an identical-data ABA.
  Caller issuance is not authority; collision refuses without retry/generation.
- Positive signed64 history positions1..N provide INTERNAL ordering/pagination only,
  not new domain IDs or optimistic versions. Position0 is the initial read cursor,
  never a stored event. Validate contiguous count/min/max/parent chain before
  checked N+1; a sparse1/MAX fixture is invalid, not healthy sequence exhaustion.

Resolving one hold does not clear another hold or an independent payment/rate/
pause/recovery condition. Recording that one resolution may be legitimate while
other holds remain; applying through remaining blockers is not. No generic
"accepted" contribution state replaces this distinction (§3.3).

## Owned proposed values and branch-specific evidence

Names below describe proposed values for later API review, not supplied C++ layouts.
Strings are exact owned bytes; envelopes are nonempty format_ref plus non-null
owned byte document (present-empty BLOB differs from absence). Use existing typed
ReceiptId, ContributionId, ApplicationOperationKey/PlanKey, RequestedUnit and Exact.

| Value | Complete logical members / meaning |
| --- | --- |
| HoldSubject | known contribution/receipt tag; exactly CID0..MAX for contribution or positive ReceiptId1..MAX for explicit non-support; no operation scope |
| HoldSource | HoldSubject; positive actual event ReceiptId; optional known event/parent/root refs; required retained configuration_ref and arrival_context_ref |
| HoldCommand | actual original operation key; versioned complete owned command-intent envelope; no generated effect/success result or authority boolean |
| HoldObservation | actual persisted application-head token required for CapturedRevision, optional/explicitly unknown for FailedCapture; versioned owned observation/failure annotation; optional ACTUAL policy/permission-revision/actor/decision refs, signed64 observed_utc_ms and nonnegative elapsed_ms only where known; annotated proposed clock values are not authoritative persisted state |
| ProposedOccurrence | positive ordinal; known action/version/rule/target refs and unit where known; optional bounded canonical Exact attempted_units; optional signed64 finalized_units ONLY when actually produced; optional known explanation. Absence is unknown, never numeric0 |
| FailureEvidence | known numeric failure stage final_conversion or aggregate_after_finalization; optional actual failing ordinal/target into the PlanEvidence list and bounded canonical Exact attempted final/aggregate value; retained lossless source/provenance annotation. Absent for a nonnumeric policy/deficit cause |
| PlanEvidence | tagged CapturedRevision or FailedCapture alternatives below; owns ordered known ProposedOccurrences plus explicit extent captured_complete, proposed_complete or known_prefix; no magic revision0/invalid ApplicationPlan/PreparedBundle |
| HoldCause | HoldRef; immutable HoldSource/Command/first Observation/PlanEvidence; kind WholeInputNumericOverflow, TimerAdjustmentDeficit or UnresolvedPolicy; kind-specific data and composition below; optional supersedes HoldRef |
| Detection | DetectionRef; complete immutable HoldCause; expected current VersionRef absent ONLY on creation; fresh proposed VersionRef; owned actual attempt observation/audit, retaining this detection separately from original cause |
| HoldVersion | HoldRef/VersionRef/history position/parent VersionRef; state active or resolved; producing DetectionRef or ResolutionRef; owned actual detection/resolution annotation and optional replacement/application link; immutable |
| HoldView | complete immutable cause; first detection; actual current version and its producing event; historical references/projections required below. No full lifetime history vector or authority certificate |
| ActiveSet | exact subject; ALL active HoldViews in unsigned raw-byte HoldRef order. No arbitrary first hold, hidden page or "clear" flag on a partial result |
| VersionLocator | HoldRef plus actual VersionRef; complete expected active-set vectors sort strictly by HoldRef and contain no duplicates |
| ResolutionEntry | HoldRef; expected current VersionRef; fresh proposed VersionRef; owned explicit decision annotation; optional replacement Detection with a NEW HoldRef/DetectionRef and supersedes link |
| ResolutionBatch | ResolutionRef; exact subject; complete expected active VersionLocator set; nonempty entries in strict unsigned raw-byte HoldRef order closing a subset; actual stable actor_ref, decision_ref, reason/options envelope; optional complete application/continuation link |
| HistoryPage | HoldRef; actual checked current VersionRef; bounded ordered FULL event/version values after an existing position or0; explicit continuation position/has_more; no claim of complete lifetime materialization |

CapturedRevision requires an ACTUAL positive stored PlanKey. Resolve its complete
selected immutable revision/source, all selected bodies and healthy scalar history;
every finalized requested occurrence copied as evidence agrees with that revision.
For aggregate failure, individually finalized signed64 values may all fit while
an Exact aggregate/final state does not. Preserve the COMPLETE captured request
list and the failing target/ordinal only where actually known. Future plan-head
advancement does not rewrite this historical selection or reject its healthy read.

FailedCapture is REQUIRED when numeric conversion prevents a valid finalized
request/PreparedBundle, and may record an explicitly unresolved capture policy that
also prevents finalization. Its stopped_for tag is numeric_conversion or
unresolved_capture_policy, NEVER a guessed numeric error. Existing RequestedEffect/
PlanCapture REQUIRES a representable signed64 request and describes already-resolved
final amounts; it cannot encode a failed conversion by inventing0 or truncating.
This alternative requires explicit pre-finalization mode, the matching stopped_for
tag, a versioned owned failed-capture/provenance envelope, the complete KNOWN
occurrence list and known/unknown field distinctions. An ACTUAL
stored PlanKey may optionally identify prior selected context; when present it
receives the SAME full selected closure/source check, not a permissive unchecked
optional reference. It does NOT pretend the failed proposal became that revision.
When absent, actual receipt/config/context/support closure and required alternative
evidence remain mandatory. An unknown pre-capture application head likewise stays
absent; a known referenced token resolves its complete immutable snapshot, never a
new fabricated current head. No fake plan/header/revision0 is inserted.

The following cause/plan/stage matrix is EXHAUSTIVE; other combinations are
invalid_plan_evidence or invalid_failure in the pure preflight order below:

| Cause | PlanEvidence / extent | Numeric FailureEvidence / required shape |
| --- | --- | --- |
| WholeInputNumericOverflow | CapturedRevision / captured_complete | REQUIRED aggregate_after_finalization; every captured occurrence has known unit/exact/finalized signed64 values, matching the selected complete revision |
| WholeInputNumericOverflow | FailedCapture / proposed_complete or known_prefix; stopped_for numeric_conversion | REQUIRED final_conversion; every occurrence whose conversion failed has finalized_units ABSENT. A known failing ordinal identifies such a retained occurrence; no representable0 is fabricated |
| TimerAdjustmentDeficit | CapturedRevision / captured_complete ONLY | Numeric FailureEvidence ABSENT; actual selected finalized negative occurrence, explicit RequireReview and its full unapplied amount required. Unfinalized input cannot manufacture a timer deficit |
| UnresolvedPolicy | CapturedRevision / captured_complete | Numeric FailureEvidence ABSENT; explicit missing-policy reason, actual captured context retained; no invented numeric failure |
| UnresolvedPolicy | FailedCapture / proposed_complete or known_prefix; stopped_for unresolved_capture_policy | Numeric FailureEvidence ABSENT; explicit missing capture-policy reason and required alternative source/provenance retained; no invented finalized plan/amount |

Extent captured_complete means ALL actual selected revision occurrences1..N;
fields required by that finalized revision are present. Extent proposed_complete
means the producer knew the WHOLE intended pre-finalization occurrence sequence
1..N, not that all amounts finalized; unknown individual fields stay absent.
Extent known_prefix means exactly the KNOWN ordered prefix1..K (possibly empty)
is retained and the remaining sequence/length is explicitly unknown. It cannot
be used for CapturedRevision or labeled complete, and cannot omit a known member.
These are recording declarations with actual known provenance, not proof of a
producer's evaluation/permissions. In every extent a known failing ordinal must
identify a retained member; absent ordinal is genuinely unknown, never invented0.
Optional prior PlanKey in FailedCapture does not change this matrix or make that
unfinished proposal equal to the prior revision's finalized requests.

This makes pre-capture failure RECORDING possible. It does not change service004's
accepted selected-complete-plan application protocol. The earlier producer/evaluator/
capture failure policy and semantic failed-capture codec remain separately reviewed
work; a nonempty document does not prove those computations happened or were legal.
Storage checks shape, references, exact representation and retained equality only.

## Cause kind and explicit composition

WholeInputNumericOverflow requires FailureEvidence and has NO associated application.
Its phase may be failed individual final conversion OR later overflow of individually
valid occurrences. The latter cannot discard earlier fitting time/score proposals.
A missing representable final amount remains absent; bounded known Exact/raw source
is retained. If the evidence exceeds storage/resource bounds, fail truthfully;
returning a transient enum does not satisfy durable-review requirements.

TimerAdjustmentDeficit is a DISTINCT timer occurrence, not NUM-02 overflow. Retain
its selected negative signed64 request (including MIN), explicit RequireReview
policy/disposition, full unapplied amount and exact request/component locator.
A held timer has actual Main/debt delta0 and the full negative request unapplied;
no original financial allocation is derived. Composition is EXPLICIT:

- DeferWholeInput requires an actually supplied externally reviewed mixed-policy
  decision annotation and no associated application. It is never inferred merely
  because the numeric timer disposition is RequiresReview.
- BesideCompleteApplication requires that explicit decision and an actual selected
  complete original bundle whose matching held timer occurrence has the retained
  zero/unapplied components. Store a typed OperationKey/occurrence link, validate
  full bundle closure/source and exact selected timer facts, and retain the hold
  beside it in the SAME caller unit. The old original claim/result remains honest;
  this does not enable replay of already-applied sibling components.

That selected application link proves record/subject/occurrence closure and owned
DATA only. The same complete record could have been staged in this unit or committed
in a prior unit; the adapter cannot certify which, or that hold/bundle COMMIT together.
Same-unit composition remains an owning-service protocol with real transaction/
completion tests. The link never grants replay of a previously processed application.

Without a reviewed applicable mixed policy, record UnresolvedPolicy as a WHOLE-input
non-success cause with explicit missing-policy meaning. Do NOT silently choose
DeferWholeInput/BesideCompleteApplication or invent fake cap/unapplied components
for post-finish lifecycle denial. Unresolved pause/recovery/action-policy meaning
may be retained without clearing those actual module conditions.

An explicit decision annotation is DATA, not CURRENT permission or policy proof.
Current access/resolution policy is enforced by its trusted owning service. Storage
may record a synthetic format with complete declared test meaning; that proves
storage/atomic linkage only. Production paths requiring unknown mixed timer,
post-finish or continuation policy remain not-ready until CORE-008/ACCT-005 (or
financial ACCT-006) provides its ACTUAL reviewed contract/examples/meaning.
No permission flag, opaque actor string or successful storage call enables them.

## Proposed operations and normal dispositions

All facade construction/copy/destruction is inert. Every read/mutation borrows the
same revocable session; no cache turns a value into a durable/authority capability.

| Operation | Exact selected scope / normal result |
| --- | --- |
| find(HoldRef) | absent only if that hold anchor is absent; present returns complete HoldView after its head/history/selected full payload/source closure |
| active(HoldSubject) | complete ActiveSet after the ALL-selected-subject metadata walk below; legitimate no anchors/active versions returns empty, never global auditing |
| history(HoldRef, after_position, optional checked VersionRef, page_count) | complete bounded page; absent hold explicit; changed checked head gives stale_history with no data; nonexistent nonzero cursor gives cursor_conflict; continuation is explicit |
| record_detection(Detection) | recorded, already_recorded, mutation_conflict, hold_conflict, stale_version, version_conflict or closed_hold; healthy refusals make NO own writes/cleanup |
| stage_resolution(ResolutionBatch) | recorded, already_recorded, mutation_conflict, stale_active_set, stale_version, version_conflict, missing_hold, closed_hold or replacement_conflict; healthy refusal makes NO own writes/cleanup |

For a detection, first resolve any existing mutation identity and its COMPLETE
historical selected closure. Exact request equality includes cause/observation,
expected/proposed versions and all optional members. Exact replay returns its
recorded historical outcome BEFORE today's version/closed-state checks, without
reappending or reactivating. Changed payload is mutation_conflict. Missing identity
then selects HoldRef and validates its complete current closure. A RESOLVED hold
returns closed_hold for this NEW mutation BEFORE inspecting stale expected version,
new cause equality or proposed-version reuse. It never implicitly reopens. For an
ACTIVE hold only, compare immutable cause (hold_conflict), expected current version
(stale_version), THEN fresh proposed-version namespace (version_conflict). A missing
hold requires absent expected version and an otherwise valid fresh creation. Thus
exact mutation retry → selected closed state → active-transition freshness/token
checks is explicit; a new observation can append only to an active hold.

A genuine reoverflow/new cause uses a NEW HoldRef, immutable DetectionRef/cause and
optional supersedes link to the same subject. An old resolved cause/history remains
unchanged. Replacing an active cause is ONLY a resolution batch with its complete
replacement Detection: close-old/create-new/head transitions happen together, or
none do. Each replacement uses fresh hold/detection/version identities; both link
directions resolve the SAME old/new causes and subject. The final subject stays
held. Replacement is not a success/apply outcome and carries no application link.
A new independent cause may also be added without closing any existing hold.

Resolution first checks exact ResolutionRef replay/changed-payload conflict, then
actual complete subject ActiveSet against expected VersionLocators, then every
entry's selected hold existence/closed state, expected current version and proposed
namespace in that order. An old expected set may therefore give stale_active_set
before an entry's closed_hold; a matching set does not make a closed entry active. It prepares ALL
allocating data/statements before writes, appends immutable events/versions, inserts
any replacements and advances per-hold heads LAST. No mutable cause/history or
contribution dimension is overwritten. Every post-write failure/exception is guarded
through actual whole-unit rollback/unknown, as for002; no partially closed batch.

A resolution WITHOUT application link may close a subset while others stay active;
its historical decision is not an application permission or a new award claim.
For a resolution WITH application link, require every currently active hold in this
application-hold namespace for that subject to be closed with no replacement in
this batch. Other payment/rate/pause/recovery/access checks remain owning-service
preconditions. Recheck current conditions and stage the actual complete new bundle
and resolution together under the SAME caller unit; never clear-then-apply commits.
The typed link resolves a complete actual bundle, never a fabricated success row.
This checks selected row/subject/occurrence closure ONLY: row presence, the link or
a local return cannot prove the bundle was newly staged THIS unit or that the two
writes commit together. The SAME-unit protocol and no clear/apply gap are service
obligations, not a storage-granted durable/transaction capability. A link to a prior
committed record cannot authorize replay or satisfy those missing service proofs.

Whole-input application links retain the same tagged input subject and actual
source/proposed-plan provenance. A later timer correction instead retains the
ORIGINAL held subject/occurrence/remaining amount PLUS a DISTINCT complete original
correction OperationKey/source. It cannot point back to the already-processed
original as if replaying that claim resolved its pending timer. Correction identity,
partial/full remainder policy, reject/defer choices and semantic decision codec
remain CORE-008/ACCT-005 gates; financial continuation is not invented here. The
storage representation retains supplied typed links/known amounts and validates
references; it does not authorize or calculate their remaining entitlement.

## Closure and complete active-set discovery

A hold anchor binds its immutable tagged subject/cause and one current head. Select
anchors by the exact GLOBAL typed subject BEFORE interpreting current status. Do
NOT INNER JOIN/filter to only existent active versions: that could make a dangling
head or malformed status disappear and falsely report an empty active set.

Walk ALL anchors whose stored subject matches the selected subject, including
inactive/resolved ones. For EACH validate anchor/key/subject types and exact match,
head existence/identity, actual typed current state and complete metadata history:
positions1..N/count/min/max, actual unique versions, parent links, matching head/
terminal event and detection/resolution ownership. No orphan/head/state is repaired.
Only after those checks classify active versus resolved. No separate cached status
is authoritative; any future redundant field must agree with that actual version.

Metadata uses bounded64-row keyset pages plus checked65 lookahead. The FIRST page
has no invented positive/nonempty lower cursor that hides malformed zero/empty/
negative keys. Actual selected COUNT/type/domain/order summaries and visited counts
cover ALL matching rows, including malformed out-of-range values; an inner join or
predicate coercion is not proof of complete membership. This does not infer that
an unrelated malformed subject/key elsewhere belongs to this subject or perform
a global orphan audit. An empty selection certifies only this declared namespace.

Fully load each returned active hold's cause, first detection, current version and
producing event, selected receipt/config/context/support/plan and application links.
Validate all declared versus actual request collections and exact optional groups,
source subject/IDs, canonical Exact components, immutable equality/crosslinks and
version transition rules. Historical current contribution dimensions/state head/
policy need NOT equal their old recorded observations. No current reevaluation.

Inactive nonreturned payload bodies are NOT loaded or certified merely to discover
active holds: their anchor/head/status/COMPLETE history metadata is still checked.
Likewise unselected past version DOCUMENT bodies are not certified by a current
HoldView; history() checks the FULL actually returned page bodies. find() resolves
its actual selected full current/first bodies even for an inactive hold. Metadata
history is not a materialization/lifetime quota. A selected malformed inactive
head/status is an error, not absence; a malformed unselected old document remains
outside that narrower projection and cannot be advertised as validated.

Every known observation state token resolves its complete stored snapshot/config/
balance closure; absent FailedCapture observation does not read/invent a head.
Source closure requires actual typed receipt/retained cfg/context; tracked support
also resolves its actual CID/original support anchor, retaining unknown fields as
NULL and not confusing event receipt with original support. Present PlanKey receives
full selected immutable plan/source plus contiguous healthy revision metadata;
FailedCapture without one receives its mandatory alternative closure. Associated
application links use full002/017 record closure, not the narrower reversal-basis
projection. No actor/action/rule/format string becomes an unrelated fake FK or proof.
The later physical/API proposal must bind all matching crosspaths explicitly.

## Bounds, preflight and failure order

Proposed positive trusted defaults (engineering limits, not product/lifetime quotas):
reference_bytes4096, value_bytes262144, exact_component_bytes16384,
known_occurrences_per_cause1024, active_holds_per_result256,
resolution_entries64, history_versions_per_page64, input_items4096,
result_items8192, owned_bytes1048576. Empty legitimate collections remain valid.
Each count ceiling reserves n+1 strictly below qsizetype/qint64 and EVERY relevant
concrete vector.max_size; combined totals use checked size_t addition without an
invented aggregate-vector capacity. Metadata64/65 is separate from all returned
collection ceilings. Oversized active set fails result_limit; no truncated clear.

Pure priority before ANY ticket/SQL/cleanup: ALL options; mutation/query/ref keys
and known subject/positive Receipt/CID-role; ALL per-vector/combined input counts;
required envelopes/scalars/known enums/whole optional groups; branch-specific
cause/plan/failure/observation shape and Exact bounds; version/entry ordering and
local replacement/link shape; aggregate owned input bytes. Optional absence counts0;
owned strings/documents/canonical components count their bytes, uint64/signed64/
positions8, enum/bool1 (uint32 source schema version4 if actually returned).
Returned aggregate totals count ACTUALLY owned members once, including repeated
owning members and each vector element; internal validation projections do not add
fictional returned items. Paged metadata remains charged to ACTUAL writer budgets.

Proposed input categories: invalid_options, invalid_key, invalid_subject,
collection_limit, invalid_cause, invalid_failure, invalid_plan_evidence,
invalid_observation, invalid_detection, invalid_resolution, invalid_cursor,
exact_component_limit and owned_byte_limit. Shape errors win before ticket access
and preserve healthy earlier caller staging, including on expired/foreign tickets.

After pure checks: actual capability truth; selected mutation identity/full closure
and duplicate comparison BEFORE current freshness; selected hold/cause identity;
complete active metadata/expected set where required; per-entry expected versions;
fresh version/replacement identities; typed retained source/reference closure;
all preparation; atomic staging/head transitions. Reads only validate their declared
selected closure. Healthy conflict/stale/closed dispositions perform no own changes.

Known storage reasons: busy, sql_failure, resource_limit, result_limit,
missing_reference, source_mismatch, invalid_stored_data and sequence_exhausted.
Actual SQL-class/NULL/key/group/ordinal/chain/noncanonical data fails invalid_stored_data;
well-typed selected field/count/byte overage fails result_limit, never truncates.
For Exact check BOTH component classes/nonempty shapes, BOTH sizes, THEN canonical
ASCII/gcd/zero grammar before arithmetic. Existing exact/ID codecs remain exact;
no unsigned-to-signed, SQLite REAL, saturation or unsafe signed-MIN negation.

Local known failures require ACTUAL whole-unit rollback before that reason escapes.
Already-aborted writer/retention failures are mapped without a second terminal
cleanup. Wrong-thread/stale/closed/protocol/invalid-statement/unmapped WorkFailure
and actual unknown remain original; uncertain rollback overrides a local reason.
All statement/binding/result-row/result-byte budgets are the SAME cumulative writer
budget across other stores in this unit. A fitting hold DTO is not promised to fit
default64 statements/1MiB values; no reset, second connection or fixed-call promise.

## Independent literal document examples

These are hand-derived future oracles, NOT tests, engine output or observed red.

| ID | Supplied arrangement | Required observation |
| --- | --- | --- |
| H01 global gate | h1 active on CID0/source7, command(alpha,A), planP1; new command(beta,B), confirmation8 or P2 | active(CID0) returns h1; no scope/key/replan bypass. Receipt-tagged7 is distinct from CID7/CID0 |
| H02 exact detection | Detectiond1 creates h1/versionv1; exact d1 replay after headv2 or resolution | same historical d1/v1, no new event/reactivation; d1 with changed cause/actor/expected/newversion conflicts |
| H03 independent causes | hOverflow/v1 and hPause/w1 both active CID0 | complete set has BOTH; resolving only overflow leaves pause active and does not clear missing evidence/rate/recovery |
| H04 stale review | read {h1:v1,h2:w1}; another acknowledged detection advances h2:w2 | batch expected old set→stale_active_set, no own writes; matching set but wrong entry version→stale_version |
| H05 no ABA/priority | v1 existed; NEW detection proposes v1 with current expectedv2 on ACTIVE h1, or proposes it after CLOSEDv3; exact original mutation retry is separate | ACTIVE/current expectedv2→version_conflict; CLOSED/new mutation→closed_hold BEFORE proposed-token reuse/stale version; exact retry→its historical result first. No overwritten cause/reactivation |
| H06 replacement/reoverflow | active h1:v2 replaced by fresh h3:u1 with new immutable cause and supersedes h1 | one unit closes h1/creates h3, subject remains held; real failure rolls back BOTH. Later reoverflow after h1 resolved uses a NEW linked HoldRef, not mutation of h1 |
| H07 whole mixed overflow | Main55/debt0; time+5 fits; scoreMAX +1 overflows | no time/score/job/claim/after-state/success result from that attempt; hold retains whole known list, failing score target/ordinal and exact9223372036854775808; prior intake remains |
| H08 pre-conversion overflow | known proposed occurrence exact9223372036854775808 cannot finalize signed64; no valid RequestedEffect/PreparedBundle or stored plan yet | FailedCapture/source/context branch records exact attempt, finalized_unitsABSENT and failure stage; no fake0/revision0. Later producer policy still requires review |
| H09 MIN exact attempt | Main0/debt0, authorized requestMIN with CarryTimeDebt would require debt9223372036854775808 | whole-input overflow with exact known attempted value; no unsafe negation/saturation. Main1/debt0 same request fits debtMAX and must not be mislabeled numerical overflow |
| H10 timer distinction | Main100/debt7/request-150, RequireReview→actual0/full-150 unapplied | timer occurrence retained. Explicit reviewed DeferWholeInput versus BesideCompleteApplication decides composition; absent policy→UnresolvedPolicy/no invented default or numerical-overflow label |
| H11 current replay boundary | original bundle retains held timer and other actual effects; later authorized correction uses NEW source/key linked to hTimer/version/remaining | one actual correction+resolution unit, original claim/result/effects unchanged; pointing at old original as a second application does not resolve the pending timer |
| H12 selected corruption | subject anchor exists but head/terminal version is missing, bad status, or extra nonpositive history row exists | active lookup fails known invalid_stored_data with real rollback before inside-catch independent BEGIN; never empty/first-page-only/repair |
| H13 scope of validation | resolved hold has healthy full metadata but malformed unselected old document | active discovery validates metadata without certifying that document; asking history page containing it fails. Unrelated other-subject orphan is not globally audited |
| H14 finite result versus history | 65 healthy history events, metadata pages64+lookahead; selected active views fit own limits | audit all65 without lifetime quota;65th malformed metadata fails. Actual active result257 under ceiling256→result_limit, never partial/clear |
| H15 resource overage | incoming Exact/document/count/owned bytes exceed pure limits, OR selected stored data/actual writer budget exceeds post-SQL limits | PURE input overage→typed input failure, NO SQL/cleanup and earlier caller staging preserved. Selected/writer post-SQL known result/resource failure→actual whole-unit rollback; real uncertainty stays original unknown. Neither is a durable-Held result |
| H16 provisional/unknown | d1/h1 staged then read; branches caller rollback, normal commit, commit acknowledgement loss | read is provisional; rollback removes ALL same-unit hold/history, normal committed completion exposes once, loss staysunknown/fail-closed/no payload/no blind retry despite a physically present hold |

## Next gates and retained release scope

Different actual DOCUMENT review is next, including individual-conversion/no-plan
alternative, identity/equality precedence, active metadata scope and resolution
policy boundary. Then explicitly assign the physical step6 proposal (preserving
EXACT prefix1–5/66), minimal API/named stubs and their separate review, independent
frozen real-SQL foundation/required green, actual test author/review/compiler/exact
discovery/own selected official environment and FIRST intended missing-behavior red.
Only that real baseline authorizes STORAGE-016 production, with separate code
review/acceptance and promoted ordinary CI. No old228/017/002 pass or old red is a
substitute. Root owns schema/migration order/build/presets/workflow/progress/pins.

Engineering identity/version/bound choices above are reviewable proposals, not new
owner policy. No owner decision is required merely to record known failures without
effects. Unknown mixed timer/post-finish/partial continuation/reject-defer semantics
remain explicit CORE-008/ACCT-005/006 gates before their product paths are enabled.
The planned port is not authorization/evaluation/delivery/recovery/UI. All seven
integrations/threeFX, both renderers/locales/themes/platforms/packaging/security/
backup/live and the complete agreed release scope remain mandatory.
