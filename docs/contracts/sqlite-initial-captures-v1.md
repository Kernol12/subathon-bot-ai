# SQLite initial reference capture — concrete proposal v1

STORAGETEST-018 / STORAGE-018, first DOCUMENT phase. Authority: specification
v0.45 §§3/5/17/18, DB-01/API-02/NUM-04 and the accepted
[CONTRACT014 consumer](reference-application-adapters-v1.md), blob
`5a12ebea` (SRI1 and its durable first-capture section). This refines that
consumer without changing its identities, meaning, entrypoints or transaction
boundary. [Physical companion](sqlite-initial-captures-schema-v1.md) proposes
one additive table after immutable1–6. No C++ header/source, schema source,
fixture, test, native operation or runtime capability is supplied here.
Different actual document/API/physical review precedes later stages.

## 1. Responsibility, missing prerequisites and ownership

Persist/discover one immutable FIRST capture per installation and COMPLETE
ReceiptIdentity. Its complete record points to the source's original receipt,
original support/CID, captured configuration/arrival/event, initial plan revision1
and execution binding. A later rule, retry, plan override, application effect or
new operation key cannot replace that meaning. This index does not admit a
configuration, evaluate a rule, grant automation, accept a contribution, apply
time, disclose protected data or implement recovery.

Actual source inspection finds no `reference_application.hpp`,
ReferenceCaptureKey/ReferenceInitialCapture declaration or SRC1/SRA1/SRX1/SRI1
codec. CONTRACT014 is an accepted consumer DOCUMENT, not an implemented supplier.
Making these codecs depend on API011 would create a dependency cycle. Therefore
propose the following MINIMAL supplier ownership within018's baseline/implementation:

| Future path | Exact responsibility |
| --- | --- |
| server/include/subathon/server/reference_capture_values.hpp | Shared fixed DTOs, limits/errors and eight pure SRC1/SRA1/SRX1/SRI1 encode/decode declarations below |
| server/src/reference_capture_values.cpp | Those bounded structural codecs only, following reviewed tests/red; no admission/evaluation/authority/SQL |
| server/include/subathon/server/sqlite_reference_capture.hpp | Frozen consumer store/key/result/error declarations, plus pure manifest declaration |
| server/src/sqlite_reference_capture.cpp | Caller-session index and narrow selected-closure validation only |

The future reference_application.hpp IMPORTS the shared declarations in the SAME
`subathon::server` namespace; it never duplicates or substitutes them. The
CONTRACT014 aggregate members/defaults/order, operation names, formats and wire
bytes remain identical. No old header is modified by this document. Root may
split the codec baseline/implementation if needed for executable task size; actual
codec acceptance must precede SQL closure execution, and neither supplies RULES006
or API011 behavior. This is an explicit implementation prerequisite, not an
unanswered product choice or a fabricated existing API.

Shared declarations are EXACT CONTRACT014's ReferenceFactsState, ReferenceRule,
ReferenceConfiguration, ReferenceSyntheticInput, ReferenceArrivalCapture,
ReferenceExecutionBinding, ReferenceAdapterLimits, ReferenceInputError and
ReferenceInputFailure, together with ReferenceCaptureKey/ReferenceInitialCapture.
Import actual ApplicationPrincipalBinding from application_service.hpp and
ApplicationPlanKey/ApplicationEnvelope from sqlite_application_bundle.hpp; do not
redeclare their ABI. No new account/source/session/read-view port is introduced.
The codec functions are exactly:

```cpp
ApplicationEnvelope encode_reference_configuration(const ReferenceConfiguration &, const ReferenceAdapterLimits & = {});
ReferenceConfiguration decode_reference_configuration(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
ApplicationEnvelope encode_reference_arrival(const ReferenceArrivalCapture &, const ReferenceAdapterLimits & = {});
ReferenceArrivalCapture decode_reference_arrival(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
ApplicationEnvelope encode_reference_execution(const ReferenceExecutionBinding &, const ReferenceAdapterLimits & = {});
ReferenceExecutionBinding decode_reference_execution(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
ApplicationEnvelope encode_reference_initial_capture(const ReferenceInitialCapture &, const ReferenceAdapterLimits & = {});
ReferenceInitialCapture decode_reference_initial_capture(const ApplicationEnvelope &, const ReferenceAdapterLimits & = {});
```

