# Reference application adapters — concrete consumer proposal v1

Task CONTRACT-014, revision 1; specification v0.45 §§2–4/6/10/17/18,
NUM-04, AUTH-06/07 and API-01/03. This proposes owned consumer interfaces and
stored formats for the reviewed [reference.v1 logical profile](reference-path-v1.md).
[ADR-0006](../decisions/0006-reference-path-prerequisites.md) retains all broader
release requirements. Actual acceptance lives in task/review evidence. No C++
header, schema, implementation, executable baseline or runtime capability is
provided by this document.

Import the existing [application service API](application-service-api-v1.md),
[receipt intake](receipt-intake-v1.md), [contribution linkage](contribution-eligibility-v1.md),
[captured plans](application-plans-v1.md), [SQLite plans](sqlite-application-plans-v1.md),
[retained context](retained-context-v1.md), [bundle](sqlite-application-bundle-v1.md),
[effective permissions API](effective-permissions-api-v1.md),
[timer lifecycle](timer-lifecycle-v1.md) and [time debt](time-debt-v1.md) unchanged.
This consumer contract cannot supply ACCESS-003's missing authority producer,
SESSION-003's missing authentication producer or recovery readiness by assertion.
Their actual contracts/implementations/reviews remain prerequisite gates below.

## 1. Admission and proposed owned types

Future namespace `subathon::server`, proposed header
`server/include/subathon/server/reference_application.hpp`. Every string is an
owned byte-exact opaque value; embedded NUL/high bytes are preserved. No string is
a login token, provider proof, authority certificate or connection handle. Existing
imported types retain their exact declarations and validation/accounting rules.
Enums below have underlying `std::uint8_t`, consecutive values from zero in the
listed order. Invalid casts are invalid input.

```cpp
enum class ReferenceFactsState : std::uint8_t { unknown, complete };
enum class ReferenceGate : std::uint8_t {
    not_ready, unsupported_configuration, denied, conflict
};
struct ReferenceUnavailable {
    ReferenceGate gate = ReferenceGate::not_ready;
    std::string reason_ref;
};
struct ReferenceRule {
    std::string rule_ref;
    std::string rule_version_ref;
    std::string action_ref;
    std::string action_version_ref;
    std::string timer_ref;
    std::int64_t add_ms = 0;
    ApplicationPrincipalBinding automation;
    std::string automation_capability_ref;
};
struct ReferenceConfiguration {
    std::string configuration_ref;
    std::string policy_revision_ref;
    core::ReceiptOrigin synthetic_origin;
    std::string receipt_namespace;
    ReferenceRule rule;
    std::optional<std::int64_t> maximum_main_ms;
    core::TimerConfig timer_config;
    // Each category must be explicitly complete and empty at admission.
    ReferenceFactsState other_rules = ReferenceFactsState::unknown;
    ReferenceFactsState modifiers = ReferenceFactsState::unknown;
    ReferenceFactsState milestones = ReferenceFactsState::unknown;
    ReferenceFactsState outbound_actions = ReferenceFactsState::unknown;
    ReferenceFactsState generated_events = ReferenceFactsState::unknown;
};
struct ReferenceSyntheticInput {
    std::string connection_ref;
    std::string event_id;
    std::optional<std::int64_t> source_utc_ms;
    QByteArray original_payload;
};
struct ReferenceArrivalCapture {
    std::string arrival_context_ref;
    std::string event_ref;
    std::int64_t server_arrival_utc_ms = 0;
    ReferenceConfiguration configuration;
    ReferenceSyntheticInput observation;
};
struct ReferenceExecutionBinding {
    std::string execution_context_ref;
    std::string arrival_context_ref;
    std::string installation_ref;
    core::ReceiptIdentity source;
    core::ReceiptId receipt_id{0};
    core::ContributionId contribution_id{0};
    ApplicationPlanKey plan;
    ReferenceRule rule;
};
struct ReferenceRuleEvaluation {
    core::RuleSelection selection;
    core::RequestedEffect request;
};
struct ReferenceAdapterLimits {
    std::uint64_t reference_bytes = 4096;
    std::uint64_t payload_bytes = 65536;
    std::uint64_t document_bytes = 262144;
    std::uint64_t value_owned_bytes = 1048576;
    std::uint64_t value_items = 4096;
    std::uint64_t clock_journal_entries = 64;
};

// Pure derivation from retained arrival configuration, never caller-supplied award.
ReferenceRuleEvaluation evaluate_reference_rule(
    const ReferenceArrivalCapture &, const ReferenceAdapterLimits & = {});
ApplicationEnvelope encode_reference_configuration(
    const ReferenceConfiguration &, const ReferenceAdapterLimits & = {});
ReferenceConfiguration decode_reference_configuration(
    const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
ApplicationEnvelope encode_reference_arrival(
    const ReferenceArrivalCapture &, const ReferenceAdapterLimits & = {});
ReferenceArrivalCapture decode_reference_arrival(
    const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
ApplicationEnvelope encode_reference_execution(
    const ReferenceExecutionBinding &, const ReferenceAdapterLimits & = {});
ReferenceExecutionBinding decode_reference_execution(
    const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
```

