# SQLite original-application claim lookup — concrete proposal revision 1

Task: STORAGETEST-017, before STORAGE-017. Authority: specification v0.45 §§3.4,5,
DB-01, NUM-04; [original service logical contract](application-service-v1.md),
ACCTCONTRACT-001; [bundle contract](sqlite-application-bundle-v1.md) and its
[unchanged physical step5](sqlite-application-bundle-schema-v1.md).
This is DOCUMENT/API-proposal work only. No header, stub, query implementation,
fixture, feature test or intended red is supplied or approved by writing this text.
Different actual document review precedes the minimal API/stub phase and its review.

## One additive query over the existing namespace

The existing store can look up an operation key. Its `stage` detects subject/source
claims only after a complete prepared bundle's pure preflight. The new query lets
an application service detect those prior claims BEFORE computing today's amounts,
without raw SQL or a dummy prepared application. It never applies anything.

One store/database represents this installation's one subathon. Original tagged
subject claims and `(operation_kind,receipt_id)` uniqueness are GLOBAL in that
existing database, including operations with different `operation_scope` values.
The query MUST NOT filter either lookup by a proposed operation scope. A new scope,
key, plan revision, confirmation receipt or requester cannot bypass an original
subject claim. Actual installation binding, source classification and current
execution/disclosure permission are service obligations, not inferred here.

No schema, migration, allocator, identifier domain or existing stage/read behavior
changes. In particular, existing physical `ApplicationIntent` equality remains
exactly as reviewed: its complete structured members include the historically
checked state token and choices. This query neither weakens that equality nor
replaces it with service-command equality.

## Proposed concrete API and owned values

Add only these values and one const method to the existing bundle public boundary:

| Proposed name | Exact members / meaning |
| --- | --- |
| `ApplicationOriginalClaimSelector` | Existing `ApplicationSubjectKind subject_kind`; `core::ReceiptId receipt_id`; `std::optional<core::ContributionId> contribution_id` |
| `SqliteOriginalClaimKind` | `subject=0`, `source=1`; this says WHICH lookup selected the record, not whether a command is authorized/equal/committed |
| `ApplicationOriginalClaimMatch` | `SqliteOriginalClaimKind kind`; owned complete `RecordedApplicationBundle existing` |
| `lookup_original_claim(const ApplicationOriginalClaimSelector &) const` | Returns `std::optional<ApplicationOriginalClaimMatch>` on `SqliteApplicationBundleStore`; no match is absent, a match ALWAYS owns the complete validated existing record |

A selector's role is explicit. `contribution` requires a present CID, including0;
`receipt` requires absent CID and caller-established non-support classification.
Missing CID alone does not establish that classification. ReceiptId is ALWAYS
positive, including when a CID is present, because it is also the source fallback
key. Full values1..UINT64_MAX and CID0..UINT64_MAX use existing typed BLOB8 codecs;
there is no unsigned-to-signed ID narrowing, numeric zero for absence, or new codec.

There is deliberately no operation key/scope, plan, preview, choices document,
expected current contribution, state token, effect, job or computed bundle in the
selector. Existing `lookup(OperationKey)` remains the exact-key operation query.
There is no reversal-identity selector or reversal claim match in this bounded
ORIGINAL-only extension; existing stage/reversal APIs retain their own behavior.

Construction/copy/destruction of the existing facade stays inert. The query borrows
its actual active revocable session; it does not BEGIN, adopt another transaction,
commit, clean up on normal return/destruction, create another connection or claim
that an earlier unit committed. Returned strings/documents/vectors are fully owned;
caller mutation/lifetime cannot rewrite them or durable records.

## Pure priority before ticket access

Use the existing `SqliteBundleLimits` and input errors, without changing their rules:

1. Validate ALL existing bundle options first, including positive byte limits and
   reserved count/max_size ceilings. Dormant output options still must be valid.
2. Require known SubjectKind, positive ReceiptId and CID presence exactly matching
   the chosen role. Any violation is `SqliteBundleInputFailure(invalid_source)`.
   Wrong-role CID0 is not interpreted as absence or repaired into another role.
