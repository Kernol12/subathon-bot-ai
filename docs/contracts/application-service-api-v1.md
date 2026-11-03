# Original application service — concrete API and codec proposal v1

ACCTTEST-004 DOCUMENT stage, from integrated main efe1c7e and claim5236d88.
Authority: [logical service](application-service-v1.md), blob
6d0cd67f487dae92a17d41eb07dda5b407c78a56; [ADR-0003](../decisions/0003-application-service-prerequisites.md),
blob e971a47e64b7cabffa19e325cee698378a2d0a8a; specification v0.45 §§3–5,
NUM-01–04, B08 and AT01–21. STORAGE-016 and STORAGE-017 are now integrated.
This proposes the next C++ boundary and independently specifiable bytes, not an
implemented codec, header, stub, fixture, service test or acceptance claim.
Different actual document review precedes every such artifact.

## A. Owned command and public surface

Namespace subathon::server; proposed header application_service.hpp and source
application_service.cpp. Reuse the actual ApplicationOperationKey, ApplicationPlanKey,
ApplicationEnvelope, ApplicationSubjectKind, ReceiptId/ContributionId, Exact,
ApplicationStateSnapshot, RecordedApplicationBundle, ApplicationHoldSubject,
ApplicationHoldActiveSet, ApplicationHoldMutationOutcome and SqliteWorkSession.
No existing storage signature, physical equality, migration or public core type changes.

Proposed enum values are explicit bytes, independent of C++ sizeof:

| Type | Values |
| --- | --- |
| ApplicationPrincipalKind | account0, automation1 |
| OriginalApplicationPurpose | captured_original0, signed_original_correction1 |
| ApplicationPreviewMode | revalidate_current0, require_exact1 |
| ApplicationReferencePurpose | advanced_state0, after_state1, hold2, detection3, hold_version4 |

OriginalApplicationCommand owns these members IN THIS ORDER:

| Member | C++-ready type / condition |
| --- | --- |
| operation | ApplicationOperationKey, both opaque references nonempty |
| subject_kind, receipt_id, contribution_id | ApplicationSubjectKind, ReceiptId, optional ContributionId; receipt positive; contribution role requires CID including0, receipt role forbids CID and requires trusted non-support classification |
| plan | ApplicationPlanKey, nonempty ref and revision1..UINT64_MAX |
| principal | ApplicationPrincipalBinding{kind, installation_ref, principal_ref}; both refs nonempty; stable account/automation identity, never connection identity |
| purpose | OriginalApplicationPurpose |
| preview_mode, preview_state_token | ApplicationPreviewMode, optional string; present token nonempty; require_exact requires presence |
| actor_ref, decision_ref, reason | optional string each; presence differs from empty; correction requires nonempty decision_ref and reason; actor stays honestly optional |
| choices | vector<ApplicationEnvelope>, ordered; format nonempty, document non-null, empty document valid |

Choices are complete explicit caller choices, not newly calculated effects, current
cap, today's policy, an allocated token or a substitute for trusted authority.
An unknown choice format is retained losslessly but cannot authorize unsupported
behavior. Policy/action adapters must explicitly understand applicable choices.
The purpose tag distinguishes an authorized ORIGINAL signed correction from financial
reversal; it does not derive the correction request or grant manual permission.
Negative requests require the correction purpose and its explicit per-occurrence
policy. CORE-008/API-007 retain manual derivation/preview/edit/permission obligations.
Financial reversals dispatch to ACCT-006 and are not accepted as this command.

Proposed callable boundary:

~~~cpp
ApplicationEnvelope encode_original_application_command(
    const OriginalApplicationCommand&, const ApplicationServiceLimits& = {});
OriginalApplicationCommand decode_original_application_command(
    const ApplicationEnvelope&, const ApplicationServiceLimits& = {});

class OriginalApplicationService final {
public:
    using Completion = std::function<void(ApplicationServiceCompletion)>;
    OriginalApplicationService(SqliteWriter&, ApplicationServicePorts&,
                               ApplicationServiceLimits = {});
    ApplicationServicePreparedWork make_work(
        const OriginalApplicationCommand&, Completion) const;
    ApplicationServiceAdmission enqueue(const OriginalApplicationCommand&, Completion);
};
~~~

ApplicationServicePreparedWork owns SqliteWriter::Work body and its MATCHING
SqliteWriter::WorkCompletion completion. make_work performs the same pure owned
preflight/copy as enqueue, but admits nothing. enqueue uses precisely that pair with
the real writer. The trusted queue host submits each pair ONCE; it may wrap body
for a test: call original_body(actual_context), then throw before returning its
QVariant. This proves AS15 using the real acknowledged COMMIT followed by outer
body exception. No new SQL/fault instrumentation or fake completion is needed.
The matching completion owns per-attempt error bookkeeping, never a substitute
normal payload. Reusing/cross-pairing body/completion is prohibited host protocol.