This is a proposed DATA interface, not a way to let clients construct authoritative
facts. A future strict configuration admission function must obtain complete facts
from the real versioned configuration owner; raw `complete` tags are not proof.
Each category above means its actual enabled set is known empty, not merely absent
from a partial load. The profile has exactly one enabled rule, one exact trigger
`synthetic.reference.support.v1`, one time-add action, unit milliseconds, zero
conditions/exclusions/cooldowns and no other actions. `ReferenceRule` cannot encode
these unsupported extras; the admission adapter must first inspect the complete
owning configuration and refuse them, rather than project them away into this DTO.
Unknown categories return not_ready; known nonempty categories return
unsupported_configuration. Full rules/modifiers/milestones remain their own tasks.

All required refs are nonempty and within reference_bytes. `add_ms` is a
nonnegative signed64 integer; negative values are invalid, zero is a real request.
Cap is absent or nonnegative signed64. TimerConfig must be a valid CORE-002 config
with absent hard_end; its grace/end/reuse values are retained, never rewritten to
reference defaults. The trusted synthetic origin has provider literal `synthetic.reference.v1`; its
source_account and receipt namespace are stable installation configuration. Connection_ref is a configured trusted test connector
binding; changing/reconnecting a transport never changes the source-account scope.
The connector supplies event_id, actual optional source UTC and original payload;
the queue supplies arrival time/current configuration/event/context refs. A connector
cannot choose arrival policy, current state, request amount or granted automation.
The trusted registration must bind the supplied connection_ref to this exact origin
and namespace. No transport route in CONTRACT-013 accepts these input types.

original_payload is a non-null bounded byte buffer; present empty is allowed.
Its bytes are retained as audit data, not parsed as a time amount or treated as
provider evidence. The fixed normalized receipt is event_type
`synthetic.reference.support.v1`, schema_version1, supplied source UTC, absent
actor ID/display/money/payment/activity references. Event identity is configured
origin + configured receipt namespace + event_id. ReceiptFacts equality is exactly
ACCT-001's existing equality. A payload change with unchanged normalized facts is
an additional arrival observation retained in its context, not a new contribution
or a substitute rule for normalized conflict detection. The reference adapter
claims no real provider original-payload consistency guarantee.

## 2. Canonical stored bytes and bounded operations

These internal annotations use application-service API grammar B unchanged:
U8 one byte; U32/U64 big endian; I64 the signed64 two's-complement big-endian bits;
B is U32 byte length plus exactly those bytes; O is U8 0/1 then a value only when
present. Bool is U8 0/1. Reject truncated frames, trailing bytes, unknown enum tags
and impossible optional groups. No JSON, Unicode transformation or numeric text
parser participates. All documents are non-null. Envelope format refs are exact:

| Value | Outer format / magic and complete payload in declared order |
| --- | --- |
| configuration | `subathon.reference.configuration.v1` / `SRC1`: B(configuration_ref),B(policy_revision_ref),Origin,B(receipt_namespace),Rule,O(I64(cap)),TimerConfig,U8(other_rules),U8(modifiers),U8(milestones),U8(outbound_actions),U8(generated_events) |
| arrival | `subathon.reference.arrival.v1` / `SRA1`: B(arrival_context_ref),B(event_ref),I64(server_arrival_utc_ms),B(encoded SRC1 document),B(connection_ref),B(event_id),O(I64(source_utc_ms)),B(original_payload) |
| execution binding | `subathon.reference.execution.v1` / `SRX1`: B(execution_context_ref),B(arrival_context_ref),B(installation_ref),Identity,U64(receipt_id),U64(contribution_id),B(plan_ref),U64(plan_revision),Rule |
| command choice | `subathon.reference.execution-ref.v1` / `SRF1`: B(execution_context_ref) |