3. Only then access the actual session/SQL.

There is no string operation key, so no invented `invalid_key` condition. The
selector has no variable strings/vectors: do not apply `input_items` or an aggregate
input `owned_bytes` check to its fixed scalar fields, or echo/count them in the
result. This follows existing read-only preflight; a valid selector with
`owned_bytes=1` passes pure validation and can then encounter actual stale-session
or selected-result limits. No query-specific options or lifetime quota is added.

Pure errors perform NO SQL, writes or cleanup and preserve earlier staged caller
work. They precede expired/wrong-thread/session errors. A valid selector must
retain the actual capability error before accessing any owner SQL on a foreign
thread; the facade acquires no construction-thread authority.

## Selection, precedence and strict matched identity

After pure preflight, perform this bounded selection order on the caller session:

1. Select the original claim by EXACT tagged subject:
   `(subject_kind, subject_id)` where subject_id is the present CID or ReceiptId
   according to the explicit role. Decode its actual stored operation key strictly.
2. If a subject claim exists, resolve its target with the complete selected closure
   of existing `lookup`. Missing/dangling/inconsistent target is a known stored-data
   failure, NEVER permission to fall through to a different source result. Verify
   the returned operation is ORIGINAL and its actual role/tagged ID equals the
   selected subject. Its own full original claim must identify that SAME operation
   and subject, not a different individually valid claim. Return `{subject,record}`.
3. Only when the subject lookup is absent, select the ORIGINAL-kind operation whose
   actual receipt_id equals selector.receipt_id. Strictly decode its operation key
   and resolve the SAME complete selected closure. Verify actual kind ORIGINAL and
   actual ReceiptId equal the selected source. Return `{source,record}`.
4. If both lookups are absent, return `nullopt` without candidate-source admission,
   current state/eligibility reads or global orphan/history audit.

The selected match's historical event receipt need NOT equal the selector receipt
for a SUBJECT match: a later linked confirmation can name the same contribution.
A SOURCE fallback's actual subject kind/CID need NOT equal the requested subject;
that is why the returned discriminator and complete actual record are explicit.
For example, changing a requested contribution subject to receipt role cannot
award an already-used original receipt through a different namespace.

A typed selector is a query predicate, not proof that its candidate ReceiptId and
CID currently correspond. It may find a subject even when its candidate source
would fail later admission. Absence does not inspect whether candidate receipt/CID
exists, was committed earlier, is linked or is eligible. Do NOT add implicit
`missing_receipt`, `missing_contribution` or candidate `source_mismatch` validation.
If an old contribution row is applied but has no application-bundle claim, this
reader does not fabricate an old bundle: absent remains absent in this namespace;
the service's later current-input path separately handles subject_unavailable.

Subject priority applies even if source fallback would select another healthy
operation, or if a lower-priority unselected record is corrupt. Only the chosen
record is fully certified. On absence there is no global malformed-row scan.
Queries still validate every actually selected key's SQL class, length, full ID
range/role and match-to-record identity; SQL predicate coercion is not a substitute.

## Inherited closure, bounds and error outcomes

A present match uses FULL existing `lookup` closure, not the narrower reversal-basis
projection. Validate operation/Intent, all three historical snapshots/configuration/
balances, historical contribution snapshot/application_after and actual current CID
identity/original-receipt target where present, exactly matching original claim,
immutable selected plan/source and healthy complete scalar history, all declared
versus actual effect/component/job/group/transition/reversal counts and ordinals,
full selected body types/canonical Exact/local and aggregate conservation, one
result and one audit. No missing result is synthesized or repaired.

The historical head/before/after and contribution snapshot are NOT compared with
today's mutable state head/current contribution dimensions, and no current cap,
eligibility, new plan, numeric effect or action set is calculated. A later healthy
plan head is allowed if the original selected revision resolves in its complete
healthy history. Sparse1/MAX-1/MAX plan history remains INVALID; a targeted MAX DTO
from another port is not a valid full application source or exhaustion proof.