The constructor is inert; references are owned by the authoritative queue host and
outlive admitted work. No facade escapes a session. The host keeps service and ports
alive through completion, closes admission before destruction and drains its queue;
this is not a transport or a new thread/event loop. enqueue is owner-thread-only.
Foreign calls return wrong_thread without storage or port calls. Invalid command,
limits or missing callback is a synchronous typed admission/input failure, before
enqueue_work; no callback/unit is promised for rejected admission. Accepted admission
copies all strings, vectors and QByteArray bytes, including fromRawData sources.
It never retains a caller pointer, stack reference or mutable shared input buffer.

The body returns QVariant::fromValue(owned ApplicationServiceResponse), using one
declared/registered metatype. The bridge requires the EXACT metatype, a non-null
owned response and its disposition/payload invariant, without coercive QVariant
conversion. Returned values are still provisional, including duplicate and held.
The matching completion consumes ONLY the actual writer WorkResult.value; an
invalid/missing/wrong-type value after otherwise normal committed completion is
a bridge failure with no response/publication, never guessed from a capture.
The host supplies real WorkResult rather than constructing a synthetic success.
Copied wrapper values deep-own all nested bytes; their lifetime is independent
of SQL/session/service destruction. No public generic QVariant becomes authority.

ApplicationServiceAdmission contains StorageError from the actual enqueue_work
admission, none exactly when accepted. It does not mean applied or committed.
Pure codec invalid input throws ApplicationServiceInputFailure; unknown format
throws ApplicationServiceUnsupportedFormat containing only the bounded format ref.
The service's only later minimal missing-feature marker will be uniquely named
OriginalApplicationServiceNotImplemented; codec stubs, if separately exposed, must
also fail unmistakably rather than manufacture valid-looking bytes.

## B. Canonical binary grammar

All documents below use this ONE grammar. Strings are arbitrary owned exact bytes,
without Unicode normalization, NUL termination or locale conversions. References
are nonempty where their field requires it. No C++ object representation is stored.

| Notation | Bytes |
| --- | --- |
| U8(x), Bool | exactly one byte; Bool is0/1 |
| U32(x), U64(x) | unsigned big-endian4/8 bytes |
| I64(x) | big-endian8-byte two's-complement signed value |
| B(x) | U32(byte length), then exactly those bytes; empty is00000000 |
| O(T) | 00 absent, or01 followed by encoding T; no other tag |
| V(T) | U32(count), then each T in retained order |
| Env | B(format_ref), B(non-null document) |
| Op, Plan | B(scope),B(operation_ref); B(plan_ref),U64(revision), respectively |
| Exact | B(canonical signed numerator ASCII),B(canonical positive denominator ASCII) |

Exact numerator has no '+' or leading zero, no negative zero; denominator positive
with no leading zero; gcd(abs(n),d)=1, zero is0/1. Existing exact bounds are checked
before constructing large integer values. Bytes/counts/additions/conversions are
checked before reserve/allocation/read; truncated fields, trailing bytes, unknown
enum/tag, noncanonical Exact, invalid role/presence or bound excess are invalid.
An unknown outer format is unsupported, not malformed known-v1 data.
No hash replaces full bytes or complete command comparison.

Command format is subathon.internal.application-command.v1. Its document is:

~~~text
ASCII "SAC1"
Op
U8(subject_kind) U64(receipt_id) O(U64(contribution_id))
Plan
U8(principal.kind) B(principal.installation_ref) B(principal.principal_ref)
U8(purpose) U8(preview_mode) O(B(preview_state_token))
O(B(actor_ref)) O(B(decision_ref)) O(B(reason))
V(Env choices)
~~~

Decode then encode is byte-identical for accepted v1. Command equality compares
EVERY member, including operation key, all tags, original preview/mode, stable
principal, ordered choices, absent versus empty and raw bytes. No field is excluded
as "only metadata." Operation retry compares this retained original command, not
today's physical ApplicationIntent.expected_state_token or a recomputed proposal.
A new operation key can never be an exact operation retry.

Independent literal command C0 uses scope s, operation o, receipt role/receipt7/noCID,
plan p/revision1, account/installation i/principal u, captured_original,
revalidate_current, all four optionals absent, no choices. Concatenate these lines:

~~~text
53414331
0000000173 000000016f
01 0000000000000007 00
0000000170 0000000000000001
00 0000000169 0000000175
00 00 00
00 00 00
00000000
~~~

C0 is58 bytes. Changing to contribution role/CID0 changes role01→00 and CID00
to010000000000000000:66 bytes. Present-empty reason replaces its00 by0100000000:
C0 becomes62 bytes; it remains different from absent. One choice format x and
document EF BB BF 00 FF 41 replaces final count by00000001 and appends
0000000178 00000006efbbbf00ff41: C0 becomes73 bytes. Positive receipt MAX,
CID MAX and plan revision MAX are all ffffffffffffffff in their OWN typed fields;
CID0 is not NULL, and receipt0/revision0 remain invalid. These literals were
derived from the grammar, not an application codec or storage helper.

## C. Trusted synchronous ports