Origin = B(provider),B(source_account); Identity = Origin,B(identifier_namespace),
B(opaque_id). Rule = B(rule_ref),B(rule_version_ref),B(action_ref),B(action_version_ref),
B(timer_ref),I64(add_ms),U8(principal kind),B(installation_ref),B(principal_ref),
B(automation_capability_ref). Principal kind MUST be automation1. TimerConfig
imports its exact application-service API grammar/CORE-002 validity. All fact tags
MUST be complete1 in an admitted configuration. An SRX1 receipt ID is1..UINT64_MAX;
contribution ID0..UINT64_MAX is valid; persisted plan revision1..UINT64_MAX.
No recursively nested SRC1/SRA1/SRX1 member exists beyond the single embedded
configuration in SRA1; command choices carry only the reference, not nested documents.
SRX1's rule equals the admitted retained arrival rule byte/value-for-value; it is
not today's rule or an authorized calculation override. Configuration and context
record keys must match their duplicated refs; SRA1/SRX1 link metadata must equal
the captured configuration, not merely some existing configuration.

Retain SRC1 as existing RetainedKind::configuration under configuration_ref, with
no configuration-link metadata. Retain SRA1 and SRX1 as existing
RetainedKind::arrival_context under their own refs, linked to the captured
configuration_ref. Their document bytes are the complete corresponding document,
without a second embedded envelope. The expected format follows from this exact
reference consumer role/key; do not infer meaning by sniffing a new format from
untrusted bytes. No RetainedKind/schema/migration or old document is changed.
Immutable exact retain returns already_present only for the same content/link;
conflict/missing_reference is explicit and cannot overwrite or alias a context.

Before deriving or encoding/decoding validate all positive limits and representability
in SIZE_MAX/PTRDIFF_MAX/qsizetype/qint64, with checked U32 framed lengths and relevant
container max_size. API error enum is proposed `ReferenceInputError` in order
`invalid_limits, input_limit, invalid_tag, invalid_reference, invalid_shape,
invalid_value, invalid_binding, unsupported_format, invalid_document, result_limit,
allocation_failure`; a fixed-message exception exposes only that code. These
codes describe this pure boundary; they do not replace actual SQL/writer errors.
For fully supplied DTOs priority is limits → borrowed complete per-field/B/I input
preflight → all tag checks → required refs → conditional shape → numeric/domain
values → cross-field bindings → complete result projection → allocation. A
malformed numeric/tag on an irrelevant field is not hidden by unsupported admission.
A decoder checks limits, outer format-ref/body limits and complete input B/I, then
unsupported outer format, then a borrowed no-allocation framing scan. Unreadable
frames yield invalid_document; no unreachable later field error is invented.
For a fully framed document, validate the decoded borrowed fields with the same
DTO priority, then project full decoded output B/I before allocation. A known magic
with malformed/trailing data is invalid_document, never healthy unsupported format.

The fixed-message exception is proposed `ReferenceInputFailure final :
std::exception`, with allocation-free `explicit ReferenceInputFailure(ReferenceInputError)
noexcept`, `code() const noexcept` and `what() const noexcept`. Its what text is
`"Reference application input failed"`; no arbitrary input or protected explanation
is copied into the failure. No pure failure mutates caller state.

B/I accounting for NEW DTOs is structural: string/QByteArray payload length counts
B; enums/bool count1; signed/unsigned64 count8; optional counts one presence byte
plus contained B/I; vector counts actual elements recursively in I. Structs sum all
declared fields/copies. For the one derived core::Exact, count canonical numerator
and denominator byte lengths; this integer request has denominator `1`, without
claiming that semantic bytes equal backend limb capacity. No capacities/terminators/allocator metadata are semantic B.
Every complete input/output/intermediate DTO independently fits
value_owned_bytes/value_items; every payload fits payload_bytes, document fits
document_bytes, reference fits reference_bytes. Choices inherit existing service
budgets separately. Check exact output framed length/B/I BEFORE allocating owned
output. A decoder scans the statically fixed field grammar, maximum64 borrowed index
records and nesting depth3; no unbounded recursion, eager DTO clone or count×size
overflow. Index payload is at most512 bytes plus the fixed-depth stack, not
misreported as result B/I. These NEW codec DTOs contain no vector, so their semantic
I is0; imported authority/service values retain their own actual vector counts. Integer scalar parsing uses exact
fixed-width bytes and never invokes floating-point conversion. Own
std::bad_alloc/std::length_error translate to allocation_failure without copying
input text into the exception. Caller-owned construction/copy and process death
remain outside this guarantee.