Their role is structural validation/owned canonical bytes. No codec performs
admit_reference_configuration, evaluate_reference_rule, current access, actual
clock loading or a fresh-reference factory. A complete tag in decoded DATA is
not evidence that an authority producer loaded complete facts.

## 2. Concrete store API and pure priority

Use the exact consumer types; these are proposed declarations, not a header:

```cpp
struct ReferenceCaptureKey {
    std::string installation_ref;
    core::ReceiptIdentity source;
};
struct ReferenceInitialCapture {
    ReferenceCaptureKey key;
    core::ReceiptId receipt_id{0};
    core::ContributionId contribution_id{0};
    std::string configuration_ref;
    std::string arrival_context_ref;
    std::string event_ref;
    ApplicationPlanKey plan;
    std::string execution_context_ref;
};
struct ReferenceCaptureLimits {
    qsizetype reference_bytes{4096};
    qsizetype document_bytes{262144};
    qsizetype owned_bytes{1048576};
};
enum class ReferenceCaptureRetainDisposition : std::uint8_t {
    inserted, already_present, conflict, missing_reference, source_mismatch
};
enum class ReferenceCaptureStorageReason : std::uint8_t {
    busy, sql_failure, resource_limit, result_limit, invalid_stored_data
};
class ReferenceCaptureStorageFailure final : public std::runtime_error {
  public:
    explicit ReferenceCaptureStorageFailure(ReferenceCaptureStorageReason);
    ReferenceCaptureStorageReason reason() const noexcept;
};
class SqliteReferenceCaptureStore final {
  public:
    explicit SqliteReferenceCaptureStore(SqliteWorkSession, ReferenceCaptureLimits = {});
    std::optional<ReferenceInitialCapture> find(const ReferenceCaptureKey &) const;
    ReferenceCaptureRetainDisposition retain_initial(const ReferenceInitialCapture &) const;
  private:
    SqliteWorkSession session_;
    ReferenceCaptureLimits limits_;
};
MigrationManifest reference_capture_repository_manifest();
```

Store construction/copy/destruction is inert and does no validation/SQL/cleanup.
Methods are repeatable const operations on the SAME active revocable capability;
retaining/copying a facade neither adopts ownership nor extends owner/thread/work
lifetime. Its first actual execute performs the real writer capability check;
there is no private phase probe, fake query, secondary connection or event loop.
Valid foreign/expired/terminal calls preserve actual wrong_thread/stale_session/
session_closed semantics. No construction-thread guard overrides the true ticket.

All three store limits must be positive, strictly below qsizetypeMAX, qint64MAX,
size_tMAX and relevant std::string/QByteArray max_size, reserving n+1 BEFORE any
addition/narrowing. Validate all fields even for absent results. Failure is
ReferenceInputFailure(invalid_limits), fixed allocation-free text
`Reference application input failed`, using the unchanged CONTRACT014 enum order.
Then borrowed complete key/candidate B and each reference ceiling are checked;
byte overflow/overage is input_limit. No extra owned copy/SQL/cleanup occurs.
Then all ten candidate references (five in a key) must be nonempty, otherwise
invalid_reference. ReceiptId0, plan revision0 or any revision other than1 is
invalid_value. CID0..UINT64_MAX is valid, never absence. No provider spelling,
Unicode, identifier coercion, sign or timestamp restriction is added to this key.
These pure failures precede stale/foreign tickets and do not roll back earlier
caller work. Required descriptor meaning is validated against actual closure later.