ApplicationServicePorts groups references to the following interfaces; all methods
run synchronously on the work's actual owner thread. They receive a finite
ApplicationReferenceReadView, NOT SqliteWorkContext/SqliteWorkSession, raw SQL,
a whole mutable store or writer. Its only method is
find_retained(const RetainedKey&) -> optional<RetainedDocument>, backed privately
by the actual SAME session's SqliteRetainedContextStore::find. Its options are
reference_bytes=min(service.reference_bytes,bundles.reference_bytes) and
document_bytes=min(storage_document_bytes,bundles.value_bytes); it preserves
actual capability, cumulative budgets and known/unknown failure truth.
The view is revoked at callback return; retaining/copying it cannot extend access.
Inputs/selected capture/current state are supplied as owned DATA; no additional
current-source/state read or mutable repository capability is exposed.
Actual future authority adapters needing other persistence reads require a focused
finite read-view extension/review, not an arbitrary SQL escape. They MUST NOT write SQL,
commit/adopt/rollback, open another reader/connection, enqueue work, pump events,
call network/provider services or publish. Healthy refusal therefore stays read-only.
Authority/configuration mutation is serialized by this same queue. An adapter unable
to satisfy these constraints is unavailable, not a permissive fake.

| Proposed method / result | Exact meaning |
| --- | --- |
| ExecutionAccess::check(read_view,command) → AccessDecision | current authenticated installation/principal binding, purpose/source/plan/choices grants; allowed/denied/unknown plus actual permission_revision_ref on allowed. Opaque command caller refs do not prove authentication |
| ExecutionAccess::disclose_application(read_view,command,record) → bool | separately authorize actual recorded operation/source/result, even for same key or subject; failed check exposes no record |
| ExecutionAccess::disclose_holds(read_view,command,active_set) → bool | separately authorize actual complete hold details; false allows only protected-details-free active_hold disposition |
| CurrentApplicationPolicy::resolve(read_view,command,inputs,access) → PolicyResolution | supported bound policy or explicit unavailable(reason); successful binding below |
| SegmentedApplicationClock::observe(read_view,command,inputs,policy) → ClockResolution | supported bounded observation or unavailable(reason), never UTC-minus-monotonic reconstruction |
| RequiredApplicationActions::prepare(read_view,command,inputs,policy,proposal) → ActionResolution | supported owned ordered required job intents plus explanation, or unavailable(reason); no effect/state mutation, no delivery |
| FreshApplicationReferences::issue(purpose,binding) → ReferenceResolution | ONE bounded owned nonempty opaque ref or unavailable; no durable allocator side effect or retry-until-unique loop |

AccessDecision is denied or unknown with NO grant token, or allowed with actual
nonempty permission_revision_ref. This is current trusted port DATA, not a reusable
public permission capability. A second disclosure check cannot be skipped because
an apply grant exists. No access decision is cached across attempts.

BoundApplicationPolicy owns: operation, exact source selector, PlanKey, checked
persisted state token/configuration_ref, permission_revision_ref, nonempty
policy_revision_ref, optional nonnegative maximum_main_ms, full core::TimerConfig,
current independent paused/recovery flags and version/explanation, admission outcome
and ordered signed-correction policies keyed by actual request ordinal.
All echoed bindings must exactly match the checked input/access result. The resolver
must honestly resolve the retained configuration meaning; refs/envelopes/booleans
alone cannot establish it. Missing configuration meaning or stale bindings are not
guessed from a document. An invalid echo/group is a port-contract failure with
whole-unit rollback; a genuinely unavailable policy is an explicit not-ready/hold
path below. The policy cannot choose a new capture, initialize a counter, reopen,
clear pauses, revalue history or override independent contribution blockers.

Admission policy outcomes are allow_active, deny_with_reason, unresolved_with_reason
or requires_other_command. No v1 outcome enables a mixed timer RequireReview bundle,
post-finish mixed time/score, paused acceptance, recovery completion, start/reopen,
financial refund, batch/silent review or milestone Run/Defer/Skip. Those paths
require their owning task's concrete reviewed representation/policy before enablement.
This is an explicit gate, not an implicit product choice to defer all timer deficits.

ApplicationClockObservation owns nonempty clock_lineage_ref/observation_ref,
configuration_ref/policy_revision_ref/checked_head echoes, observed_utc_ms and
vector<core::TimerClockStep>. Each elapsed_ms is nonnegative; UTC and monotonic
elapsed stay separate. Segments are ordered, pause is constant WITHIN each segment,
their checked elapsed sum fits signed64, and the final UTC equals observed_utc_ms.
The actual adapter supplies a valid complete interval from the retained observation,
including boundaries at pause changes and configured hard end. It must not fabricate
segments from only today's pause boolean. An empty vector means the SAME retained
observation/UTC and unchanged binding; it is not permission to skip a newly reached
deadline. A zero-elapsed final segment can observe changed UTC/hard end.
Clock lineage is an explicit retained-observation identifier, not a process-local
tick or portable downtime checkpoint. Restart/recovery meaning remains separately
reviewed; unavailable lineage/segmentation is not-ready.

Reference binding owns command operation/source/PlanKey, persisted head token,
purpose and optional request ordinal when meaningful. Call only after admission and
the complete proposal requires that reference; hold detection uses hold/detection/
hold_version, successful application uses advanced_state only if needed and after_state
always. Verify returned keys/purpose and all within-attempt required distinctions.
The store enforces actual namespace non-reuse; collisions map explicitly below.
No regenerate loop, fake sequential StateRevision or issuer claim of durability.
Production uniqueness/exhaustion semantics require a real reviewed issuer; a
deterministic finite scripted fake establishes baseline order/collision handling only.

