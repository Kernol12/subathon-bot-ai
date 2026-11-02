# SQLite application bundle — concrete proposal revision 1

STORAGETEST-002 contract/API/physical stage ONLY. Authority: specification v0.45
§§3.3–3.5,4,8,16/DB-01/NUM-01–04 and accepted
[application bundle revision1](application-transaction-bundle-v1.md), blob3c8cbd86,
including its ALL-returns-provisional protocol and AT01–AT21. Companion:
[exact proposed physical step5](sqlite-application-bundle-schema-v1.md).
No header/stub/production source/test/fixture/manifest implementation is supplied.
Different actual contract review precedes the next authorized stage.

## Scope and honest preconditions

Persist ONE supplied application bundle atomically on the caller's active
SqliteWorkSession. The port never opens/begins/commits, owns destructor cleanup,
adopts an owning receipt/contribution transaction, evaluates rules, finalizes
numeric effects, grants permission, dispatches jobs or reconstructs a restart.
The authoritative Qt owner performs the only real COMMIT and normal completion.
ALL lookups/reads/duplicates/claims/stage results are transaction-visible internal
data, possibly own staged rows. Presence/closure proves no prior-commit fact.
Earlier durable receipt/config/context/contribution/plan capture is a CALLER
precondition established by actual earlier acknowledged units, not inferred by SQL.

Current authority/eligibility/clock segmentation/hard-end/cap/milestone/numeric
correctness is independently established by the application before preparation.
This storage adapter preserves the supplied derivation and checks the shape,
identity, algebra and actual references explicitly listed below. It does not
claim that a storage-only state/job document or a nonempty semantic ref establishes
those preconditions. ACCT-004/006 and relevant API/job/clock contracts remain gates.

B08: any known failed stage rolls back the WHOLE caller unit, including earlier
staged work. Earlier DURABLE intake stays pending/not-applied; no prepared effect,
new snapshot/head/status/claim/job/result/audit survives or is published. Actual
unknown COMMIT/cleanup remains unknown/fail-closed/no normal payload/no replay.
Confirmed COMMIT then body exception before normal completion remains committed
without normal work payload; later publication failure cannot erase a completed
work result or durable operation record. No original red is repeated here.

## Owned proposed values (not supplied C++ code)

Use existing core ReceiptId, ContributionId, RequestedUnit, Exact, ContributionState,
TimerState and TimerTransition. Strings are owned exact bytes, not QString text.
Documents are non-null owned QByteArray; present empty BLOB differs from absence.
The names here fix data meaning for a later minimal header review, not layouts.

| Value | Complete proposed members / meaning |
| --- | --- |
| OperationKey | nonempty opaque operation_scope, operation_ref; stable trusted scope, no client/session/provider identity inference |
| PlanKey | nonempty plan_ref, positive uint64 revision; actual selected immutable plan |
| SubjectKind | contribution0 or receipt1; EXPLICIT trusted classification. contribution requires present CID (including0); receipt requires absent CID AND caller-established non-support classification. Missing CID alone never establishes non-support |
| Envelope | nonempty format_ref, non-null owned document; versioned bounded OWNED content, no fake semantic FK |
| StateSnapshot | nonempty opaque token; Envelope; actual retained configuration_ref; complete TimerState fields; nonnegative signed64 time_debt_ms; ordered Balance vector |
| Balance | kind2=score micro-points or3=whole count; nonempty target_ref; signed64 units. Sort strictly by kind then unsigned raw-byte target_ref; duplicate keys invalid |
| Intent | OperationKey; kind original0/reversal1; SubjectKind; ReceiptId; optional CID; PlanKey; expected_state_token; optional complete expected CURRENT ContributionState exactly when CID present; choices Envelope; reversal_ref required/nonempty only for reversal |
| Effect | existing selected request ordinal1..N; arithmetic_role direct0/time_addition1/time_reversal2; RequestedUnit; Exact exact_requested_units; signed64 requested_units; Exact rounding_residual; optional nonnegative signed64 maximum_main_ms for time_addition; explicit RefundDeficitPolicy and TimeReversalDisposition only for time_reversal; application_explanation; ordered Component vector |
| Component | kind0 Main delta,1 debt delta,2 score delta,3 whole-count delta,4 unapplied/held time,5 cap-limited time; signed64 units; target_ref present only for2/3 |
| JobIntent | nonempty scoped job_ref; optional selected effect ordinal; optional nonempty group_ref plus positive signed64 group_ordinal as a whole group; Envelope. Ordered vector preserves job occurrence order |
| Audit | optional actor_ref, optional decision_ref, optional reason; Envelope. Unknown remains absent; present empty optional value is preserved, never authority |
| ReversalLink | current effect ordinal; original OperationKey+request ordinal+credited component kind0/2/3; expected prior basis position (nonnegative signed64); expected prior signed cumulative; new signed cumulative; signed64 requested_increment; Exact cumulative fraction, exact entitlement, residual; full_refund bool |
| PreparedBundle | Intent; supplied advanced-before StateSnapshot; after StateSnapshot with fresh token; advance_explanation; ordered effects/jobs/transitions/reversal links; result Envelope; one Audit |
| RecordedBundle | owned decoded Intent, immutable head/advanced-before/after snapshots, historical contribution snapshot/application_after when present, all ordered effect/component/job/transition/reversal data, result and audit. Does not certify current authorization or current state |
| ApplicationInputs | actual current StateSnapshot; actual selected complete CapturedPlanRevision/head; actual current ContributionState/support provenance when present. Owned values, not an authority capability |