Key B is the sum of its five reference lengths. Record B is the sum of its ten
reference lengths plus24 (receipt/CID/revision8 each); I=0. Find's absent result
has no record bytes/items; a present result has exactly record B/I, without echoing
a second key or an invented optional-byte charge. Each selected/intermediate
closure DTO separately fits owned_bytes before ownership; that is not a sum of
all historical attempts or a lifetime quota. Document scratch separately fits
document_bytes and each reference reference_bytes. Larger valid stored data
under an explicitly larger profile remains possible; no truncation/default or
silent smaller bound substitutes for it. SQL cumulative budgets remain independent.

## 3. Canonical structural codec supplier

Import CONTRACT014 §2 grammar EXACTLY: magic ASCII4, U8, big-endian U32/U64,
signed64 two's-complement I64, B=U32 byte count+payload, O=0/1+optional value.
No JSON/locale/Unicode repair, floating conversion or hash-normalized identity.
All documents are non-null; no trailing bytes/alternative magic/group tags.

| DTO | Exact format / payload following its magic |
| --- | --- |
| ReferenceConfiguration | subathon.reference.configuration.v1 / SRC1: B(configuration_ref),B(policy_revision_ref),Origin,B(receipt_namespace),Rule,O(I64(cap)),TimerConfig,U8(other_rules),U8(modifiers),U8(milestones),U8(outbound_actions),U8(generated_events) |
| ReferenceArrivalCapture | subathon.reference.arrival.v1 / SRA1: B(arrival_context_ref),B(event_ref),I64(server_arrival_utc_ms),B(SRC1 document),B(connection_ref),B(event_id),O(I64(source_utc_ms)),B(original_payload) |
| ReferenceExecutionBinding | subathon.reference.execution.v1 / SRX1: B(execution_context_ref),B(arrival_context_ref),B(installation_ref),Identity,U64(receipt_id),U64(contribution_id),B(plan_ref),U64(plan_revision),Rule |
| ReferenceInitialCapture | subathon.reference.initial-capture.v1 / SRI1: B(installation_ref),Identity,U64(receipt_id),U64(contribution_id),B(configuration_ref),B(arrival_context_ref),B(event_ref),B(plan_ref),U64(plan_revision),B(execution_context_ref) |

Origin=B(provider),B(source_account). Identity=Origin,B(identifier_namespace),
B(opaque_id). Rule=B(rule_ref),B(rule_version_ref),B(action_ref),B(action_version_ref),
B(timer_ref),I64(add_ms),U8(principal kind),B(installation_ref),B(principal_ref),
B(automation_capability_ref). TimerConfig=end_mode U8,grace_duration I64,reuse U8,
O(I64(hard_end)), preserving existing known enums/nonnull optional meaning.
Principal must be automation1. Known account0 in this required automation role is
invalid_binding; an unknown principal enum is invalid_tag. Fact enums accept only
unknown0/complete1 as raw typed DATA;018 closure requires all five complete1 and
supported synthetic/no-deadline meaning, independently of codec success. This
refines error spelling without making decoding an admission certificate.

All ReferenceAdapterLimits defaults/fields remain as declared in CONTRACT014,
including positive value_items/clock_journal_entries even though fixed codec DTOs
have I0/no clock journal. Validate positive representability before actual framed
lengths/conversions, with checked U32 limits for EACH B payload and n+1 reservation
for owned destinations. No arbitrary global document<=U32MAX restriction is
introduced when only its individual fields need U32 framing; embedded SRC1's B
length must itself fit U32. Every actual input/output/intermediate independently
fits value_owned_bytes/value_items; refs/payload/documents use their field bounds.
The unchanged six uint64 limits, in order, are reference_bytes4096,
payload_bytes65536, document_bytes262144, value_owned_bytes1048576,
value_items4096 and clock_journal_entries64. ReferenceInputError's unchanged
uint8 order is invalid_limits, input_limit, invalid_tag, invalid_reference,
invalid_shape, invalid_value, invalid_binding, unsupported_format,
invalid_document, result_limit, allocation_failure. ReferenceInputFailure derives
std::exception with explicit noexcept construction from that code, code() noexcept
and what() noexcept; its fixed message above never allocates/copies input.
All integer/enum/domain checks and cross-field consistency follow CONTRACT014
priority. SRI1 uses the same reference/Receipt/CID/revision1 rules above. SRC1/SRA1
required fields/negative add/cap/Grace and invalid TimerConfig are rejected; SRA1
arrival/config refs and embedded configuration remain exact. SRX1 requires its
installation equal rule.automation.installation_ref, positive receipt, present
CID including0 and positive plan revision1..UINT64_MAX, as CONTRACT014 specifies.
The selected initial index closure separately requires that SRX1 revision equal
SRI1's1. No codec claims the external receipt/plan/configuration actually exists.