Reuse the exact existing selected field/vector bounds, SQL-class-before-size/
canonical-grammar error order, actual-count checks before full vector allocation,
checked result totals and64/65 structural history walks. Metadata traversal does
not acquire a lifetime materialization quota. The new result has these exact totals:

- `result_items` counts the existing complete RecordedBundle vector elements ONLY.
  The fixed wrapper/enum and selector add no vector elements. The existing original
  fixture has20:2+2+2 snapshot balances,4 effects,8 components,2 jobs,0 transitions,
  0 reversals. Internal selected plan/provenance DTOs used for closure are not
  returned members and do not add fictional result elements.
- A present match's `owned_bytes` is EXACT complete RecordedBundle logical bytes
  PLUS1 for the new subject/source enum. Count every actually returned owned
  string/document/canonical component and scalar under the existing byte roles;
  the selector is not echoed. Optional absence adds0 bytes/items, and optional
  engagement is not an extra invented boolean byte.
- Compute the extra enum total using checked addition or the equivalent safe
  subtraction after options validation. No wrap/narrowing or change to old
  lookup/stage/basis accounting. The selected full record is validated under its
  existing per-field/structure/result bounds; then the new wrapper's total must
  fit. A wrapper one-over is `result_limit` after actual whole-unit rollback.
  Never truncate, omit the discriminator or return a partial body to fit.
- All SQL statement/binding/result-row/result-byte budgets remain the SAME actual
  cumulative writer budget, including selector queries plus complete lookup.
  No reset, separate writer/session, cached fake result or fixed-call-count promise.

Malformed actually selected SQL type/NULL/group/key/domain/count/ordinal/crosslink/
canonical data, missing claimed operation/closure, or a match inconsistent with its
selected subject/source is `invalid_stored_data`, preserving existing lookup's
field/structure priority. Well-typed selected field/vector/aggregate/wrapper overage
is `result_limit`. Known busy/sql/resource/result failures map through existing
`SqliteBundleStorageFailure` reasons only after actual rollback is established.
Local known selected-data failures roll back the ENTIRE caller unit, including
prior unrelated staged work, before exposing that known failure. Already-aborted
writer/retention errors are not 'proved' with a second terminal cleanup operation.

Wrong-thread, stale/terminal session, protocol/invalid-statement/unmapped WorkFailure
and actual uncertain COMMIT/rollback remain the original `SqliteWorkFailure` truth.
Unknown outranks a local selected-data error; fail-close/no normal payload/no blind
retry is unchanged. A terminal-session call can make this work's protocol error
sticky even when caught, as the existing writer contract specifies. The query
never substitutes a known recorded application for the outcome of THIS unit.

Healthy present/absent results are read-only and make no own cleanup. They can see
application rows/claims staged earlier in the SAME unit, so they are provisional.
Actual caller rollback/abandonment removes that entire staged application; actual
COMMIT and normal work completion instead expose it once. An old known-durable
record still cannot turn this attempt's read-only COMMIT acknowledgement loss into
normal success. Publication, access, retry/outcome resolution and hold policy stay
outside this query.

## Independent literal examples for the later fixture author

These examples are DOCUMENT oracles, not executable tests/producer output. Actual
independent frozen fixture authoring follows document/API approval. All referenced
full records must have real retained source/configuration/plan/claim/result/audit
closure; raw claim rows alone are not a substitute.

For priority/full-range examples define two healthy ORIGINAL records A and B in
one database, in this order. This is a NEW minimal fixture proposal, distinct from
the older independently frozen twenty-item original fixture used for materialized
vector-bound examples:

| Member | Record A | Record B |
| --- | --- | --- |
| OperationKey | `(alpha,A)` | `(beta,B)` |
| Actual selected receipt / tagged CID | Receipt7 / CID0 | ReceiptUINT64_MAX / CIDUINT64_MAX |
| Plan key / complete history | `(PA,1)`, only revision1 | `(PB,1)`, only revision1 |
| Historical head / before / after token | a / a / b | b / b / c |
| Snapshots | Each token a,b,c: format `s`, present-empty BLOB document, actual retained config `cfg`, Active/Main10, Grace120000, waitfalse, debt0, no balances | Same complete immutable snapshot values except token |
| Captured/current-before contribution | PaymentEvidence not_applicable; monetaryfalse, no original money/resolution, missing_ratefalse, reviewnot_required, EvidencePolicy accept_provider_report with policy_ref `p`, no review decision, applicationnot_applied, reversalnone, settlementclear | Same conditions; distinct actual source/CID |
| Historical application_after / current row | applied / applied | applied / applied |
| Original/support / source closure | Actual receipt7 with complete immutable identity/arrival, bounded support observation linkedCID0; plan event7/CID0; known retained cfg/context | Actual receiptMAX/support linkedCIDMAX; plan eventMAX/CIDMAX; known retained cfg/context |
| Plan scalar/body | No parent/override, valuationnot_applicable, zero rules/modifiers/requests and valid source facts with no money | Same structure, distinct source identity |
| Effects/components/jobs/transitions/reversals | All empty; all result counts0 | All empty; all result counts0 |
| Choices / result / audit | Format `c`/`r`/`u`, each present-empty BLOB; actor/decision/reason absent; advance_explanation empty | Same |
| Actual matching original claim | tagged contribution/CID0 → alpha/A | tagged contribution/CIDMAX → beta/B |

For both sources, pin retained configuration `cfg` and arrival context `ctx`
(linked to cfg), both with present-empty documents. Receipt origins are
`(synthetic,installation)`, identifier namespace `event`, distinct opaque IDs
`receipt-A`/`receipt-B`; event type `synthetic.support`, schema1, optional source
instant/actors/money/payment/activity references absent, arrivalUTC0, connection
`connection`, cfg/context refs `cfg`/`ctx`. Plan event/root refs are `event-A` or
`event-B`, parent absent. Contribution activity identity is absent; its bounded
support observation uses the same receipt, support kind, evidence not_applicable,
these exact support conditions, no verified activity and its actual CID. Both
current contribution rows retain those source fields and applicationapplied;
current state head may be c. New fixtures must preserve real required allocator/
catalog rows from the independent old foundation, without invoking allocators or
claiming MAX-plan/history coverage.

Unknown provider facts remain absent; these are explicit storage-only synthetic
meanings, not financial confirmation, current permission or production action
capability. Source/support payload values do not appear in the returned record
except its existing owned members; the later fixture author must freeze their
actual independent literal setup and complete constraints.

For A, literal logical RecordedBundle bytes are161: Intent45 + three snapshots
32+32+32 + historical contribution18 + result1 + audit1 + empty advance/children0.
Intent45 is operation6 + kind/subject2 + Receipt8 + CID8 + PlanKey10 + headtoken1 +
expectedContributionState9 + choices1. Each snapshot32 is token1+format1+document0+
configuration3+timer/debt27. Historical contribution18 is CID8+state9+application1.
The new matched wrapper is162; existing lookup(A)'s161 is unchanged. The two
record scopes/references, typed MAX IDs and empty vectors do not create a MAX plan
history. The following are required observable outcomes:

| ID | Selector / condition | Required result |
| --- | --- | --- |
| OC01 | contribution/CID0, ReceiptMAX with BOTH healthy A/B present | SUBJECT A; first match wins even though source fallback would be B in another operation scope; do not reject candidate source/CID association or inspect B merely to return A |
| OC02 | receipt role/noCID, ReceiptMAX with A/B present | No receipt-tagged subject exists, then SOURCE B; actual stored subject remains contribution/CIDMAX and returned operation scope remains beta |
| OC03 | contribution/CID0, Receipt8 from a later linked confirmation | SUBJECT A whose original application event is7, not8; confirmation cannot become another award. Candidate8 need not be reread to return the selected A |
| OC04 | contribution/CID42, Receipt9, neither predicate matched | Absent; no invented record, source-admission failure, current-state read or global orphan audit, even if candidate9/42 do not exist |
| OC05 | known role but Receipt0, or receipt role with present CID0, or contribution role without CID | Pure invalid_source before ticket/SQL/cleanup. CID0 with positive Receipt7 and contribution role remains valid; UINT64_MAX is preserved exactly |
| OC06 | options owned_bytes0 plus invalid selector on expired/foreign ticket | invalid_options first. Valid options plus invalid selector→invalid_source first. Valid selector→actual stale_session/wrong_thread/session_closed; no owner SQL access on foreign thread |
| OC07 | A selected, new owned_bytes162 then161 |162 returns complete SUBJECT A;161→known result_limit/whole caller rollback. Existing operation lookup(A) with161 still fits. Selector fixed scalars are not echoed/count-budgeted |
| OC08 | Existing older full twenty-item original, result_items20 then19; input_items1 in both |20 returns full match;19→known result_limit/whole caller rollback. The new enum adds1 byte,0 vector items; no input quota substitutes for returned quota |
| OC09 | Actually selected A missing result/audit/contribution snapshot, source7 fallback selects A whose own claim is missing, or selected claim has dangling target; old nonempty fixture supplies selected job/canonical/group corruption | Full selected invalid_stored_data with ACTUAL rollback before inside-catch independent BEGIN; no source fallback after selected subject corruption, no repair/synthesized success. A minimal zero-job A cannot lose a nonexistent job; child corruption examples use the independent nonempty old fixture |
| OC10 | Healthy A selected while today's state head, current review/evidence or a later healthy selected-plan head differs | Return historical A exactly; no current revaluation/eligibility overwrite. Corrupt original immutable selected source/plan history still fails; sparse1/MAX-1/MAX is invalid, never full healthy history |
| OC11 | A staged in caller unit, then new query; branch rollback versus commit/reopen | Own query returns provisional SUBJECT A. Outside reader sees no new claim until COMMIT. Known rollback removes ALL staged application/claim; committed normal completion/reopen retains exactly one A |
| OC12 | Healthy A/B reads with caller marker already staged, followed by explicit COMMIT | Read makes no own write/cleanup; marker remains active and commits. Absent query behaves identically with respect to caller work |
| OC13 | Selected corruption with actual rollback acknowledgement loss, or healthy existing read followed by actual COMMIT acknowledgement loss | Preserve actual unknown WorkFailure/fail-close/no normal payload, not a recorded-application success or fabricated known rollback; no blind retry |
| OC14 | Change caller buffers/destroy query facade after return, or use copied ticket on foreign/expired/terminal phase | Returned complete record owns its bytes; facade destruction never commits/cleans; pure/capability/error priority and writer sticky terminal semantics remain truthful |

## Service ordering, compatibility and next gate

The service's ordering is current execution access → existing operation-key lookup
and authorized complete COMMAND comparison → this typed claim query and current
historical disclosure authorization → active hold query → actual fresh input/
policy/clock/numeric work. This query supplies no command parser or authority.
Its returned choices remain opaque, including valid old synthetic formats. The
service canonical-command codec/binding is separately reviewed; it may not compare
a newly derived stage token with an old command's preview to manufacture conflict.

No old header fields/enums/methods, source behavior, schema/migration/manifest,
35 reviewed bundle/209 full-preset assertions or fixture data is weakened by this
proposal. Current APIs can still stage/read the same records. Only a future
additive declaration/new immediate named missing-feature stub is authorized after
actual document approval and explicit phase assignment. The stub belongs only to
this new entrypoint; existing implemented methods are never relabeled unimplemented.

Then independently review the actual header/stub/harness, author separate frozen
fixtures/real-SQL tests, review those exact tests, compile/discover, observe the
new reader's own intended stub-red, and retain its raw evidence. Old209, old bundle
red or this document's examples cannot substitute for it. STORAGE-017 production
waits for that gate and later separate code review/real acceptance/ordinary CI.
No service/application/permission/provider/hold/recovery/UI/platform capability is
inferred. All seven integrations/three rate providers and the full agreed release
scope remain mandatory; no live provider or supporter data is involved.