Limits are internal selected resource ceilings, not a lifetime contribution quota.
They do not enlarge imported store/service/permission/writer bounds. If separate
profiles disagree, the operation fails at the actual narrower boundary; it cannot
silently trim refs/history/output. On post-BEGIN local bad stored known semantics,
use ACCT-004's real whole-unit rollback/local failure mapping. Unknown durability
always preserves the actual writer outcome, never a locally inferred rollback.

## 3. Capture and durable intake choreography

Run only on the authoritative Qt queue with existing controlled writer and real
SqliteReceiptRepository, SqliteContributionRepository, SqliteApplicationPlanStore
and SqliteRetainedContextStore. Their actual legacy adopt/session constraints and
real commit rules remain unchanged. In units1/2 the host begins once, stages any
prerequisite retained data, then transfers that SAME active session to the existing
adopting repository; its successful service transaction owns the ONE real commit.
The host performs no second commit/cleanup after adoption. A remembered synchronous
service return remains provisional until normal actual writer completion.
One host sequences the following units; no
nested owner, connector write, secondary connection, raw-SQL bypass or port write:

1. Validate trusted synthetic registration and complete configuration admission.
   Sample actual arrival UTC independently of monotonic elapsed. Allocate bounded
   event/context refs using the fresh-reference strategy below, capture SRA1/SRC1
   and retain them with the receipt in the SAME real intake unit. Invoke existing
   ReceiptIntake with normalized ReceiptInput and session-adopting repository.
   Acknowledged normal completion establishes the receipt/attempt/context record;
   the new source's first attempt also retains the original configuration. An
   unknown outcome stops admission/reconciliation; do not proceed with a guessed ID.
2. On `conflict`, preserve its attempt and stop application; no new support/plan or
   authority arises. On created/retry, reread actual immutable original receipt and
   its ORIGINAL SRA1/SRC1 references. Retry never recaptures today's rule. This read
   and ContributionEvidenceService run in one subsequent actual unit. Submit support,
   evidence not_applicable, no VerifiedActivityLink and explicit support conditions:
   nonmonetary, no money/resolution/missing-rate, review not_required,
   require_evidence_review with retained synthetic policy_ref/no review decision,
   application not_applied, reversal none, settlement clear. evidence_admission.policy_ref is exactly the captured configuration_ref,
whose SRC1 reference semantics include this synthetic nonfinancial admission.
These facts come only
   from the trusted nonfinancial synthetic registration/config; real adapters cannot
   obtain them from absent source fields. ACCT-002 receipt deduplication yields the
   existing CID on retry; no activity/name/money/time resemblance is used.
3. Retain the first plan using the actual contribution/receipt snapshot and captured
   SRA1/SRC1 in one real plan unit. Construct PlanEvent from immutable original
   receipt, event_ref=root_event_ref, absent parent, actual receipt ID/CID and arrival.
   PlanValuation is not_applicable/no historical value. Evaluate the retained rule,
   call capture_application_plan with exactly that RuleSelection and explicitly
   empty known modifier result, then retain_initial. Initial revision is1 without
   parent/override. Persist SRX1 binding to the actual receipt/CID/plan1/rule and
   captured configuration in this same plan unit. A retry discovers the existing
   initial capture/binding; it never allocates a competing historical meaning.
   Exact already_present is reusable; conflict/stale/unsupported retained meaning
   stops. Durable source→initial-plan binding lookup must use a concrete bounded
   lookup owned by API-011; no scan or deterministic hash is assumed to prove uniqueness.
4. Only acknowledged normal completion of prior units permits submitting an
   OriginalApplicationCommand. Subject=contribution, actual receipt/CID/plan1,
   principal=the captured automation binding, purpose=captured_original,
   preview=revalidate_current, absent preview/actor/decision/reason, choices exactly
   one SRF1 envelope for the retained execution_context_ref. Operation scope/ref are
   fresh actual operation identity supplied by the host; choosing a new key does not
   change source identity or bypass existing claims. Call OriginalApplicationService,
   preserving its one real application unit and completion bridge entirely.

A source-index/lookup for initial plan/binding is **not present in today's retained
store API**. API-011 must first provide/review the narrow lookup/retention mechanism
with its necessary physical schema/migration and test-first gates, or prove an
existing indexed owner meets it. This document fixes uniqueness/semantics but does
not invent an existing method or authorize a migration. Until then capture orchestration
is not_ready; an in-memory map or unbounded retained-row scan cannot supply durable
retry identity. This is a concrete technical prerequisite, not an owner choice.