Encoding validates limits→borrowed complete per-field/B/I→all enum tags→required
refs→conditional shapes→numeric values→cross-bindings→complete framed output B/I
and capacity projection BEFORE allocation. Decoder validates options→complete
outer envelope bounds/B/I→unsupported outer format→borrowed framing scan. Truncated
or trailing/magic-invalid frames are invalid_document; fully framed values then
use the same tag/ref/shape/value/binding priority, followed by output projection.
Do not invent an error in an unreachable field of a truncated document. Fixed
scan uses at most64 borrowed index records, at most512 payload bytes, nesting3
(SRA1→SRC1→fixed nested rule/config); no variable DTO tree is owned first. Codec
bad_alloc/length_error maps to ReferenceInputFailure(allocation_failure), with
no SQL or mutation. Format bytes count in complete encoded-envelope B in addition
to its document; they are not hidden in document length.

Proposed later minimal markers (not source here): each of the eight codec bodies
immediately throws ReferenceCaptureValuesNotImplemented with fixed text
`Reference capture values are intentionally unimplemented; review tests first`.
Manifest/find/retain each immediately throws SqliteReferenceCaptureNotImplemented,
text `SQLite initial captures are intentionally unimplemented; review tests first`.
Only inert declarations/constructors/errors are released after actual document
review; no validator/manifest SQL or successful codec precedes a marker.

## 4. Selected SQL, first binding and complete closure

Runtime SQL is fixed adapter-private trusted code through actual
SqliteWorkSession::execute, explicitly main-qualified and positionally bound.
No SQL string/handle or generic reader is exposed to API011. Receipt/contribution
owning repository wrappers cannot be borrowed here: their adoption transfers
commit/cleanup, and receipt find loads growing attempts. Bundle read_inputs also
selects mutable head/current policy not needed here. Do not use those shortcuts.
Private typed original/support/revision1 projections are necessary and are tested
through this facade; existing selected decoder helpers may be factored/reused only
after actual source review, without importing their broader current/full-history
scope or creating a new public bypass. No old port/API changes are required.

Find selects at most2 matching rows by the COMPLETE indexed five-part key. None
returns nullopt without reading candidate receipts/configs/orphan plans elsewhere.
Two matches are selected corruption, never first/last selection. A found row is
strictly decoded with actual SQL typeof/length/null/domain checks, complete row B
before materialization, and ALL selected closure below. No return of a partly
filled record. This is a selected read, not a global audit of off-key malformed
rows, receipt attempts, unindexed orphans or unselected plan history.

Retain performs source-key lookup AND `(installation_ref,receipt_id)` unique-key
lookup with bounded2-row selection. Validate source match closure first, then any
distinct receipt match closure, BEFORE candidate equality/refusal or NEW candidate
reference resolution. Both lookups must identify the same complete stored record
for already_present. All thirteen fields equal→already_present; any different
candidate or competing receipt binding→conflict. Missing proposed plan/context
cannot mask existing conflict; corrupt selected original cannot hide behind it.
No own write/cleanup occurs for these healthy dispositions.