Actions receive the COMPLETE final proposal, actual occurrence components and retained
selected capture. Their jobs must satisfy existing bundle field/count/group rules,
preserve occurrence order and use only actual selected ordinals. Unknown action or
missing required job semantics is unavailable; returning zero jobs cannot silently
discard an action obligation. Baseline synthetic actions below are non-deliverable.
Real action/provider/job adapters remain separate gates.

## D. Queue, calculation and staging algorithm

Every accepted request has one enqueue_work body, ONE context.begin() and one actual
session.commit() on its normal path. No outer "vote", nested begin, borrowed legacy
owning repository/adoption, automatic retry or separate clock/hold commit.
Use actual SqliteApplicationBundleStore and SqliteApplicationHoldStore on THIS session.

1. Check current execution access. Denied/unknown yields denied with no protected
   record, clock, arithmetic, actions or reference calls. Commit the read-only unit.
2. lookup(command.operation). Full selected storage closure first; then disclosure
   for actual historical source/result. If denied, disclose_denied without record.
   Decode known command/result/audit semantics. Unsupported healthy format yields
   not_ready(unsupported_retained_format), no payload/revaluation. Malformed known-v1
   or contradictory duplicates yield known semantic failure/whole rollback.
   Exact complete command returns existing_operation with unchanged old record.
   Changed command returns operation_conflict with no old payload or own audit.
3. lookup_original_claim(actual typed selector). Global tagged subject FIRST, then
   ORIGINAL receipt source. Authorize disclosure of a match. Return subject_claimed
   or source_claimed with its supported, fully checked historical record; it is not
   a retry of this new key and never reevaluates current amounts/conditions. Unknown
   retained format follows step2. Confirmation/source fallback may name a different
   actual historical receipt or subject; never reinterpret it as candidate admission.
4. holds.active(actual tagged subject). This audits ALL anchors including inactive
   and returns ALL active views or failure; no first-only or truncated-clear result.
   A nonempty set yields active_hold, with details only after disclose_holds.
   No current plan/preview/clock/action/token work bypasses this gate.
5. bundle.read_inputs(command.plan,subject_kind). missing_plan→missing_plan;
   stale_plan→plan_conflict WITHOUT actual head payload (this API returns none).
   Ready supplies complete selected capture, actual current state/head, current
   contribution/support and actual plan head. Check exact receipt/CID/source/role
   and current eligibility; compare actual plan head independently of state token.
   RequireExactPreview mismatch→state_conflict; revalidate_current records whether
   present preview differs, without replacing the requested plan revision.
6. Decode supported current state annotation, resolve bound policy and clock. Keep
   independent eligibility/pause/recovery blockers for FINAL admission. If policy
   or segmentation is unavailable, no invented observation/reducer call occurs.
   With known policy/segments, exclude Ready BEFORE advance_timer; it requires the
   start command. Advance Active/Finished through all segments, preserving ordered
   transitions and hard-end precedence even while paused, then decide final admission.
   A refusal commits no clock-only snapshot/head and publishes no proposed finish.
7. For admitted Active work calculate ALL ordered requests in memory. Use
   assess_contribution_eligibility on actual current state; empty blockers authorize
   only further planning. Preserve every current dimension and immutable support.
   Unavailable lifecycle/mixed/action policy yields the explicit gate below.
   No staging/action preparation/reference issuance precedes this complete calculation.
8. On full success prepare jobs and supported codecs, issue required fresh tokens,
   stage ONE PreparedApplicationBundle. intent.choices is the encoded original
   command; intent.expected_state_token is the ACTUAL persisted head; expected
   contribution is the COMPLETE current state, not captured support conditions.
   No hidden before/head commit. stage dispositions map as in sectionF.
9. A numeric overflow instead issues hold/detection/version refs and records ONE
   CapturedRevision numeric_overflow/defer_whole_input detection. Copy complete
   selected request evidence, exact known failing ordinal/target/attempt and original
   command; stage NO bundle, effect/job/claim/success result/contribution patch.
   Observation.persisted_state_token is the ACTUAL retained head, NEVER a fresh
   unstored advanced-before token. Proposed clock evidence belongs owned annotation.
   Creation expected_version is absent; no existing active hold is overwritten.
10. Required unavailable policy with known full input/source/head can similarly
    record unresolved_policy with specific missing_policy_reason, no numeric/timer
    fiction. Before full trustworthy input/reference closure, return not_ready and
    claim no durable hold. Resolution/continuation is outside this command.
11. Actual COMMIT, then return the owned response as the work's QVariant normally.
    Only the writer's committed/none/PRESENT correctly typed normal completion value
    can populate the public response and invoke the publisher. Synchronous commit
    return, captured local variables, old duplicate rows or staging cannot publish.

Known full-input blockers can yield ineligible without new hold because current
independent dimensions already retain them; this neither clears nor overwrites
them. Pause/recovery unresolved-policy handling does not authorize ACCT-005 acceptance.
Ready/Finished/unsupported mixed representation is not a fabricated cap-limited
award, unapplied component, complete success or automatic reopen.