Pure evaluate_reference_rule returns one selected RuleSelection identifying captured
rule/version with fixed explanation `reference.single-rule.match.v1`. Its one
RequestedEffect copies action/action-version/rule/timer, unit milliseconds,
requires_historical_valuation=false, exact_requested_units=Exact(add_ms),
requested_units=add_ms, calculation_explanation=`reference.integer-ms.v1`,
reversal_behavior_ref=`reference.original-time.v1`. Direct configured integers
have residual0 and no fabricated money/rate/modifier/rounding steps. That reversal
ref records future ownership; it grants no refund or reversal capability. Derivation
must check bindings and admission before returning. Changing current r2 cannot
change this request or turn a retry into a recapture. RULES-006 implements this
narrow derivation; the complete rule editor/planner remains RULES-001–005/003.

## 4. Current authority consumer contract

Existing ExecutionAccess::check runs BEFORE protected operation/claim/hold lookup;
its command alone has no trustworthy amount. It first decodes the single SRF1,
loads SRX1 through the finite ApplicationReferenceReadView, and obtains captured
configuration/SRA1 meaning. All duplicated principal/installation/source/receipt/
CID/plan/rule/action/version/amount/target/capability refs must agree. These trusted
internal reads never disclose data to the caller. A missing/unavailable owner or
capability binding maps to ApplicationAccessUnknown; actual refusal maps denied.
A malformed known stored document follows existing known-semantic whole rollback,
not a convenient denial that hides corrupt state. No cached permit or caller bool.

Proposed consumer DATA and one synchronous read-only producer seam:

```cpp
enum class ReferenceAuthorityUse : std::uint8_t {
    execute_time_add, disclose_application, disclose_holds
};
struct ReferenceAuthorityRequest {
    ReferenceAuthorityUse use;
    ApplicationPrincipalBinding principal;
    ReferenceExecutionBinding captured;
    ApplicationOperationKey requested_operation;
    std::optional<ApplicationOperationKey> historical_operation;
};
struct ReferenceAuthorityFacts {
    ReferenceAuthorityRequest binding;
    std::string permission_revision_ref;
    std::string automation_revision_ref;
    core::EffectivePermissionInput permission_input;
};
using ReferenceAuthorityResolution = std::variant<
    ReferenceAuthorityFacts, ApplicationAccessDenied, ApplicationAccessUnknown>;
class ReferenceAuthoritySource {
  public:
    virtual ~ReferenceAuthoritySource() = default;
    virtual ReferenceAuthorityResolution resolve(
        const ApplicationReferenceReadView &, const ReferenceAuthorityRequest &) = 0;
};
```

The producer must load CURRENT authenticated automation ownership, explicit
capability authorization, owner-configured delegation bounds, actual installation,
rule/action/version/captured source scope and current permission/catalog/owner facts
from the same serialized authority. An automation capability being configured is
not a permit; rule editing/use permissions cannot manufacture its grant. Denied,
revoked, unknown, stale or unbound authority never produces a usable facts result.
A facts result is still raw DATA: the consumer verifies every binding and evaluates
its actual core::EffectivePermissionInput using the reviewed ACCESS-002 evaluator.
Only a permit plus all current bound provenance becomes ApplicationAccessAllowed
with the returned actual permission_revision_ref. No authority result is cached
across attempts or disclosure. Actual ACCESS-003 producer contract/storage and
implementation remain absent; this seam is its consumer requirement, not a
replacement authorization algorithm or invented completed ACCESS-003 capability.

The composite source resource has length16+len(provider)+len(source_account)+
len(identifier_namespace); require that full length within the actual selected
permission reference_bytes before constructing it. Three4096-byte source components
do not automatically fit a4096-byte composite bound. A caller may select a larger
validated permission profile; the adapter never silently truncates its source key.

The proposed narrow catalog mapping is exact; the real authority catalog must
explicitly support it, with no implicit alias to an unreviewed broad grant:

| Use | Operation / resource tuple / quantities |
| --- | --- |
| execute_time_add | `reference.time-add.v1`; resources `installation`,`timer`,`source`; quantity `adjustment`,unit `ms`,integer domain0..INT64_MAX, value captured add_ms/1 |
| disclose_application | `reference.application-read.v1`; same resource tuple, empty quantities; bind actual recorded source/plan/timer/principal rather than trusting today's candidate |
| disclose_holds | `reference.hold-read.v1`; same resource tuple, empty quantities; bind ALL requested actual hold subjects and immutable captures; any unresolvable/denied member yields false and no partial detail list |