Only when both indexed bindings are absent resolve the candidate's actual selected
closure, in this order. For any present root, validate its own selected structure/
mandatory targets before comparing the candidate. A direct healthy absent candidate
target is missing_reference; a PRESENT stored row's dangling intrinsic target is
invalid_stored_data. Healthy well-typed references that describe another source/
configuration/CID/plan/execution mean source_mismatch. Once an existing index selects
the closure, ANY missing/dangling/mismatched meaning is invalid_stored_data instead.
Well-typed selected materialization overages remain result_limit in either path.

| Selection | Exact immutable facts and equality obligations |
| --- | --- |
| Actual receipt by positive BLOB8 ID | Original four-part identity/facts/arrival only, not attempts. Identity=key.source. Fixed synthetic event_type/schema1, source UTC exactly SRA1 observation, absent actor/display/money/payment/activity fields. Original cfg/context equal record cfg/arrival. Connection and arrival UTC match SRA1. Earlier actual receipt commit remains host precondition; same-session SELECT does not prove it |
| Actual contribution by CID including0 | Select ID/original_receipt_id and immutable activity group, not mutable current conditions/payment/application. Original receipt equals record.receipt_id; reference synthetic activity identity is absent. Select its ORIGINAL support observation at that same receipt, kind support, association to this exact CID, evidence not_applicable, absent activity/proof and exact captured synthetic support conditions described below |
| Retained SRC1/SRA1/SRX1 | Exact typed configuration/context namespaces/keys/documents. SRC1 has no cfg link; SRA1 and SRX1 have present link exactly record.configuration_ref. Decode fixed format by known role, not sniffed format. Embedded SRA1 SRC1 equals actual retained SRC1 byte/value-for-value. Arrival/event/connection/event ID/source/account and original facts all agree; SRX1 duplicates every record ID/ref, captured rule and installation, never today's rule |
| Selected initial plan | Header's immutable event/root/parent/receipt/CID and revision1 only. revision1 has no parent/override, event=root=record.event_ref, parent absent, original receipt projection and present CID exact. Captured state equals original support, valuation not_applicable/no quote. Exactly one selected rule ordinal1 and one request ordinal1, no modifiers; rule/version and request fields equal SRC1 rule as below. Current plan head, other revision bodies, claims/effects/current state are NOT selected/audited |

Synthetic support conditions are nonmonetary, no original money/resolution,
missing_rate=false, review not_required, evidence_policy require_evidence_review,
policy_ref=captured configuration_ref, no review decision, application not_applied,
reversal none, settlement clear. These are immutable captured conditions, never
requirements that today's mutable contribution dimensions still equal them.
Plan rule selected=true/explanation=`reference.single-rule.match.v1`; one request
has the captured action/action-version/rule/timer refs, milliseconds, no historical
valuation, exact canonical add_ms/1 and requested_units=add_ms, explanation
`reference.integer-ms.v1`, reversal ref=`reference.original-time.v1`. This is strict
stored binding comparison, not invocation/copy of the rule evaluator or a grant.
All five category tags complete1, provider synthetic.reference.v1, admitted absent
hard deadline and SRX/SRC rule automation installation=key.installation_ref are
required for the indexed meaning. Current permission/delegation is never inferred.

Closure fixed-cardinality preflight observes actual SQL class/group/count and
length metadata BEFORE payload/child ownership. Each selected root/payload DTO's
complete semantic B must fit owned_bytes; no eager full plan find/callback result
is built to discover its size. Counts for the selected r1 child collections are
1/0/1, with actual contiguous ordinals; decode the complete selected rows only.
No application bundle reader, full growing plan-history/receipt-attempt quota,
unbounded scan or process cache is needed. All cumulative writer SQL/binding/row/
byte budgets include the index and closure reads/writes; exhaustion is an actual
safe refusal, not reason to reset budgets, split units or hide work.

## 5. Atomicity, errors and publication