The15 flattened current contribution fields checked independently by storage are
payment_evidence, monetary, original_decimal, currency, resolution_ref, missing_rate,
review, evidence_policy, policy_ref, decision_choice, decision_ref,
decision_actor_ref, application, reversal and settlement. Service copies the
COMPLETE ContributionState, including every nested optional/present-empty member;
no manual partial comparison or token coverage assumption. A confirmation may
promote current evidence without rewriting support. Stage separately rechecks
plan and all current fields.

For each positive/zero milliseconds request call CORE-005 plan_time_addition on
the CURRENT ordered proposal Main/debt and current cap. Pass ONLY main_added_ms to
CORE-002 apply_applicable_time_credit using the ORIGINAL clock-advanced TimerState
for that occurrence, BEFORE installing any numeric Main_after. Store debt_after
separately and require returned timer.Main equals numeric balance_after.Main.
This prevents double addition; debt repayment/request/capped amounts never rescue
Grace/wait. Actual0 preserves Grace/wait and independent pauses. Above-cap Main is
retained while debt can still repay; do not call CORE-001's narrow cap helper.

For an authorized negative original correction call plan_time_reversal with explicit
per-ordinal policy; never negate INT64_MIN. For Applied install actual Main/debt,
then one zero-elapsed advance_timer at the same final UTC/pause/config for ending
semantics; retain resulting transitions. RequiresReview retains the timer-specific
proposal but DOES NOT choose global status. Mixed composition has no enabled v1
policy: return not_ready(required_mixed_timer_policy), without inventing whole-input
numeric overflow or silently approving unrelated effects. Future owning policy must
define an atomic timer hold plus complete bundle OR explicit whole-input deferral.

For micro_points/whole_count, target(kind,raw key) MUST already exist. Missing target
is not_ready(required_counter_definition); no implicit zero or vector insertion.
Use Exact checked signed64 final addition in occurrence order; no caps/saturation.
Copy exact requested and residual exact-requested minus finalized unchanged. Check
ALL final balances/aggregate deltas before writing anything. A later overflow holds
the whole input, not its fitting prefix. Input requests already underwent final
half-even conversion upstream; do not rerun rules/rates or round actual cap credit.

For success, clock-only advanced-before preserves head debt, all counter keys/values,
configuration_ref and format_ref. Reuse head token ONLY if all typed fields and
entire owned envelope exactly match; otherwise a single unused advanced token.
After always has another unused token, including actual0. Construct annotations
without allocating a token before the entire outcome needs it. No rebind/ABA/loop.

## E. Versioned stored meanings

These formats describe storage annotations ONLY, not provider/wire/delivery or
downtime restore schemas. An outer known format's document must match its magic,
grammar and ALL duplicated authoritative typed fields. Healthy unsupported outer
format is explicit not-ready; malformed known format is a known semantic failure,
never current-policy inference. All documents are non-null, including empty DATA
inside B; these particular complete documents cannot themselves be empty.

Use grammarB; nested field abbreviations below are complete:

- Timer = U8(lifecycle Ready0/Active1/Finished2),U8(phase Main0/Grace1),
  I64(Main),I64(Grace),Bool(wait).
- Balance = U8(kind2/3),B(target),I64(units), strict kind/unsigned-byte-key order.
- Clock = B(lineage),B(observation),I64(observedUTC),V(I64(elapsed),I64(UTC),Bool(paused)).
- TimerConfig = U8(end Immediate0/Grace1/Confirmation2),I64(graceDuration),
  U8(reuse Fresh0/Shared1),O(I64(hardEnd)).
- Component = U8(kind0..5),I64(units),O(B(target)), exact existing role/target rules.
- Effect = I64(ordinal),U8(arithmetic),U8(unit),Exact(requested),I64(finalized),
  Exact(residual),O(I64(cap)),O(U8(deficitPolicy)),O(U8(deficitDisposition)),
  B(explanation),V(Component). Values/order/groups EXACT existing bundle semantics.
- Job = B(job_ref),O(I64(source_ordinal)),O(B(group)),O(I64(group_ordinal)),Env.
- Transitions = V(U8(core transition0..5 in declared header order)).
- Policy = B(config),B(policy_revision),B(permission_revision),O(I64(cap)),TimerConfig,
  Bool(paused),Bool(recovery_pending),B(pause_recovery_revision),B(explanation).
- Failure = U8(stage final_conversion0/aggregate_after_finalization1),
  O(I64(ordinal)),O(B(target)),O(Exact(attempt)).

| Outer format | Complete document after four ASCII magic bytes |
| --- | --- |
| subathon.internal.application-state.v1 / SAS1 | B(configuration_ref), O(B(basis_persisted_token)), Clock, B(policy_revision), Timer, I64(debt), V(Balance) |
| subathon.internal.application-result.v1 / SAR1 | B(encoded SAC1 document), B(persisted_head_token), B(advanced_before_token), B(after_token), Bool(revalidated), V(Effect), V(Job), Transitions |
| subathon.internal.application-audit.v1 / SAA1 | B(encoded SAC1 document), Policy, Clock, O(B(actor)),O(B(decision)),O(B(reason)), Plan, U8(outcome success0/numeric_overflow1/unresolved_policy2), O(Failure), O(B(missing_policy_reason)), B(calculation_explanation), B(action_explanation) |
| subathon.internal.application-job-synthetic.v1 / SAJ1 | I64(source_ordinal), B(action_ref), B(action_version), B(target), U8(unit), I64(requested), V(Component), B(synthetic_action_ref), B(payload) |