There is no new numeric StateRevision, EffectId, JobId or application allocator.
Effect identity is `(OperationKey, selected request ordinal)`. Component identity
adds its fixed kind; two equal action refs at different request positions NEVER
collapse. Original subject identity is tagged CID or ReceiptId; revision is not
part of the original award claim. Job identity is `(OperationKey, job_ref)`.

### State-token binding and declared coverage

State snapshots are immutable append-only rows keyed by opaque byte token. Token
binds ALL typed fields, format/document, configuration ref and balance values.
The only mutable head points to one snapshot. Every stage compares Intent.expected_state_token with the persisted current head.
Its supplied advanced-before snapshot either equals that exact head snapshot/token
or uses another previously UNUSED token; the after snapshot always uses a previously
UNUSED different token, even for an actual-zero application.
An existing token cannot be rebound, reused with identical data, selected as a new
head or used for ABA. There is no token arithmetic/order or implicit zero token.
Failed uncommitted inserts may be retried only under ordinary known-outcome policy.

Coverage is EXACTLY this snapshot's typed fields/balances and owned envelope/config
reference. It is NOT a universal revision for old contribution evidence/review
writers, plan-head appends, current permission or external clocks. Existing14
writers do not advance it. Therefore stage separately compares the complete actual
CURRENT ContributionState with Intent.expected state, checks actual selected plan
head/history, and relies on the explicit upstream current-policy preconditions.
The configuration reference resolves actual retained bytes but does not certify
that a cap/deadline was interpreted correctly. Any future writer of these declared
snapshot fields must use the same reviewed CAS/token protocol; no silent bypass.

Three snapshots keep freshness and arithmetic distinct: (1) the persisted head
selected by CAS, (2) supplied advanced-before at the command observation, (3) supplied
after application. Operation stores all three tokens plus advance_explanation;
RecordedBundle retains all three immutable values. If advanced-before token equals
head token, ALL data must exactly match. Otherwise it is a fresh immutable row and
requires nonempty bounded advance_explanation. An old token cannot be reused as
an intermediate snapshot either. No extra state-head commit occurs between them.

The head→advanced-before step may change timer lifecycle/phase/Main/Grace/wait and
its owned observation document. Clock segmentation, actual elapsed/UTC/config/hard-
end/recovery interpretation and legal transition chronology are UPSTREAM
preconditions, not computed here. It must preserve debt, all score/count keys AND
values, configuration_ref and format_ref: a clock-only advance cannot hide an
accounting/configuration mutation. No process-local tick becomes a checkpoint.
TimerTransition values are supplied/retained in order, not invented timestamps.

From advanced-before→after, storage DOES check exact aggregate deltas: after Main
and debt equal before values plus all matching actual component0/1 deltas; every
score/count balance equals its before value plus matching component2/3 deltas.
Untouched fields of these balances remain exact. Calculate these identities in Exact/
checked arithmetic, then require representable final values/nonnegative Main/debt.
This is conservation, not deciding capacity, debt-first order, rounding or legality
of lifecycle/Grace/wait changes. Those supplied policies and trace remain upstream.

Balance keys/configuration_ref/format_ref must be identical across all three
snapshots; adding/removing configured targets or upgrading a state format needs
its own reviewed configuration/state protocol. Component2/3 target must equal the
selected request's target_ref and an actual advanced-before Balance of matching
kind. Main/debt kinds use the typed singleton timer, not an invented physical
meaning for a string such as "main". Numeric policy still establishes the semantic
target denotes that timer. Current contribution conditions are compared separately.

### Effects and supplied derivation versus validation

All selected plan requests are represented exactly once, in ordinal order1..N;
N may be0. Effect unit/exact request/resolved request must equal that actual immutable
request. The selected plan retains its complete original/rate/currency/snapshot/
source/time/manual/rule/modifier/action/explanation/reversal provenance. Historical
strings remain unchanged; actual application explanation is retained separately.