Resource installation is captured installation_ref; timer is captured timer_ref;
source is the byte string `RSS1` followed by B(provider),B(source_account),
B(identifier_namespace). Event opaque ID remains in captured binding; different
events share a documented source scope but not a receipt/claim. No delimiter join,
case folding or origin-less event-ID comparison. Each permission operation is
ordinary class (no automatic protected-owner shortcut), with exactly those three
resource slots. Request values are the FULL configured request, even when actual
cap credit is0 or120000. The producer's catalog may impose stricter complete domains
only if that becomes explicitly unsupported/not_ready, never by clipping the
request. All granted tuples/denies/individual resource precedence follow ACCESS-002;
there is no union across amount/resource grants. Binding.permission_input.actor is
the ACTUAL current automation-account subject selected by ACCESS-003, not a raw
principal_ref assumed to be an account; the producer must establish that mapping.
permission_input.snapshot and catalog.revision are actual current revisions and
permission_revision_ref names the combined current resolution including
changed automation authorization. An empty/noncurrent revision is unavailable.

Disclosure loads/validates the historical record's own SRF1/SRX1/capture, asks the
producer anew for that actual record, and then checks that decision; prior execute
permit or a matching new-key candidate cannot expose another result. Actual hold
subjects require the same current per-subject disclosure logic. The missing durable
account/catalog/capability read producer cannot be replaced by arbitrary SQL from
these callbacks; if finite retained reads cannot supply it, a focused read-view
extension must be specified/reviewed under ACCESS-003/API-011 before implementation.
No callback opens a connection, writes, commits, queues nested work or calls network.

## 5. Actual adapters for the existing ACCT-004 ports

Implement the five existing interfaces unchanged; construction borrows queue-owned
current configuration, authority source, clock journal and bounded entropy provider.
Their owners outlive all prepared work and are drained before destruction. Every
callback uses const checked values and its finite read view. No port changes
application state or publishes. Actual adapters must prove the following:

| Existing port | Concrete reference obligation |
| --- | --- |
| ExecutionAccess | Section4; current full-request authorization before protected lookup, separate actual-record/whole-hold disclosure; unknown never allow |
| CurrentApplicationPolicy | Decode current head SAS1 and current SRC1 from inputs.state.configuration_ref, load retained captured SRX1/SRA1, require all source/plan/principal/captured request bindings, current supported configuration and current automation permission revision. Return exact BoundApplicationPolicy echoes; current cap/full TimerConfig/policy revision, paused=false, recovery_pending=false and actual known pause/recovery revision. Admission allow_active only for known supported state; correction_policies empty. It cannot load today's rule as a replacement captured request |
| SegmentedApplicationClock | Decode SAS1 observation ref/lineage and use the trusted queue-owned journal described below. Produce actual current observation/config/policy/head echoes, ordered nonnegative monotonic segments and independent sampled UTC. No UTC-difference elapsed or fabricated empty interval |
| RequiredApplicationActions | Recheck captured AND current supported configuration and calculated proposal. Prove no enabled milestone/outbound/generated action obligations and exactly the one captured time request. Only then return jobs={} with explanation `reference.no-external-actions.v1`; missing action knowledge returns unavailable, never empty jobs by default |
| FreshApplicationReferences | Issue ONE opaque ref per required exact binding/purpose, with no write/side effect to accounting; echo every binding. Collision/exhaustion/error refuses; never regenerate automatically or return a constant |

Current policy must separately establish Main-positive Active/Main phase, no
awaiting-confirmation, debt0, balances empty, no hard deadline/pause/recovery review.
The clock-advanced result must remain in this profile before credit. Crossing Main
expiry enters an unsupported policy path: no time add is silently applied as if it
were still Active/Main. Return not_ready before new intake where known; if an
already-retained candidate reaches an existing ACCT-004 required-unresolved-policy
path, preserve its truthful durable held_policy/whole-input hold outcome instead
of rewriting it to success. Earlier receipt/capture remain. A port returning malformed
binding causes actual rollback. Known complete unsupported policy is not corrupt
stored data and must not be thrown as a semantic failure. Existing eligibility and
active-hold gates run in ACCT-004's prescribed order regardless of profile checks.

Current/captured configuration may differ at execution: the current supported cap,
TimerConfig and policy revision apply; the retained rule request/source identity
never changes. If current config changes trigger/source/target/automation semantics
in a way the adapter cannot validate against captured authority, return unavailable;
never infer it from matching text. Explicit RP09 tests may change only current cap
under the same serialized admitted policy, not invent a reference mutation endpoint.

### Clock journal and state observation