SAS1 configuration/timer/debt/balances must equal the enclosing snapshot. For newly
advanced/after snapshots its basis is the actual persisted input head, and its Clock
is the same bound command observation; after repeats it without advancing time.
Initialization/authoritative independent clock commands own valid prior annotations;
an initial annotation may have absent basis. A clock-only advance preserves format
and configuration and changes only its allowed timer/observation fields. An unchanged
annotation can be reused only for identical observation/binding/state. No SAS1
observation certifies a restart gap, producer policy or authority.

SAR1 command must equal intent.choices; identity/source/plan/preview/caller come from
that complete decoded command and agree with typed intent. Actual expected head
need not equal preview. Its three tokens, all effects/components/jobs/transitions,
revalidation and job occurrence order must match the full recorded bundle exactly.
Revalidated is true exactly when revalidate_current has a PRESENT preview token
different from the recorded persisted head; absent preview is false. The response
existing/new disposition is not stored here and never rewrites historical SAR1.

SAA1 command/source/plan/actor/decision/reason copies agree with typed audit/command;
policy/config/clock agree with successful snapshot observation/provenance, and
numeric/success outcomes have exactly the applicable failure/missing-reason groups.
A successful audit has neither Failure nor missing reason; numeric has Failure only;
unresolved_policy has missing reason only. Its policy values are retained explanations,
not reusable grants. Hold audit/first+attempt annotations use this same SAA1 with
actual known bound policy/clock; if those values are unavailable, use the separate
hold observation grammar immediately below rather than inventing known values.

subathon.internal.application-hold-observation.v1 uses magic SAH1 then
B(encoded SAC1), B(actual persisted head), O(Policy), O(Clock), O(Failure),
O(B(missing_policy_reason)), B(explanation). Every present duplicate agrees with
typed cause/observation/failure. Missing Policy/Clock stays absent; captured source/
plan/head remain actual known references. Detection.audit is SAH1 for non-success
holds, and its typed policy/permission/actor/decision/times copy ONLY known fields.
A failed arithmetic proposal is DATA inside annotation, not a retained snapshot.

SAJ1 is strictly synthetic and non-deliverable. It must match its referenced captured
occurrence and actual calculated components; job's source ordinal is required and
must equal its embedded ordinal. Its arbitrary payload may include BOM/NUL/FF/empty.
No provider worker may send this format. Real captured actions/job semantics need
their own reviewed formats/adapters/tests; service success in synthetic fixtures
does not verify delivery or authorize an unsupported production action.

Independent compound byte anchors use C0 above; they are grammar examples, not
a fabricated healthy database/source fixture. Clock l/t at UTC1000 with one
(0,1000,false) segment is39 bytes:

~~~text
000000016c 0000000174 00000000000003e8 00000001
0000000000000000 00000000000003e8 00
~~~

A clock-advanced SAS1 annotation with config c, basis a, that Clock, policy v,
Active/Main58/retainedGrace90/no-wait, debt0/no balances is90 bytes:

~~~text
53415331 0000000163 01 0000000161
000000016c 0000000174 00000000000003e8 00000001
0000000000000000 00000000000003e8 00
0000000176 01 00 000000000000003a 000000000000005a 00
0000000000000000 00000000
~~~

The CS01 one-effect encoding is88 bytes. Concatenate these independently explicit
fields: ordinal1; arithmetic1/unit0; Exact10/1; requested10; Exact0/1; present cap60;
absent deficit policy/disposition; empty explanation; three components:

~~~text
0000000000000001 01 00
000000023130 0000000131 000000000000000a
0000000130 0000000131 01 000000000000003c 00 00
00000000 00000003
00 0000000000000002 00
01 0000000000000000 00
05 0000000000000008 00
~~~

SAR1 with B(C0), tokens a/b/c, revalidated=false, that one Effect, no jobs/transitions
is182 bytes:4+(4+58)+3*(4+1)+1+(4+88)+4+4. It records Main+2/debt0/capped8
without inventing an after-state body in the result. SAA1 using C0, config c/policy v/
permission g/cap60/Immediate/graceDuration0/Fresh/no hardend/unpaused/no recovery/
pause revision q/empty explanation, the same Clock, absent actor/decision/reason,
plan p/1, success/no failure/no missing reason and empty two explanations is178
bytes. These byte counts include framing; the owning DTO budgets below do not.

## F. Results, failures and exact bounds

ApplicationServiceResponse owns disposition plus ONLY the indicated payload:

| Disposition (explicit value) | Payload |
| --- | --- |
| applied0 | complete RecordedApplicationBundle obtained through actual same-session lookup after staged; still provisional until completion |
| existing_operation1, subject_claimed2, source_claimed3 | authorized supported complete historical RecordedApplicationBundle |
| operation_conflict4, denied5, disclose_denied6, missing_plan7, plan_conflict8, state_conflict9 | no historical/current protected payload |
| ineligible10 | ordered unique ContributionBlocker values only, from actual current assessment, subject to execution access; no historical record |
| active_hold11 | optional complete ApplicationHoldActiveSet; absent when disclosure denied; never a truncated set |
| held_numeric12, held_policy13 | ApplicationHoldMutationOutcome with actual historical produced version(s), plus hold ref; no application record |
| not_ready14 | bounded explicit reason ref only, no success/hold claim |
| stale_contribution15, stale_state16, state_token_conflict17, subject_unavailable18 | no protected payload |
| stage_conflict19 | variant<SqliteBundleStageDisposition,SqliteHoldMutationDisposition> actual typed tag, no existing data |

Do not attach actual head to plan_conflict: neither stale read nor stage returns it.
state_conflict is the pure requested-preview mismatch; stale_state is later storage
CAS refusal. New staged success uses lookup to obtain fully checked historical
record, then verifies known codecs before commit. already_present and claim stage
outcomes repeat disclosure and supported-codec checks, compare complete original
command for same key, then map as early lookup. operation_conflict maps4; stale_state16;
stale_contribution15; stale_plan8; state_token_conflict17; subject_unavailable18.
Any reversal-only disposition is a service/source contract failure, not an original
success. Healthy remaining storage/hold identity conflicts map19, retaining exact
typed tag. Never auto-retry, clear a hold, allocate another key or discard the refusal.

ApplicationServiceInputError values are invalid_options0, invalid_key1,
invalid_source2, collection_limit3, invalid_body4, invalid_conditional_group5,
owned_byte_limit6. Failure source is bundle0, hold1, retained2, port_contract3,
retained_semantics4 or bridge5. For the first three it retains the exact existing
typed storage/retention reason; for port/semantics/bridge its reason is invalid_binding0,
invalid_output1, invalid_known_document2, contradictory_document3, result_limit4
or invalid_completion_payload5. WorkError/TransactionOutcome remain separate.
No unbounded exception text or protected document leaks through failure metadata.

ApplicationServiceCompletion contains actual TransactionOutcome transaction,
WorkError work_error, optional ApplicationServiceFailure failure and optional
ApplicationServiceResponse response. response is present EXACTLY when the real
SqliteWorkResult is committed/none with a PRESENT correctly typed normal value.
Other outcomes never synthesize response from captured data. failure records a
bounded category/source reason, not an alternate success channel. Unknown transaction
dominates every remembered local reason; preserve original WorkError. A confirmed
COMMIT followed by body exception is committed/work_exception with NO response.
Completion always receives the actual completion, including failure/unknown, so
the host can observe them. Downstream response/publication consumes only its present
authorized normal payload; authoritative new-state notification applies only to
applied, not a denial/duplicate/provisional finish. A publisher failure is a later
response failure and cannot
change accounting/completion history. Admission failure is separate.

Service-local malformed port/known-v1 semantic contradiction must establish real
whole-unit rollback before failure. Known store failures already did so: map their
exact source/reason without a second rollback. Original SqliteWorkFailure capability/
protocol/stale/terminal/unknown remains original; writer-established known query
busy/sql/resource/result retain their actual known rollback/reason. If active service
cleanup fails, actual unknown outranks the local failure. Never turn unknown into
rolled_back from a logical enum. Fault injection uses existing real writer mechanisms,
not a synthetic boolean pretending COMMIT failed.

Proposed ApplicationServiceLimits owns SqliteBundleLimits bundles{}, SqliteHoldLimits
holds{}, reference_bytes4096, choice_bytes262144, command_bytes262144,
storage_document_bytes262144, choices_per_command1024, clock_segments1024,
input_owned_bytes1048576, result_owned_bytes4194304, result_items65536.
All positive; checked representability leaves n+1 below qsizetype/qint64/size_t and
each relevant vector max_size. Validate ALL nested options including dormant holds/
bundle profiles first. Profiles independently cap their stores; no lifetime history
quota or reset of actual writer budgets. Service ceilings never weaken stored limits.
Encoded document bytes also obey the actual selected store's value_bytes; Exact
components obey its component_bytes. A valid command can still exceed a selected
store's application aggregate input budget: that is an explicit configured-bound
failure, never silent truncation or product input-count policy.

Pure input order: options → all required keys/source IDs/enums/presence → ALL counts
→ ALL bodies/documents → conditional purpose/preview groups → aggregate owned bytes.
No ticket, cleanup, queue mutation, port or feature call precedes successful preflight.
For encoding/decoding, command_bytes and all checked framing additions precede
allocation; encoded lengths must fit U32 independently of positive ceilings.
Unknown input format is unsupported after valid envelope/options/bounds, not a
fallback parser. Pure rejection preserves any caller unit elsewhere.