| Arithmetic role | Exact component set and local checks |
| --- | --- |
| direct0 | unit score1→only kind2; unit whole_count2→only kind3. Its SOLE component.units MUST EXACTLY equal requested_units; signed actual units retained. No unrepresented score/count limiting/hold and no evaluator/eligibility inference |
| time_addition1 | unit0, request>=0; kinds0/1/5 EXACTLY: Main>=0, debt<=0, cap_limited>=0. In exact arithmetic request=Main-debt+cap_limited |
| time_reversal2 | unit0, request<=0; kinds0/1/4 EXACTLY: Main<=0, debt>=0, unapplied<=0. In exact arithmetic request=Main-debt+unapplied; explicit known policy/disposition retained |

Component vectors use ascending kind, no duplicate/extra/missing kind. Zero values
remain actual records. `rounding_residual = exact_requested_units - requested_units`
is checked exactly; this is conservation, NOT recomputing half-even rounding.
No stage recomputes min/headroom/debt-first order, upstream rounding, lifecycle,
refund policy or CHOOSE score/count amounts/state changes. Exact copied amount and
aggregate-delta equality are the storage conservation checks above. Numeric values and aggregate state
proposals already passed their separate exact/core/application checks. Overflow
must be held BEFORE a valid PreparedBundle; never saturate/partially apply. All
stored fields still require exact signed64 types/range. INT64_MIN is valid request
data; every check involving negation/difference uses exact/checked arithmetic.

Debt repayment/Main addition/cap limiting and Main/debt/held reversal remain
separate. Only main_added can rescue eligible Active Grace/wait; this adapter
supplies no such decision. RequireReview is a timer deficit hold, not automatic
whole-product status. Existing above-cap narrow-credit rejection and newer
plan_time_addition's retained above-cap Main remain distinct preconditions.

### Reversal identity, actual basis and progress

A reversal Intent's `(operation_scope,reversal_ref)` and actual source ReceiptId
bind one whole refund/correction input. UNIQUE(kind,receipt_id) additionally prevents
another key repeating that input. These are internal identities, NOT provider/cross-
service correlation proof. Distinct partial receipts/identities may reference the
SAME original effect/component. A current captured request occurrence may aggregate MANY original component
bases; link identity is current occurrence PLUS original effect/component, without
a new LinkId or invented/split plan request. Every reversal effect has at least one
link; original operations have none. Its request equals the exact SUM of its links'
requested increments. Different current occurrences may consume distinct consecutive
portions of the same basis under the same refund; the per-basis positions/cumulative
chain must advance in the declared order. Duplicate same-current-occurrence/basis
links are invalid; two equal action refs at different current ordinals remain distinct.

Resolve a stored ORIGINAL-kind operation through the selected-source closure
specified below. This projection does NOT certify the complete original application
or all of its payloads merely because its selected rows exist. Resolve its immutable
selected credited component0/2/3, with nonnegative actual amount;
cap-limited5, unapplied4 and debt delta1 are NEVER assumed refundable credit bases. Require matching unit and actual requested occurrence/target.
The basis is its actual nonnegative credited component amount, not requested or a
current rule. Signed original corrections remain valid stored effects; inventing a
financial refund of a negative correction is not this positive-credit entitlement
contract. Main basis0 must come from an original time-addition effect with debt
repayment0; refund allocation for an award that repaid debt is explicitly NOT enabled.
Main-versus-prior-debt repayment financial allocation remains ACCT-006's unresolved
gate: this storage representation preserves separate supplied bases, and does not
authorize/derive that combined product path. No unknown allocation is enabled.

For each original component, append positions1..N preserve reversal HISTORY ORDER
only (like receipt-attempt ordinals, not a new domain ID). Position0 is an optimistic
"no prior occurrence" token, never a stored row. All actual positions are positive
signed64; compare actual head before checked+1. Exhaustion is explicit, not wrap/gap
search. A tiny sparse position1/MAX fixture is NOT a healthy complete history or valid
exhaustion proof: typed raw MAX fields and physically infeasible contiguous MAX
history remain distinct. Malformed count/gaps fail BEFORE sequence_exhausted.
Position/cumulative progress is updated in the same unit as the new link,
effects/state/jobs/result/audit. Zero increments still append distinct provenance. Links sort by current request
ordinal then original scope/op/ordinal/kind raw-byte key. For a repeated basis in
one bundle, the first expected position/cumulative compares with actual progress;
later links compare with the immediately preceding proposed link, then all append
and advance one final head atomically. This is not a separate commit per link.

Audit the existing per-basis metadata in pages64: exact typed positions/count/min/
max/head, every previous→cumulative chain and actual terminal occurrence/head links.
Absent head requires no occurrences; no reset/repair. No full materialization cap
becomes a lifetime refund quota. Compare expected prior position AND cumulative;
stale returns no-own-write stale_reversal. Both reversal-head FK paths must resolve
the SAME original component/position/operation occurrence/cumulative value, not two
independent valid rows. For the matching current head:

- Fraction is canonical Exact in[0,1] and cannot decrease from the last selected
  occurrence's fraction. Full flag requires fraction1 and cumulative=actual basis.