After successful closure, insert the complete thirteen-column row ONCE. No upsert,
update/delete, index rebinding, generated domain ID, BEGIN/adopt/COMMIT/retry or
secondary writer exists in this facade. Exact source/install+receipt uniqueness
is SQL-backed with full BLOB keys, not a hash proof. Conflict/missing_reference/
source_mismatch/already_present add no own rows and leave healthy caller work active.
Caller composes plan1+SRX1+index in ONE actual unit; after attempted staging any
non-inserted refusal or exception requires its whole cleanup before reporting
capture refusal. It must not commit competing/orphan staged plan/context data.
An exact already-present reuse publishes only its already validated original
closure after this work's own acknowledged normal completion.

Known busy/sql_failure/resource_limit/result_limit and selected invalid_stored_data
map to ReferenceCaptureStorageFailure ONLY after actual whole-session rollback.
what() is exactly `Reference initial capture did not commit`; only reason() exposes
the bounded category. Already rolled-back writer/retention failures keep their
recorded truth; a second inactive cleanup call is not proof of a new rollback.
Capability/invalid-statement/protocol/unmapped WorkFailure remains original.
Actual uncertain cleanup/commit outranks every local reason, fail-closes admission
and propagates SqliteWorkFailure(uncertain_outcome). No stored record turns THIS
unit's unknown outcome into success. The store never maps caller commit errors as
its own errors. A caught allocation/length failure after its own writes must clean
the whole unit before propagation; uncertainty overrides it. Do not relabel ordinary
bad_alloc as resource_limit or claim a known storage reason without evidence.
Pure codecs separately provide their specified allocation-free input-failure map.
The host must clean any partially staged capture attempt even when failure occurred
before this index wrote; owner finalization remains the backstop, not a substitute
for caught-exception hygiene.

Find/retain returns are provisional even when they read earlier staged plan/SRX/
index from this same session. Only actual transaction=committed,error=none and
normal owned completion permits a usable binding. A known rollback preserves the
prior committed intake/contribution units but removes new plan/SRX/index/markers.
Commit-unknown never authorizes fresh refs/replay. Confirmed commit followed by a
body exception preserves durable rows but no normal result. No declaration claims
power-loss recovery, operational readiness, provider confirmation or live authority.

## 6. Independent literal examples for future author/reviewer

These are document oracles, not authored/executed tests. Main fixture A is an
explicit synthetic source: key installation I, origin(synthetic.reference.v1,A),
namespace N,event E; Receipt7,CID0,cfg C,arrival X,event V,plan(P,1),execution Y.
Use retained full SRC1/SRA1/SRX1 above with one immutable rule R/r/action a/b/timer T,
add_ms600000, automation(I,U), capability K, known-empty categories and no deadline.
SRA1 event V/UTC0/connection Q/event E/source UTC absent/present-empty payload.
Receipt original fixed synthetic facts/schema1/UTC absent, arrival0/Q/C/X; support
and initial plan use exactly the captured conditions/request above. Old receipt
and contribution allocator metadata must independently match actual fixture rows;
CID0 is real, never SQLNULL. Do not manufacture malformed allocator metadata merely
to obtain a tiny fixture. These records prove no real provider or accepted authority.