Logical service owned-byte accounting is separate from wire/framing length:
string/document actual bytes; uint64/int64 8; enum/bool1; optional NONE0/present value
only; vectors sum actual elements; no pointer/capacity/padding/framing charge.
Command bytes are the sum of ALL owned members in sectionA, counting each copy;
its choices additionally count both format and document. Completion response bytes
are disposition1 plus actual payload members; records/hold DTOs use their existing
published accounting unchanged, plus new owned wrapper members. Fixed disposition/
payload presence has no phantom vector item. Items are actual vector elements,
recursively, counting each owning copy: choices, blockers, segments, returned record
and hold containers by their respective published rules. Response not_ready adds
reason bytes; held adds hold-ref bytes AND its actual outcome; inactive absent
payloads add0. Checked overflow maps input owned_byte_limit before enqueue or known
result_limit/whole rollback after SQL, before allocating/returning the oversized copy.

For C0 logical command bytes: scope1+op1+subject1+receipt8+planref1+revision8+
principal-kind1+installation1+principal1+purpose1+previewmode1=25, zero items.
CID0 adds8→33; present-empty reason adds0 logical bytes but remains distinct in
canonical encoding. Choice x/six bytes adds7→32 and one item. Admission snapshots
each actual owned byte once; duplicated command documents in prepared envelopes
are EACH charged by the existing bundle/hold owning-input formulas. A result-only
new enum is charged1; absent optional payload0. Codec framing bounds are separate
and must not be substituted for those independent ownership oracles.

## G. Independent calculation/protocol examples and next gates

These are manual expected outcomes, not observed service tests or generated outputs.
The logical AS01–21 examples remain binding; this table fixes concrete baseline
boundaries for later independently frozen fixtures.

| ID | Required observation |
| --- | --- |
| CS01 | Main58/debt0/request10/cap60→Main60/debt0/components Main+2,debt0,capped8; CORE002 consumes original Main58 and actual2, never numeric Main60 plus2 |
| CS02 | Main55/debt2/request10/cap60→Main60/debt0/components +5,-2,+3. Above-cap Main100/debt5/request8/cap60→Main100/debt0/components0,-5,+3 |
| CS03 | Grace90 or waitMain0, debt5/request5/cap0→actualMain0; Grace/wait preserved. Request7/no cap→debt0/Main2; only actual2 rescues |
| CS04 | Mixed time+5 fits but scoreMAX+1 fails: only whole-input numeric hold; all source/capture remain, no success claim/jobs/after/head. Hold refers retained head a even if clock proposed Main100→90; no phantom token b |
| CS05 | Same command and key after committed record returns immutable old result without policy/clock/actions/issuer. Revoke execute→denied/no historic lookup. Grant execute but deny disclosure→disclose_denied/no payload |
| CS06 | New key/plan/confirmation for CID0 hits global subject claim before active/current policy; source fallback may identify another actual recorded receipt/subject. It never awards again or calls current arithmetic |
| CS07 | Empty own current hold set permits further checks; active h1/h2 under any old operation/plan blocks all new work. Oversized/corrupt active set fails, never appears clear |
| CS08 | Deadline1000 observedUTC1000 with pausedActive produces ordered hard-end/finish proposal BEFORE final pause admission. Refusal commits no clock mutation. Ready never reaches advance_timer |
| CS09 | RequireExactPreview stale→state_conflict; revalidate stale retains original preview a while physical expected head b. A changed current cap with matching token still resolves current policy; current rules never recapture old requests |
| CS10 | Alter ANY actual current contribution field after read or append a newer actual plan head before stage→explicit storage stale refusal, no output claim. Missing balance target→not_ready, no implicit zero |
| CS11 | Known B08 failure after staged effects but before full result/jobs→real whole rollback/no payload, prior intake durable. COMMIT-ack loss→unknown/no normal response, even for an old duplicate read |
| CS12 | COMMIT acknowledged then body throws→committed/no response. Normal committed completion then publication fails→record persists. Fresh service/reopened file returns authorized original record without effects |
| CS13 | Signed requestINT64_MIN/Main1/debt0/CarryTimeDebt→Main0/debtMAX. Same request/Main0/debt0 overflows→whole-input numeric hold. RequireReview remains timer-specific and cannot enable unresolved mixed/global policy |
| CS14 | Unknown healthy command/state/result outer format→not_ready, no fabricated corrupt record or current reinterpretation. Malformed known magic/trailing bytes/contradictory typed copy→known semantic rollback |
| CS15 | One issuer collision is explicit conflict; issuer called once for that purpose, no regeneration. Held/refused/duplicate work issues no success tokens. All retained response buffers survive service/session/writer destruction |

Next: different actual DOCUMENT review; then minimal header/unique stubs and build
harness review, with no executable behavior; independently authored literal codecs/
source/state/action fixtures and tests; full independent test review; selected
foundation green/registration guard; FIRST same-clean service/codec intended marker
red; only then ACCT-004 production. No old264 aggregate establishes that red.

Required future actual adapters include authentication/disclosure/current configuration/
policy/segmented clock/reference uniqueness and captured action/job meaning. Their
scripted synthetic fakes prove only ordering and storage transaction composition.
CORE-008/API-007 signed manual derivation/permission, ACCT-005 review/batch/silent/
continuation, ACCT-006 financial entitlements/refunds, milestones, post-finish mixed
policy, recovery/downtime/start/reopen/counter-definition commands and real provider
delivery remain mandatory owning-task gates. This document does not enable unknown
behavior or remove any of seven integrations/three FX providers/UI/platform/security/
backup/live/release obligations.