- Require0<=previous<=cumulative<=actual basis, including basis0→both0. Thus stale/over-original
  amounts cannot be accepted just because the caller supplied them.
- Exact entitlement equals actual basis×fraction; residual equals entitlement minus
  new cumulative; each link.requested_increment equals `-(new cumulative - previous)` in exact
  arithmetic and must fit its supplied signed64 value. Per-effect SUM of all these
  increments equals that actual immutable captured request's requested_units. No independent
  per-refund rounding or reevaluation occurs. Whether the supplied cumulative is
  the correctly rounded entitlement is the ACCT-006 precondition, not a storage claim.
- Partial can reach the rounded full amount early (1 micro-point at2/3); full refund
  still records exact remaining increment0. Whole-count full-refund policy and
  verified financial correlation are upstream prerequisites, not invented here.

The adapter patches ONLY current contribution.application for successful ORIGINAL
stage (not_applied→applied). It never overwrites current evidence/valuation/review/
reversal/settlement from an old plan or expected snapshot. Reversal stage records
its full supplied history/actual components and preserves current application and
all other dimensions. Financial reversal classification/review/held multi-effect
status updates need their explicit future application contract/non-owning extension;
this is not a complete refund workflow or a scope deletion.

## Proposed operations and exact selected scopes

The new facade is constructed from active SqliteWorkSession+BundleLimits; construction,
copy/destruction do no SQL/cleanup. All methods const, no local single-use guard.
Names/dispositions are concrete proposals for the next minimal API review.
A pure `application_bundle_repository_manifest()` description will copy actual
prefix1–4 and append exact approved step5/full66/starts0–5; it grants no SQL/update
authority. All meaningful NEW API functions begin as immediate named stubs only
AFTER actual contract approval; constructor is inert. No such artifact exists here.

| Operation | Meaning / normal results |
| --- | --- |
| retain_initial_state(snapshot) | Only empty state namespace may create immutable snapshot/balances+head. Existing current snapshot exact-equal→already_present; otherwise conflict, never reset/repoint. Selected corruption fails. All returns provisional |
| read_state() | Optional current snapshot via actual head; zero head+zero snapshots→absent, missing/inconsistent metadata→failure. Return all its declared balances under bounds |
| read_inputs(PlanKey,SubjectKind) | missing_plan/stale_plan/ready; actual full structural plan audit before stale classification; ready owns selected head body, current snapshot and current contribution projection. Explicit source classification remains a caller precondition |
| lookup(OperationKey) | Optional complete RecordedBundle, own staged or earlier rows; strict full selected closure below. Absence performs no global orphan audit |
| read_reversal_basis(OriginalComponent) — ADDED projection, pending API/extension review | ready/reversal_conflict with optional OWNED selected basis/current progress; uses the exact selected-source closure and per-basis metadata audit below, not full RecordedBundle/global payload loading. Missing actual basis is a typed failure, never invented credit0 |
| stage(PreparedBundle) | staged/already_present/operation_conflict/subject_claimed/source_claimed/reversal_identity_conflict/stale_state/stale_contribution/stale_plan/state_token_conflict/stale_reversal/reversal_conflict/subject_unavailable. Every non-staged outcome makes NO own writes/cleanup; existing-result references/data remain provisional |

Original-operation claim is tagged CID0..MAX or ReceiptId1..MAX according to explicit
SubjectKind. Same-key exact Intent compares its complete structured values including
expected token/CURRENT-state snapshot/choices bytes, but EXCLUDES newly computed
advanced-before/after snapshots, advance_explanation, effects/jobs/result/audit. A duplicate can return the original valid
outcome even when a newly supplied VALID computed outcome differs. Pure malformed
arguments still fail before SQL. Healthy original-subject/source claims cannot
be bypassed by another key, confirmation, plan revision or fresh state-b.

No owning14 wrapper is instantiated. New current-contribution resolution selects
the actual full current row, its bounded original receipt/support anchor and actual
typed references, without growing attempts/observation-history reads. Immutable
support conditions need not equal current state. Event receipt may be a linked
confirmation distinct from original support; no originalReceipt==eventReceipt rule.
Selected plan event/ReceiptId/optionalCID must match Intent; physical identity does
not prove provider classification or prior commitment. Old14 may contain an applied
state without this new bundle claim; that is subject_unavailable (no award/repair),
not a fabricated complete previous bundle or automatic stored-corruption label.

Plan resolution adds the proof missing from public15.find_revision: canonical
key types/domain/cardinality/min/max/count/head plus all64-page structural scalar/
parent/authorized-audit/capture-valuation/original-money/dependency-flag metadata;
then fully decode selected rules/modifiers/requests and actual source/typed retained
cfg/context/known cfg. Current known cfg may differ from arrival cfg. Stage requires
selected revision==healthy actual head. Historical lookup permits an older selected
revision in a healthy later history. Never use a sparse targeted MAX DTO as complete
history, inspect mutable receipt attempts or apply full-find history caps to staging.