| ID | Operation/fault | Independent required oracle |
| --- | --- | --- |
| IC18-01 | Pure hypothetical ten one-byte refs; Receipt7/CID0/revision1 | DTO B34/I0; SRI1 document78 bytes; format37 bytes, complete encoded envelope B115. Encode owned bound115 fits/114 gives result_limit; document bound78 fits/77 refuses before allocation. Record-only34 does NOT fit the encoded-envelope budget. This hypothetical key is NOT fixture A or admitted synthetic closure |
| IC18-02 | A canonical key and record | Key B26/I0; record B55/I0; SRI1 document99/envelope B136. CID0 stays eight zero bytes, Receipt7 ends07; receipt/CID MAX stays eight ff with identical lengths |
| IC18-03 | Find absent source while unrelated/unindexed orphans exist | nullopt, no global closure scan/readiness/effect inference; healthy caller marker remains active |
| IC18-04 | Stage A plan1/SRX/index then find in same active unit | inserted plus complete owned provisional A; outside reader absent until acknowledged commit; rollback removes ALL three, earlier intake/support remain |
| IC18-05 | Commit A normally/reopen; exact retain A | already_present; one immutable row and original P/1/Y even after healthy P history/application effects advance |
| IC18-06 | Existing A, same key but proposed P2/Y2 or nonexistent new cfg | conflict BEFORE proposed new target resolution; A unchanged; no own cleanup, host rolls back any competing staged P2/Y2 |
| IC18-07 | Existing install+Receipt7 binding, candidate another source key | conflict after full existing closure; no alias/index overwrite. Alter one exact source component for a DIFFERENT actual receipt and full matching closure instead remains a distinct independent key |
| IC18-08 | No existing binding; direct missing receipt/CID/config/SRA/SRX/initial plan | missing_reference with no own insert; valid references naming different source/CID/event/rule →source_mismatch; host removes attempted staged orphans |
| IC18-09 | Existing selected A with missing SRX/plan/receipt/support, wrong cfg link/ID/canonical body/ordinal | invalid_stored_data after actual WHOLE rollback, never nullopt/conflict replacement; inside-catch independent BEGIN proves released lock |
| IC18-10 | A current contribution review/evidence/application changes; plan has later versions | Historical first binding/support/plan1 unchanged; no current-state/plan-HEAD authority; selected original support/plan1 corruption still refuses |
| IC18-11 | Candidate Receipt0/revision0/revision2/empty ref, oversized field, invalid limits on expired/foreign ticket | invalid_limits→input_limit→invalid_reference→invalid_value before ticket/SQL/cleanup; valid candidate retains exact writer capability error; CID0/MAX valid |
| IC18-12 | Well-typed selected ref/document/complete DTO at bound and one over; malformed known-size field | Exact fits; one over→result_limit before ownership and known whole rollback; fitting malformed type/frame/domain→invalid_stored_data. No truncation or unread semantic claim |
| IC18-13 | Two queued candidate units for same key, second uses fresh refs | Single writer serializes; first normal commit wins, second finds exact original or conflict; no second committed plan/SRX orphan or guessed equal hash |
| IC18-14 | Fault after index insert/before commit, or refusal after plan/SRX staging | Known rollback removes entire plan/SRX/index and caller marker; later explicit healthy attempt recomputes/finds; do not retry an unknown outcome |
| IC18-15 | Lost acknowledgement after physical COMMIT, or cleanup acknowledgement loss | Actual unknown/fail-close/no normal binding/no replay, not invented known rollback; post-COMMIT body throw is committed/work_exception/no normal binding |
| IC18-16 | Consume cumulative SQL/result/binding budget on second otherwise fitting operation | Whole existing unit rolls back with actual mapped known reason or unknown; no per-method reset or fabricated result |
| IC18-17 | Mutate/destroy caller DTO and facade after return | Returned record owns every ref/ID; destructor no SQL/commit/cleanup; copied ticket does not prolong owner/work |
| IC18-18 | SRI1/SRC1/SRA1/SRX1 wrong format/magic/truncation/trailing/unknown tag/overflow lengths/null QByteArray | Complete outer bounds precede format/framing; borrowed framing before decoded ownership; no readiness success, repair or SQL from codec |

Actual independent document/API/schema review precedes minimal interfaces/stubs;
then literal fixtures/old-primitive diagnostic/full test review and successful
compile precede a new intended marker red. STORAGE018 waits for integrated baseline,
implementation/code/CI review and real passing selected Qt SQL acceptance. Old SQL
or rule-component passes never count as018 red. All UI/client/authority/rules/
provider/FX/recovery/platform/package/live release scope remains with existing tasks.