A fresh disposable reference fixture is initialized through an explicitly reviewed
owner-only test/startup path with an actual SAS1 committed snapshot and matching
clock journal anchor. This contract supplies no public start, restore or editor
route. The anchor contains clock_lineage_ref, observation_ref, separately sampled
monotonic integer milliseconds and UTC milliseconds; journal records additionally
bind checked state/config/policy references. The journal is owner-thread-only,
read-only inside service callbacks and bounded by clock_journal_entries. Sampling
and assigning a new observation ref occurs before the service callback on the
queue, with no authoritative state/SQL mutation. The callback uses that reserved
observation; it does not resample time after permission/policy evaluation.

For this no-pause/no-deadline profile one segment covers monotonic_now-anchor;
require same live clock lineage and monotonic_now>=anchor, checked difference and
sum within signed64. Require elapsed<current Main so this admitted interval remains Active/Main; an
interval reaching/exceeding expiry is unavailable for this reference adapter, not
a silently skipped CORE-002 transition or permission to resume Grace. Current cap
may change under the serialized supported configuration; changes affecting timer
interval semantics without retained boundary evidence are unavailable. Its
utc_now_ms is independently sampled observed_utc_ms. An
identical anchor/UTC may use no segments; changed UTC with elapsed0 uses a zero
segment so the actual observation is retained. Never substitute current UTC for
missing stored metadata. CORE-002 advances through that actual segment BEFORE
CORE-005 cap-limited credit; ACCT-004 already owns this sequencing and atomic state.

Keep the anchor named by CURRENT committed head and every outstanding reserved
observation until its work completes; unused failed-work observations may be
released, never consumed as a new head. If capacity cannot preserve these entries,
refuse new work explicitly. After normal applied completion, the new committed SAS1
observation becomes the anchor; after known rollback retain the old head/anchor.
After unknown commit stop admission/reconciliation; choosing either anchor is not
safe. A post-COMMIT body failure has no normal success publication; resolve its
actual committed head independently before subsequent admission. The journal is
volatile lineage evidence, not a durable restart checkpoint. Restart, unavailable
anchor, discontinuous monotonic clock or unreviewed recovery means not_ready; raw
SQL reopen never manufactures a new zero-elapsed anchor.

### Fresh references and truthful publication

A proposed reference issuer obtains32 OS-random bytes from a bounded injected
entropy interface, then returns `RRF1` + U8(ApplicationReferencePurpose) +32 bytes,
37 bytes total. Clock observation refs use `RCO1`+32 bytes,36 total; initial event,
context/execution/plan refs use distinct fixed four-byte domains+32 bytes,36 each.
The actual OS entropy adapter/selected Qt or existing dependency binding needs
separate implementation/source tests/review; this document selects no new library
or token-storage/security shortcut. Failure or short entropy result is unavailable.
Each purpose is called once when the existing service needs it. Prefixes separate
purpose classes; actual stores enforce namespace non-reuse and return collision
conflict. Randomness alone does not certify uniqueness or durability. No fallback
to time, process counter, eventID hash or loop-until-unique. All bindings/refs obey
existing service/store ceilings; a profile with reference_bytes<37 cannot support
this issuer and fails admission rather than shortening the value.

Application success publishes only from actual ApplicationServiceCompletion with
transaction committed, work_error none and present normally returned typed response.
SAS1/SAR1/SAA1 remain exactly the existing formats; no new timer annotation or
second state commit. Request600000/cap120000 still records full request and actual
components separately. Actual0 still commits its source claim/result/audit and a
fresh after-token. A known rollback leaves intake/capture but no application effect,
claim/state/result/audit/job. Unknown suppresses normal output and stops admission;
no automatic replay under a new key. A body exception after acknowledged COMMIT
leaves durable accounting but no normal success. Later independently authorized
committed reads can resolve it; publication failure never reverses accounting.

## 6. Independent literal oracles and remaining gates

These are specification-derived expected values, not observed executable results.
Use real disposable SQLite and queue, explicit fake synthetic registration/clock/
authority facts, initial Active/Main3480000/debt0/Grace120000/no pause/deadline,
cap3600000, captured r1/add600000, observationUTC1000/elapsed0, head a. IDs a/b
are distinct opaque bytes; sequence names are explanatory, not numeric revisions.