Full lookup validates fixed operation/result/audit/state/contribution-snapshot/claim
closure; exact declared counts versus actual rows; full effect/component/job/
transition/link types, fields, identity and ordinal/group coverage. Current state
head/CURRENT contribution values are NOT compared with historical after-state/
expected contribution snapshot. Current CID lookup here validates identity/actual
original-receipt identity target, not unselected current state/anchor. All immutable
plan/receipt/config provenance still resolves. It cannot invent a result for an
incomplete bundle, normalize corrupt bytes or replay to repair missing rows.

A reversal SOURCE lookup validates the following SELECTED-SOURCE closure. It is
not full lookup or a whole-application certificate. "Declared-count metadata" means
comparison with ACTUAL sibling rows and their scalar identity/order metadata,
not trusting result counters alone:

| Source projection | Exact checks / intentionally unselected data |
| --- | --- |
| Fixed rows | Decode the original operation header/Intent and require kind0; exactly one result and audit, exactly one matching tagged original claim, and one matching historical contribution snapshot iff CID present. Validate their declared fields/bytes under existing bounds. Resolve immutable head/before/after scalar/envelope/config rows and actual source/CID identity; never compare them with today's head/current contribution |
| State balance metadata | For EACH referenced snapshot, actual balance COUNT equals its declared balance_count. Inspect token/kind/target keys, known kind2/3, exact snapshot ownership, required typed/nonempty target and unique canonical key order. Head/before/after keysets must agree. Unselected balance AMOUNTS and whole-operation numeric deltas are not certified by this metadata projection; fully decode the selected component's required before-target row |
| Effects | Actual COUNT equals result.effect_count and actual selected plan-request count. Inspect operation/plan key equality, typed request ordinals exactly1..N and known arithmetic_role/unit with the corresponding role/unit shape. These are scalar identity/order checks. Unselected Exact/request/residual/amount algebra and application_explanation are NOT decoded/certified |
| Components | Actual total COUNT equals result.component_count. Inspect operation/request/kind keys; every parent effect exists, no missing/extra/duplicate kind and exact role-specific kind set. Inspect before_token equality and target presence/type/identity, including actual referenced before Balance for counter kinds. Unselected component AMOUNTS are not decoded or summed. The selected original EFFECT's complete component set IS fully decoded and checked for its local signs/partition/direct equality/actual basis, including debt-repayment0 for refundable Main |
| Jobs | Actual COUNT equals result.job_count; inspect operation identity, global ordinal exactly1..N, typed/nonempty unique op-scoped job_ref, optional source ordinal membership, whole group-ref/ordinal presence and each op-scoped group's ordinals exactly1..group count. Do NOT select/certify unselected job format/document bodies or delivery semantics |
| Transitions | Actual COUNT equals result.transition_count; inspect operation ownership, ordinal exactly1..N, actual INTEGER/known transition0..5. This validates the supplied scalar trace vocabulary/order only, not clock/event chronology |
| Reversal rows | Original-kind source requires result.reversal_count0 AND actual COUNT0 in reversals for that operation. It cannot hide unexpected reversal children by declaring0 |
| Selected meaning | Fully decode the selected original effect and its complete components/target, and the already-declared selected complete immutable plan/source projection, checking actual request/unit/target/provenance and actual nonnegative credited basis. All existing selected-field bounds apply. No recursive reversal-of-reversal source: original kind must0 |

Sibling metadata is visited in finite64-row keyset pages with checked65 lookahead;
identity/order counters use checked arithmetic and actual cumulative writer budgets.
Do NOT apply effects_per_bundle/jobs_per_bundle/transitions_per_bundle/result_items/
owned_bytes materialization totals to the number of historical METADATA rows. There
is no lifetime quota or unbounded vector containing all siblings. Existing bounds
still apply to fixed/actually selected owned fields and selected plan bodies; the
per-basis progress audit remains independent of full RecordedBundle read caps.

A missing unselected sibling or ordinal/group gap therefore fails this declared
structural closure. An existing unselected job document or effect rational can
still be malformed without this narrower projection certifying or decoding it;
full lookup would inspect those bodies. Nor does this projection recheck the full
original operation's snapshot/component-delta conservation from unselected amounts.
Do not call its outcome a complete-application validation or repair/replay missing
records. These scoped checks preserve the selected actual-credit bound and progress
requirements above without silently turning source resolution into a global
historical payload audit.

### Added typed reversal-basis read — pending actual extension review

Root explicitly authorizes this narrow additional projection for the minimal API;
it was NOT retroactively included in approval4d7b98a2 of the earlier five-method
contract. It avoids requiring a future caller to use private/raw SQL or fabricate
expected progress. Earlier reviewed semantics/physical14SQL remain unchanged.

Proposed const method `read_reversal_basis(ApplicationOriginalComponent)` returns
`ApplicationReversalBasisResult`: ready has one owned basis; reversal_conflict has
no basis and makes no own writes/cleanup. The locator is original OperationKey,
positive signed request ordinal and credited component kind0/2/3, with no new ID.
Pure priority is options, operation key→invalid_key, then ordinal/kind→invalid_reversal,
BEFORE ticket access. No expected prior position is passed to this read.

Ready contains the actual original locator/explicit stored SubjectKind/PlanKey,
selected complete captured revision (including immutable receipt/CID/arrival/rate/
selection/request provenance), selected complete ApplicationEffect/component set,
actual nonnegative credited units, selected before Balance only for score/count,
actual application-before token/configuration ref, optional actual contribution
original ReceiptId and current progress. Original support ReceiptId need not equal
event ReceiptId. This is DATA, not a complete original application/authority certificate.

Current progress owns actual positive position/cumulative/fraction/full flag and
last-occurrence key (current OperationKey/request ordinal PLUS original component).
For an actually resolved valid basis with NO occurrences/head it returns position0,
cumulative0,fraction0,fullfalse,no last. Missing locator uses missing_reference;
known non-original operation kind uses source_mismatch. Corrupt/dangling selected
stored closure, head without matching history, missing progress head with existing
occurrences, noncanonical selected last fields or sparse MAX use the existing
stored/reference failures and ACTUAL known rollback; no unknown source becomes a
zero basis. A valid original but ineligible actual credited basis (negative amount,
wrong arithmetic credit role or the unsupported debt-repayment allocation path)
returns reversal_conflict after its declared selected-source checks; no policy is
invented. The corresponding established stage basis/progress predicates are unchanged.

Both actual progress-head FK paths must resolve the SAME selected last occurrence,
original component/position/cumulative and its canonical fraction/full fields;
per-basis structural history uses the existing64/65 metadata audit. Historical
state/result is not compared with today's current state. No full original bundle
or unselected sibling payloads are materialized/certified beyond the exact preceding
selected-source table. Result vector/owned-byte bounds apply to ACTUALLY returned
plan children/component set/optional target and owning key/Exact fields; they do
not cap sibling/history metadata counts. Existing selected field bounds and writer
cumulative budgets remain. The snapshot may become stale before another unit;
stage rechecks actual progress/identity, not authority from this returned value.
ALL results remain provisional under the borrowed session/caller commit protocol.

## Bounds, byte roles and failure priority

Propose BundleLimits with positive qsizetype defaults:

| Option | Default / scope |
| --- | --- |
| reference_bytes |4096; all keys/tokens/format/config/target/group/job/actor/decision/plan refs including optional present strings and selected retained keys |
| value_bytes |262144; each owned document/explanation/reason, source event/actor/money/rate/factor/retained payload under existing value roles |
| component_bytes |16384; EACH canonical numerator/denominator including numerator sign, shared with selected plan Exact values; not old parser/finalizer limits |
| balances_per_state |1024; one selected/new snapshot's balance vector |
| plan_items_per_vector |1024; each selected rules/modifiers/requests vector, NOT historical revision count |
| effects_per_bundle / jobs_per_bundle / transitions_per_bundle / links_per_bundle |1024 EACH; one input/selected bundle; components max3 per effect with exact role set; links independently bounded and many per effect allowed |
| input_items |12288; checked total advanced-before+after balances+effects+components+jobs+transitions+links; fixed result/audit/contribution snapshot excluded |
| result_items |24576; checked total of all returned balance/plan-child/effect/component/job/transition/link vector elements, including all three snapshots |
| owned_bytes |1048576; checked total of owned input OR one returned aggregate payload, separately; not actual C++ heap accounting or writer binding budget |

For each count n first require n>=1, then strict n<qsizetypeMAX/qint64MAX and below
EACH relevant concrete vector.max_size(), reserving n+1. Totals below size_tMAX,
checked addition/subtraction before conversion/allocation. Fixed metadata page64
uses checked65 lookahead/continuation; no fixed SQL-call or elapsed-time promise.
Input items are checked before walking element bodies; empty vectors remain valid.

Owned-byte accounting counts byte lengths of every logical owned string/document/
canonical component,8 per uint64/signed64/position and1 per enum/bool; optional
absence0. Count a logical owned member once, not future duplicated SQL bindings.
Returned selected source/plan/state/result members count as actually owned. Heap
availability/closure pointees are not magically bounded. Preparing repeated SQL
bindings consumes the EXISTING separate cumulative writer budgets: default64
statements,1048576 binding bytes,1024 result rows/1048576 result bytes and65536 SQL
bytes. A value satisfying BundleLimits is NOT promised to fit those defaults.
SQL/budget exhaustion must honestly roll back; tests configure recorded adequate
finite budgets, never reset counters or treat selected read limits as history quotas.