| Oracle | Required observation |
| --- | --- |
| RP01 | Full grant600000; request600000/1, final600000, residual0; after Main3600000/debt0/Grace120000; components main+120000,debt0,capped480000. One source claim/effect/result/audit and0 jobs, new after-token b; committed snapshot only after normal completion |
| RP02 | Exact e1 retry creates another receipt attempt but uses original capture/r1; already committed application returns historical actual120000, no current arithmetic/clock/reference call and no new state |
| RP03 | Same e1/new operation key or concurrent queued duplicate: same CID/source claim wins; one total actual120000, no second award/new after-token |
| RP04 | Actual failure after staging before COMMIT establishes rollback: durable earlier receipt/plan remain, head a/Main3480000;0 claim/effect/new result/audit/jobs and0 success publications |
| RP05 | Authorized explicit retry AFTER RP04 known rollback rechecks current rights/cap/state and applies once; an unknown prior outcome does not permit this path |
| RP06 | Actual COMMIT applied but acknowledgement unknown: actual unknown error, no normal payload or guessed snapshot; stop admission, no new key/replay/anchor advancement |
| RP07 | COMMIT acknowledged then outer body throws: committed/work_exception, no response; later independently authorized actual read may resolve b once; no second award |
| RP08 | Revoke automation/required permission while queued: denied/no historic lookup. Grant only120000 for original600000: deny even though actual cap permits120000. Deny read after execute permit: no historical result |
| RP09 | Current supported cap3540000 before execution: same r1 request600000, actual60000/capped540000; persisted cap/config/observation coherent, no preview-derived120000 |
| RP10 | Main already3600000: actual0/capped600000, one committed application/new after-token; duplicate still0 and no second application |
| RP11 | Current r2/add900000 after r1 capture: e1 request remains600000/r1; a distinct new event using r2 captures900000; never overwrite original SRC1/SRA1/SRX1 |
| RP12 | Unknown category, enabled unsupported rule/modifier/milestone/action, pause/debt/Grace/deadline/recovery/missing clock anchor: not_ready/unsupported before fresh intake where known. Retained-input ACCT004 hold path remains truthful; no invented defaults or silently omitted feature |
| RP14-A | add_ms=-1 or unknown enum: invalid_value/invalid_tag before result allocation; INT64_MAX is valid configured request; no float/narrowing/absolute-value overflow |
| RP14-B | reference length4096 fits;4097 input_limit. Null payload invalid_shape; non-null empty fits. Payload65536 fits;65537 input_limit. Framed trailing byte invalid_document, unknown healthy outer format unsupported_format |
| RP14-C | Current Main1/no cap/requestINT64_MAX: signed64 aggregate overflows; existing service records whole-input numeric hold; original head remains, no partial +MAX-1 award/success claim/jobs. The request/capture stays exact |
| RP14-D | Issuer repeats an occupied state token once: explicit actual conflict, no regenerate. Complete result above budget: real failure/rollback, no partial payload; unknown cleanup remains unknown |
| RC01 | elapsed30000 before credit: Main3480000→3450000, then request600000/current cap3600000 gives actual150000/capped450000, after3600000. UTC difference alone is not elapsed |
| RC02 | add_ms0: exact0/1, final0/residual0; source applied once, after-token still new and actual0; no fabricated cancellation/negative request |

RP13 HTTP negotiation/authentication/disclosure framing belongs CONTRACT-013;
this consumer supplies no HTTP session mechanism or alias. Future tests independently
freeze source mappings, binary frames/B/I boundary counts, all new error stages,
borrowed lifetime/owned result behavior, interrupted capture units, indexed durable
source→plan binding, current authority/clock/no-actions production adapters and each
RP/RC oracle. Compilation failure or a fake commit is not an intended red baseline.

Necessary unresolved technical producer gates at authoring:

- ACCESS-003's actual automation-account/delegation/current revision/explicit
  capability producer and finite persistence reads; SESSION-003 for HTTP identity.
  Consumer mapping above is concrete, but raw capability/principal refs prove no grant.
- API-011's narrow durable unique source→initial-plan/execution-binding lookup and
  actual current configuration/pause/recovery facts, plus any reviewed read-view
  extension. Existing generic retained API cannot be claimed to provide that index.
- Real queue clock sampling/journal, entropy uniqueness/collision behavior and safe
  initial SAS1 fixture/bootstrap; no periodic/restart/recovery support is supplied.
- Actual different CONTRACT014 document review, then RULESTEST006/APITEST011
  reviewed executable tests/valid red → RULES006/API011 implementation/separate
  source review/current CI; API001/session/access and CONTRACT013/API010 plus
  APITEST008/API008 real HTTP/SQLite path acceptance remain required.

These are explicit implementation/contract integration gates, not owner product
questions. No existing port/header/schema is silently extended and no new behavior
is marked supported by this proposal. All complete rules, financial review/refunds,
modifiers/milestones, all seven integrations, all three FX providers, both renderers,
locales/themes/platforms, packaging/licenses/backup/security/live release scope remains.