Pure preflight order (before ANY ticket/SQL/cleanup): options; OperationKey/PlanKey/
source role and positive ReceiptId/revision; ALL vector/combined counts; token and
both supplied StateSnapshot scalar/key/envelope shape; expected contribution presence/15 known
state dimensions and choices; effect scalar/role/component shape then Exact bounds/
conservation in occurrence order; exact advanced-before→after aggregate deltas; jobs/groups; transitions; result/audit; reversal
key/position/Exact/local algebra; aggregate owned bytes. Inputs reject with distinct
proposed invalid_options/key/source/state/contribution/collection/effect/component/
job/transition/result/audit/reversal/exact_component_limit/owned_byte_limit categories.
Exact borrowed canonical codecs retain their own options/grammar/bound order.
Concrete header will name these categories before tests; exception text is not oracle.

Stage's stored/domain order after pure checks:

1. Existing same-key operation→FULL selected closure, then exact Intent comparison:
   already_present or operation_conflict before fresh-state/new-target evaluation.
2. Existing original subject, same-kind receipt or scoped reversal identity→validate
   its existing selected closure, then claim/conflict result; no new key alias/write.
3. Validate current head/snapshot/config; compare expected token→stale_state. For
   present CID resolve complete current contribution and compare ALL expected state
   dimensions→stale_contribution. Applied-without-new-claim→subject_unavailable.
4. Resolve candidate actual plan/source and complete structural proof; mismatch of
   healthy actual head→stale_plan. Actual event/source must match; actual typed
   references and selected immutable fields fail explicitly if missing/corrupt.
5. Validate any already-present proposed fresh before/after-token row, then
   state_token_conflict; never reuse/rebind it. Advanced-before head equality or
   permitted clock-only differences, config and exact balance-key/delta correspondence
   resolve under the same unit; no source meaning invented from string equality.
6. Resolve original reversal sources and healthy per-basis metadata; compare expected
   progress→stale_reversal. Actual basis/interval/algebra mismatch→reversal_conflict.
   Matching exhausted signed position→known sequence_exhausted, no wrap/reuse.
7. Prepare ALL allocating statements/bindings/tracking/result buffers BEFORE writes.
   Stage required immutable advanced-before/after snapshots/balances, operation, contribution snapshot/claim,
   effects/components/transitions; patch ONLY application for original; jobs/result/
   audit/reversal occurrences+heads; advance head LAST under checked persisted head token.
   Guard EVERY post-write allocation/error through real whole-unit rollback/unknown.

After existing-operation/claim handling, current-token/contribution/plan freshness
and proposed fresh-token checks, a well-shaped supplied advanced-before snapshot
must satisfy its actual persisted-head relationship. If its token equals the head
but its bound data differs, or a fresh clock-only snapshot changes preserved debt,
balance keys/values, configuration or format, report `source_mismatch` and perform
the required known whole-unit rollback. These comparisons require actual stored
head data: they are not pure input errors, `stale_state` for a matching expected
token, or `invalid_stored_data` when the selected stored rows are healthy. The
existing refusal/rollback requirement is unchanged; this fixes its error category.

Known storage reasons: busy/sql_failure/resource_limit/result_limit/missing_state/
missing_receipt/missing_contribution/missing_plan/missing_reference/source_mismatch/
invalid_stored_data/sequence_exhausted. Local known failures require ACTUAL rollback;
already-aborted writer/retention errors map without second cleanup. Pure input and
no-own-write refusals preserve prior staged caller work. Wrong-thread/stale/terminal/
protocol/invalid-statement/unmapped WorkFailure and actual unknown stay original.
Constructor/destructor never manufacture cleanup proof or authority. Stored wrong
SQL class/NULL/group/domain/ordinal/chain/canonical shape→invalid_stored_data;
well-typed selected count/byte/component overage→result_limit, never truncate. For
Exact, validate BOTH component SQL classes/nonempty shapes, then both sizes, then
canonical grammar/gcd/zero before arithmetic. Required complete groups and sibling
ordinals are not inferred from QVariant conversions or one existing child row.

## Literal examples and baseline obligations

AT01–AT21 remain binding independent examples; these are additional concrete storage
vectors, not evidence that storage computes the supplied policy. No future producer
may generate expected bytes/amounts/schema. Existing immutable prefix fixtures and
raw stdlib/SQLite can independently establish literal setup/oracles after approval.

| ID | Concrete supplied/observed storage oracle |
| --- | --- |
| SB01 | original op(scope-s,apply-a), receipt7/CID0,plan-P1,expected tokena,afterb; requested600000,role addition,components(0:+120000,1:0,5:480000),state afterMain3600000/debt0; one supplied job, one result/audit. Prior Main3480000; all become durable together ONLY after caller COMMIT/normal return |
| SB02 | same op/Intent but a new VALID computed proposal under today's state→same old record; op-B with FRESH tokenb/same CID0→subject claim. In-unit reads see own provisional records; abandon/known rollback removes ALL; separate committed branch retains exactly one set on reopen |
| SB03 | independent supplied debt example Main55/debt2/request10/cap60,components+5/-2/+3,after60/0; above-cap100/5/request8/cap60→components0/-5/+3,after100/0. These assert copied values/conservation, not execution of debt/cap policy |
| SB04 | original actual time basis300000 and score40000000; distinct refund receipt8/9 and reversal refs r1/r2,positions1/2: prior0→75000 then75000→300000 for time; requests-75000/-225000. Actual original, prior progress, source identity and exact fractions1/4 then1 resolve; no second credit or original rewrite |
| SB05 | original1 micro-point, fractions1/3,2/3,1; supplied cumulative0,1,1; requests0,-1,0; residuals1/3,-1/3,0; three distinct stored occurrences. Reused old position/cumulative is stale; cumulative2 exceeds actual1 and refuses, regardless supplied claim |
| SB06 | two identical action refs at plan positions1/2 remain distinct effect/component PKs; swapping source unit/target or referring cap-limited component5 as reversal basis fails. Optional CID NULL and BLOB0 are different subjects |
| SB07 | existing tokena can never be reused as after token, even with identical payload, after headb or in an ABA attempt. Same token/different typed/document content never upserts. A changed current14 review/evidence cell with unchanged token returns stale_contribution, and successful original patch preserves every other CURRENT cell |
| SB08 | sparse plan1,MAX-1,MAX cannot satisfy application proof, even though old targeted MAX DTO is possible. Actual uint64 foreign fields use BLOB8/typed full range; codec0 does not become ReceiptId/revision0; no fake valid MAX-history exhaustion |
| SB09 | conditional disposable job/result INSERT trigger raises real SQL error ONLY after actual intent/effect/after-state/current application patch are staged. Capture actual reached preconditions via raw helper proof; inside-catch independent BEGIN IMMEDIATE plus complete typed snapshot proves whole rollback while facade lives. Fault-off retry starts from durable prior state |
| SB10 | actual staged whole bundle/headb, caller COMMIT ack-loss→unknown/no normal payload/no replay, possibly whole bundle durable. Cleanup ack-loss outranks selected local failure; no inactive cleanup becomes known rollback. Later response failure cannot undo a normally completed committed bundle |
| SB11 | exact3/2 request2 residual-1/2 and5/2 request2 residual+1/2; signedMIN request is not negated unsafely. Larger correctly typed component→result_limit before grammar; fitting noncanonical/SQLTEXT/equivalent numeric class→invalid_stored_data. A fitting >4096-bit canonical value can be preserved as storage-only data; this does not claim the separately bounded finalizer/actual application would accept it |
| SB12 | corrupt/missing selected result, audit, declared sibling, claim, source or progress head fails without repair. Historical lookup after head/counter/contribution changes returns old snapshots, not a false comparison with today's state. Large history is audited in finite64-page queries with actual cumulative budgets |
| SB13 | one captured refund request ordinal1 aggregates two original Main credits100 and50. Links to those distinct originals move cumulative0→20 and0→10; increments-20/-10 SUM to the immutable request-30. ONE current effect occurrence remains ordinal1 with TWO links; no adapter-generated request ordinal or synthetic plan revision. Stale/over-original relation on either link prevents the WHOLE bundle |
| SB14 | actual original score100, immutable refund request-20, fraction1/5/prior0/new cumulative20/linkincrement-20: sole direct score component MUST be-20. Component-1000 is invalid even if after balance was adjusted by-1000 and every link field looks valid. This is local copied-amount conservation, not rerunning refund evaluation |
| SB15 — added typed-read example, pending actual extension review | actual original Main credit100 with no progress→ready credit100/ms/original plan/receipt/CID provenance plus position0/cumulative0/fraction0/fullfalse/no last. After actual stored occurrence position1/cumulative20/fraction1/5, return those exact current fields and the complete last-occurrence key. Missing original is a typed reference failure, not ready credit0; missing unselected sibling fails metadata, while unselected body payload is not certified. No helper evaluation, whole-bundle materialization or authority follows |

## Compatibility and next gates

Initial state and job envelopes have explicit storage-only synthetic meanings in
physical companion. No complete job delivery schema, timer recovery checkpoint,
permission proof, provider correlation or refund Main/debt allocation is finalized.
New typed balance namespaces are actual snapshot rows, not fake semantic config FKs.
No old interface/schema/fixture/assertion/producer is edited by these documents.
Root owns exact additive source/manifest/order/build/pins/progress integration.

Different ACTUAL contract/physical review precedes minimal API+immediate named stubs;
then different actual API/build review, independent fixture/old-writer green and
feature test author/review, selected official environment gate and first intended
red. Only reviewed observed missing behavior authorizes STORAGE-002 implementation.
All stable checks remain. Full seven integrations/three FX, UI/renderers/locales/
themes/platform/package/security/backup/recovery/live scope stays required.
