# Exact retained valuation API and storage — revision 1

Task: [RATECONTRACT-002](../tasks/EPIC-017/RATECONTRACT-002.md).
Authority: specification v0.45 §§3.5, 9, NUM-04, FX-03/06, and the reviewed
[logical bridge](exact-retained-valuations-v1.md). This refines that bridge; its
19 examples and owned/opaque provenance classification remain requirements.
This document is an API/format/target-schema contract, not authored headers,
migration bytes, executable tests, intended red or implemented capability.

## 1. Audit and migration prerequisite

Audit base: main `38b482d` (PR117). Preserve existing legacy defaults and all
shipped assertions. The affected surfaces are:

| Surface | Concrete obligation |
| --- | --- |
| `core/application_plan.hpp/.cpp` | Add the exact alternative below; preserve legacy capture/revise/history behavior and existing signatures |
| `core/rate_snapshot.hpp/.cpp` | Reuse existing pure input/query/factories without changing their formats or limits; reconstruct from owned values as in §3 |
| `ContributionValuation` and contribution SQL | Keep monetary/original-money/resolution/missing-rate fields; they are a decision projection, not a snapshot store |
| `sqlite_application_plans.cpp` | Read/write the tagged valuation in the same retained revision and preserve all old scalar/child/source checks |
| `sqlite_application_bundle_access.hpp` | Update independent scalar, anchor, metadata, selected-body and selected/full closure paths; a plan-store-only implementation is insufficient |
| `sqlite_application_bundle_values.hpp` | Charge every new owned field/snapshot, including repeated actual copies, in plan input/result metrics |
| `application_service.cpp` and command codec | Service consumes already captured requests through `ApplicationBundleInputs`; it has no current rate producer. Preserve access-before-protected-lookup, duplicate/hold/source checks and numeric request/effect distinction |
| `sqlite_reference_capture.cpp`, `reference_capture_values.cpp` | Preserve no-money initial path and SRC1/SRA1/SRX1/SRI1 bytes; update physical plan inspection to recognize new tags and refuse monetary exact plans in this path |
| Physical app/hold/reference consumers | `subathon_app_operations` and hold causes reference plan revisions; app effects reference plan requests; reference initial captures reference revisions. Keep those table names, key meanings and real FKs valid |

Migration4's `subathon_plan_revisions` CHECK requires a complete legacy decimal
group for `valuation_state=2`. Adding an exact side table while filling that group
with fake decimals, or recording an exact resolved value as missing, is forbidden.
Parallel revision tables also fail to satisfy existing app/hold/reference FKs.
The target therefore keeps the existing table/key names and rebuilds this ONE
parent table in a new migration, preserving every old row value and child/FK.
This is additive versioned representation, not permission to edit migration4.

**Explicit prerequisite:** [STORAGECONTRACT-003](../tasks/EPIC-009/STORAGECONTRACT-003.md)
must freeze the narrow controlled parent-table rebuild mechanism, exact final DDL,
full catalog projection/framing and version/ID allocation, writer admission,
foreign-key checking, backup/update authority and failure/cleanup sequence.
[STORAGETEST-021](../tasks/EPIC-009/STORAGETEST-021.md) and
[STORAGE-021](../tasks/EPIC-009/STORAGE-021.md) must separately review, observe
real behavioral red and implement that prerequisite before RATESTEST-009 uses it.
All three remain separate planned work. This document neither grants arbitrary
PRAGMA access nor assumes a CREATE/DROP/rename sequence is safe with foreign keys
ON. The existing controlled writer/catalog has no certified populated-parent
rebuild capability. Do not bypass it or add a second authoritative writer.

The new manifest must copy the complete current compiled prefix1–8, including
account/session step8, byte-for-byte. The next free version at implementation
integration is allocated by STORAGECONTRACT-003; do not reuse version4 or assume
plan-prefix4 is the current database. Earlier factories, SQL, step IDs, digests,
expected per-step schemas, starting-version and row fixtures remain frozen.

## 2. Core API and plan alternative

Future header: `core/include/subathon/core/exact_valuation.hpp`, namespace
`subathon::core`, C++20/selected Boost/standard library only. `string`, `optional`,
`variant` below are their `std` types. DTO construction is inert. Result values
own their data and expose only `const Data &data() const noexcept`, with defaulted
copy/move and no public default/data constructor, like the rate-model wrappers.
All feature input references are synchronous borrowed input; no reference survives.

```cpp
struct IdentityValuationBasis {};
struct SnapshotValuationInput { EffectiveRateSnapshotInput snapshot; };
struct ManualValuationBasis {
    std::string entered_from_currency, entered_to_currency, original_decimal;
    std::string decision_ref, actor_ref, reason;
    std::optional<std::int64_t> recorded_at_utc_ms;
};
using ExactValuationBasisInput = std::variant<IdentityValuationBasis,
    SnapshotValuationInput, ManualValuationBasis>;
struct ExactRetainedValuationInput {
    std::string resolution_ref;
    ReceiptMoney original_money;
    std::string comparison_currency, effective_numerator, effective_denominator;
    ExactValuationBasisInput basis;
};
struct SnapshotValuationBasis { EffectiveRateSnapshot snapshot; };
using ExactValuationBasis = std::variant<IdentityValuationBasis,
    SnapshotValuationBasis, ManualValuationBasis>;
struct ExactRetainedValuationData {
    std::string resolution_ref;
    ReceiptMoney original_money;
    std::string comparison_currency, effective_numerator, effective_denominator;
    ExactValuationBasis basis;
};
struct ExactValuationDocument { std::string format_ref, document; };
```

`ExactRetainedValuation` is the immutable wrapper for
`ExactRetainedValuationData`. The canonical numerator/denominator are the effective
rate; they do not duplicate an owned `Exact`. Arithmetic helpers parse them under
their fixed bounds. Snapshot pair key is exactly
`(basis.snapshot.data().snapshot_ref, original_money.currency, comparison_currency)`;
no independent editable copy can contradict it. It is retained in the complete
value through those fields, and the durable snapshot key below has the same bytes.

```cpp
[[nodiscard]] ExactRetainedValuation make_exact_retained_valuation(
    const ExactRetainedValuationInput &input);
[[nodiscard]] ExactValuationDocument encode_exact_retained_valuation(
    const ExactRetainedValuation &value);
[[nodiscard]] ExactRetainedValuation decode_exact_retained_valuation(
    const ExactValuationDocument &document);
[[nodiscard]] ExactValuationDocument encode_valuation_snapshot(
    const EffectiveRateSnapshot &snapshot);
[[nodiscard]] EffectiveRateSnapshot decode_valuation_snapshot(
    const ExactValuationDocument &document);
```

These APIs use the fixed local profile in §4. A caller's lower store/service/writer
limits remain independent. No caller option can enlarge the bridge profile.
Add `std::optional<ExactRetainedValuation> exact` **after** the existing
`PlanValuation.historical`; default absence preserves all existing aggregate
initializers and legacy behavior. `PlanValuationState` remains 0/1/2. Resolved
requires exactly ONE of legacy historical or exact. Both other states require
neither. Missing-historical still forbids valuation-dependent requests.

Exact capture consistency additionally requires actual event money, byte-exact
equality of its two strings with exact.original_money, and a monetary captured
ContributionValuation with the same original money, `missing_rate=false` and a
present resolution_ref equal to exact.resolution_ref. This checks the captured
projection, not a future contribution's mutable current resolution. Legacy
captures retain their existing weaker structural validation; do not reinterpret
opaque old rates or newly reject old stored projections. A new manual resolution
must append a normally authorized audited plan revision. Factory DTO identities
never supply actual permission. RATES-008/ACCT-005/API-002 retain real producer,
review/continuation and authority gates.

Core `capture_application_plan` and `revise_application_plan` retain their existing
signatures. All new exact inputs must be bounded before the existing capture or
history copy: at most1024 revisions and4096 total rule/modifier/request elements,
whole returned exact-containing plan logical bytes ≤1MiB. These are new
materialization bounds for exact-containing core results, not lifetime storage
quotas and not retroactive limits on unchanged legacy-only helper calls. The
storage append operation still validates old structural history in bounded pages
without full-plan materialization.

## 3. Validation, reconstruction and authority

Positive integer components use `[1-9][0-9]*`, ≤2467 bytes EACH and ≤8192 bits EACH,
with gcd1. Reject noncanonical stored values; do not normalize corrupt bytes.
Money uses `make_rate_money`/the existing 512-byte,256-digit, exponent256 parser;
negative money and signed-zero source spelling remain data. Currencies are
nonempty opaque bytes ≤64, not automatically supported ISO currencies.

For identity, currencies must be byte-equal and N/D must be1/1. No snapshot/manual
fields exist. For manual, currencies differ; required actor/decision/reason are
nonempty. Entered pair must be precisely the original/comparison pair in either
direction, entered decimal must be positive, and its parsed value or inverse must
be exactly N/D. No table override kind, provider observation or cross path is
created. Equal-currency manual inputs are invalid even when rate1/1.

For snapshot, currencies differ and the full selected snapshot is owned.
Reconstruction from `EffectiveRateSnapshot::data()` creates one bounded
`EffectiveRateSnapshotInput`: copy scalar identity/parent/construction time;
copy every observation context and quote order with each quote's
`rate.original()`; copy every override and its original spelling/kind/optional
actor/time; copy constraints in order; serialize each component member's positive
canonical Exact numerator/denominator in canonical model order. No current lookup,
provider call or raw-evidence target is substituted. First preflight that complete
projection, then reconstruct, then call existing `quote_effective_rate(input,
from,to,profile)` which independently validates the complete input on every call.
Require `available` and exact N/D equality. The decoded owned snapshot is obtained
by `make_effective_rate_snapshot(input,profile)` and must agree field-for-field,
including original spellings, optional presence and ordered model content.

Missing pair is a separate producer outcome, not a valid resolved value. If lookup
returns missing during construction of a purported resolved capture, report
`missing_pair`; a contradictory retained available fraction is `inconsistent_rate`.
Identity is never substituted for a missing foreign pair. Lower admission refusal
is a resource failure, not missing historical evidence.

The owned closure is exactly all selected snapshot observation/quote/time,
override/actor/reason, constraint, component and order data; snapshot key/content
must match the typed shared row. Manual owns all entered fields. Provider,
configuration, request, source-evidence, override decision and optional parent
snapshot identities remain exact opaque identity bytes: no external target lookup,
ancestry walk or provider-evidence verification is promised. Provider configuration
identities do not target application retained configurations. Existing plan/event
configuration, receipt, contribution and arrival links retain their existing typed
closure checks. Future RATES-005/007 typed external closure needs its own reviewed
ports/store dependencies before admission; this bridge does not require fictional
producer availability or claim those references were verified.

## 4. Finite ownership and admission profile

Logical B/I use the rate API metric: string bytes; signed/unsigned64=8 bytes;
enum/variant tag=1; optional absent0/present child; Exact=canonical component
strings; ExactDecimal=original spelling plus Exact; vector contributes child B
and length+recursive child I. Structs sum actual fields. No C++ padding, capacity,
shared-target deduplication or phantom framing bytes enter this metric. Actual
encoded document bytes have an independent length bound. Repeated actual owned
copies count each time in the value that contains them.

The snapshot profile is the existing `RateSnapshotLimits` with: observations16,
quotes_per_observation128,total_quotes256,overrides64,constraints256,currencies64,
items2048,owned_bytes24576; existing currency64/reference256/time512/reason4096
and4096-bit potential ceilings stay unchanged. These are lower bridge admission
limits, not changes to RATES-001's 8MiB model. Complete raw AND parsed snapshot
must separately fit. Canonical snapshot document ≤32768 bytes. Exact valuation
resolution/decision/actor refs ≤256, manual reason ≤4096 and money/entered decimal
≤512; complete raw AND output valuation B≤65536,I≤4096 and full document≤65536.
The logical001 16MiB valuation ceiling is an outer maximum; this concrete profile
refuses earlier explicitly.

| Owning operation/value | Bound before allocation/copy |
| --- | --- |
| Factory borrowed preflight | Inspect all counts, field lengths and checked recursive totals before any body copy; raw snapshot/valuation B/I as above |
| Reconstruction/semantic validation | Each complete temporary input/output obeys its own snapshot/valuation bound. At most one reconstruction, one parsed snapshot and one exact output coexist; combined constructed scratch B≤262144,I≤8192, excluding caller input |
| Snapshot/valuation public encode | Preflight input and exact full framed output length; output Envelope B counts actual format_ref+document, ≤65792; no output allocation before size pass |
| Public decode | Borrowed format_ref≤256/document≤65536 (snapshot≤32768); full framing/count/length scan first. Owned result under the value bound above; input document is not copied just to parse it |
| SQL retain/append input | Existing SqlitePlanLimits remain; exact group≤65536 and snapshot≤32768 ALSO fit actual per-field/statement/session budgets. Aggregate exact-containing candidate B≤1MiB,I≤8192 before owning staging data |
| SQL selected revision | Complete owned selected capture including hydrated snapshot B≤1MiB,I≤8192; existing selected vector/field bounds remain. No truncated projection or key-only snapshot masquerading as owned result |
| SQL full find containing exact values | Complete returned ApplicationPlan B≤1MiB,I≤8192 plus existing1024 revisions/4096 materialized old vector items. Charge each hydrated copy, even a shared snapshot used by many revisions; unchanged legacy-only results keep existing limits |
| Bundle/hold/original-claim selected inputs/results | Existing bundle owned_bytes1MiB/input12288/result24576 and hold profiles continue; charge complete hydrated exact value in actual plan copies. Selected source overage is result_limit with whole rollback |
| Application service | Command input1MiB, stored document262144, intermediate/result4MiB stay unchanged; new complete intermediate plan/snapshot copies are included before construction. No rate data is added to an otherwise reference-only command |

Each operation also consumes the actual cumulative writer budgets. Default SQL
bound/result bytes are1MiB, result rows1024 and statements64. A single32KiB snapshot
is not an8MiB blob; neither it nor65KiB valuation promises arbitrary counts fit
one transaction. Precompute known binding/result projections and refuse safely;
count checks/streaming pages never reset cumulative work. Existing profile values
can be lowered by their trusted callers and can refuse an otherwise valid bridge.
Do not widen old fields, schema limits or service settings to make examples pass.

Scratch indexing is ≤8192 fixed-size borrowed-reference/index records, not copied
whole models or speculative matrices. Byte/count arithmetic uses checked size_t
and destination representability before addition/reserve. Incremental positive
integer parsing checks bit growth before multiply/add; gcd operates only within
bounded components. Arithmetic intermediates obey16384 bits and use cancellation
before multiplication; normalized final requested units must fit CORE-001's4096-bit
components before half-even rounding/int64 range checking. Allocation failure is
reported distinctly; these logical quotas do not guarantee allocator/CPU success.
No public unlimited-history read or all-snapshots materialization API is added.

## 5. Complete canonical byte grammars

All formats are internal. `U8/U32/U64` are unsigned big-endian with fixed1/4/8
bytes; signed64 `I64` is the two's-complement bit pattern in U64. `B(x)` is
U32(byte length)+exact bytes. `O(X)` is U8(0) for absent or U8(1)+X for present;
other tags invalid. `V(X)` is U32(count)+ordered X. No NUL terminator, padding,
locale, inferred default, trailing extension or ignored byte is allowed. Empty
present optional strings differ from absent. All count/length arithmetic and
complete framing are validated borrowed before allocation, then complete count
bounds precede body semantic validation. `B` is allowed opaque binary; only the
named numeric fields use ASCII grammar. Resulting snapshots/valuations are
semantically revalidated, not merely deserialized.

`subathon.internal.valuation-snapshot.v1` document begins literal `RSV1`:

```text
RSV1 B(snapshot_ref) O(B(parent_snapshot_ref)) O(I64(constructed_at_utc_ms))
 V(Observation) V(Override) V(Constraint) V(Component)
Observation = Context V(Quote)
Context = B(observation_ref) B(provider_ref) B(configuration_ref)
 B(request_ref) B(source_evidence_ref) O(B(provider_snapshot_ref))
 O(I64(fetched_at_utc_ms)) TimeEvidence TimeEvidence
TimeEvidence = O(B(original_text)) O(I64(utc_ms))
Quote = B(from_currency) B(to_currency) B(original_decimal)
Override = B(override_ref) U8(kind) B(from_currency) B(to_currency)
 B(original_decimal) B(decision_ref) O(B(actor_ref)) B(reason)
 O(I64(recorded_at_utc_ms))
Constraint = U8(0) B(observation_ref) U64(quote_ordinal)
          | U8(1) B(override_ref)
Component = V(Member)
Member = B(currency) B(canonical_positive_numerator) B(canonical_positive_denominator)
```

TimeEvidence order is source-rate then publication. Override kind temporary0 or
pinned1. Quote ordinals are existing model index+1, never re-sorted/re-numbered
independently. Components/members/constraints follow the canonical admitted model
order; reject stored out-of-order representations, do not silently sort them.
Potential components obey4096-bit/1234-byte limits. No parsed quote Exact is stored:
its source spelling reconstructs it exactly. There is no omitted provenance field.

`subathon.internal.exact-valuation.v1` document is the full transport-free owned
value, literal `ERV1`:

```text
ERV1 B(resolution_ref) B(original_money.original_decimal)
 B(original_money.currency) B(comparison_currency)
 B(effective_numerator) B(effective_denominator) Basis
Basis = U8(0)                                      # identity
      | U8(1) B(complete RSV1 document)             # snapshot_pair
      | U8(2) Manual                               # manual_valuation
Manual = B(entered_from_currency) B(entered_to_currency) B(original_decimal)
 B(decision_ref) B(actor_ref) B(reason) O(I64(recorded_at_utc_ms))
```

The internal retained form `subathon.internal.exact-valuation-key.v1`, literal
`EVK1`, has the identical common fields/identity/manual alternatives; its snapshot
alternative is U8(1)+B(snapshot_ref), not an embedded snapshot. This form is **not**
a public owned valuation. Its selected typed row must be resolved in the same
caller session and hydrated before any value is returned/accepted. Unknown key
or wrong namespace cannot fall back to current tables. The server's storage
implementation exposes no public API that returns a key-only value as resolved.

Public server helper signatures for isolated typed codec tests are:

```cpp
struct RetainedExactValuationDocument {
    ExactValuationDocument value; // EVK1 format, owned
    std::optional<ExactValuationDocument> snapshot; // RSV1 iff basis snapshot
};
[[nodiscard]] RetainedExactValuationDocument encode_retained_exact_valuation(
    const core::ExactRetainedValuation &value);
[[nodiscard]] core::ExactRetainedValuation decode_retained_exact_valuation(
    const RetainedExactValuationDocument &document);
```

They live in `subathon::server`, use core document type, have no SQL/authority or
lookup. The snapshot document carries its own snapshot identity which must match
the EVK1 key. Missing supplied snapshot yields missing_target; unexpected supplied
snapshot for identity/manual or conflicting identity/content is rejected. Combined
codec input/output B≤131072,I≤8192; individual bounds above still apply.

Dispatch first checks valid fixed bounds/outer format identity. Unknown healthy
outer format returns unsupported_format; known outer format with wrong magic,
truncation, unknown basis/enum, malformed integer or trailing bytes is malformed
known data. No old v1 body receives a suffix. SAC1/SAA1 encode plan keys rather
than valuation bodies, and SAR1/SAH2 retain results/failures, so their bytes/tags
remain unchanged. Adding an exact stored plan does not authorize any new semantic
choice envelope. Unknown actual service format still follows existing not_ready.

## 6. Target schema, immutable storage and read scopes

No shipped table is modified by this contract commit. The eventual new migration
has this fully specified target representation; STORAGECONTRACT-003 must freeze
its exact DDL bytes/catalog projection and safe execution recipe before tests.

Add table `subathon_valuation_snapshots` with columns, in order:
`snapshot_ref BLOB NOT NULL PRIMARY KEY` (typeof blob,length>0),
`format_ref BLOB NOT NULL` (typeof blob,length>0),
`document BLOB NOT NULL` (typeof blob). No rowid alias, uniqueness beyond key,
trigger, allocator or mutable-current pointer. Format is the RSV1 outer identity
above. Snapshot identity/content is immutable: identical canonical format+document
reuses the row, different bytes under one key yields identity_conflict without
writes. The complete encoded bytes, including unknown optional metadata, are the
comparison; hashing alone cannot replace content comparison.

Rebuild `subathon_plan_revisions` retaining its existing34 columns in their exact
order and existing declared types/nullability, all original PK/FK/key/audit,
contribution and scalar CHECKs, except replace the old whole valuation-group CHECK
with the tag dispatch below. Append these FOUR columns in order:

| Column | Definition/meaning |
| --- | --- |
| valuation_format | INTEGER NOT NULL DEFAULT0; typeof integer AND IN(0,1) |
| exact_format_ref | nullable BLOB; new EVK1 format identity only when format1 |
| exact_document | nullable BLOB; canonical complete EVK1 bytes only when format1 |
| exact_snapshot_ref | nullable BLOB FK REFERENCES subathon_valuation_snapshots(snapshot_ref); exact key iff snapshot basis |

Format0 requires all new three BLOB columns SQLNULL and exactly the old whole
valuation-group CHECK, preserving all old null/present-empty possibilities.
Format1 requires valuation_state2, all ELEVEN old historical-group columns
SQLNULL (historical_resolution_ref through manual_decision_ref), typeof BLOB and
nonempty exact_format_ref, typeof BLOB exact_document, and exact_snapshot_ref
SQLNULL or nonempty BLOB. Runtime bounded decoding requires EVK1 identity and
exact_snapshot_ref presence/bytes to agree with its basis/key. No new cap is
imposed on legacy BLOBs by DDL. No decimal placeholder, fake provider snapshot,
REAL value or newly inferred timestamp is permitted. Legacy nonvaluation
`resolution_ref` in ContributionValuation remains present independently.

Old34-column rows copy byte-for-byte with valuation_format0 and new fields NULL;
new exact writers use format1 and all old historical fields NULL. The row's
existing parent and child FK identities remain the same, so legacy→exact→legacy
history is possible without duplicate revision identities or a union namespace.
All child tables/headers/receipts/contributions/app/hold/reference tables and
foreign-key definitions stay unchanged. The new immutable snapshot table supplies
typed selected closure but no external provider/config/request store. Snapshot
rows are retained while any plan revision refers to them; no deletion/edit API is
added. Missing shared row is missing_target on admission, invalid_stored_data on
an existing selected capture. Existing same-session staged rows are provisional;
reuse is not evidence they were previously committed.

The public `SqliteApplicationPlanStore` constructor/retain_initial/append/find/
find_revision signatures remain unchanged. Exact snapshot insertion is internal
to retain/append; no independent auto-committing snapshot store is exposed. The
new compiled manifest must be explicitly inspected/migrated by the authorized
installation lifecycle before use. Old binaries fail their supported catalog/
expected-schema gates before invoking old plan readers or writers; they may not
open version-new data and project old columns as a missing quotation. Current
binaries retain the old-format write/read path, without automatic semantic upgrade.

Full find validates/materializes complete history and all selected snapshot
bodies, within complete output budgets (the new aggregate ceiling applies when
the returned history contains an exact alternative). Selected find validates exactly the chosen
row/body/ordered children/shared snapshot plus existing header/head/max/initial
anchor/local parent and typed source obligations. It does not materialize all
ancestors or certify full history continuity. Initial-anchor and append metadata
checks must recognize the new tag and validate its valuation shape, exact original
money/resolution projection and required shared target using bounded key/shape
queries; they do not certify unrelated unselected snapshot bodies. Malformed
selected/initial bodies fail; unselected full snapshot payload validation belongs
to full materialization. Append still validates all old structural chain/audit/
valuation metadata in bounded pages without a full-read quota; cumulative writer
limits apply. Do not use full history loading as an append gate.

Bundle selected/full/claim/hold readers use the same tag semantics and complete
byte accounting, including actual selected closure; no private old-decimal-only
projection may silently bypass new consistency checks. V1 reference captures
remain no-money/not-applicable with one fixed request. Their structural reader
recognizes format1 and refuses it as outside this producer's supported scope;
it never upgrades SRC1/SRA1/SRX1/SRI1 to monetary forms or follows a current quote.

## 7. Error order, atomicity and exact effect calculation

New pure exceptions are `ExactValuationFailure : std::invalid_argument`, accessor
`code() noexcept`, holding `ExactValuationError` with these named alternatives:
`resource_limit, unsupported_format, malformed_document, invalid_reference,
invalid_money, invalid_basis, invalid_rate, missing_pair, inconsistent_rate,
missing_target, identity_conflict, allocation_failure`. No input text/secret in
messages. Known parser/model failures map by meaning; unknown exceptions are not
fabricated business success. `std::bad_alloc` from feature-owned construction is
translated to allocation_failure without allocating an attacker-sized diagnostic.

Pure validation priority is: fixed outer limits and all borrowed count/byte bounds
(resource_limit), outer version (unsupported_format), complete framing/known tags
(malformed_document), reference/money/common rational grammar and positivity,
basis shape, full snapshot/manual consistency, then exact output projection before
construction. Fields within one stage use declaration/grammar order. Bit/temporary
bounds apply before attempted numeric growth, even when later semantic faults
exist. Missing_target/identity_conflict apply to retained codec closure after
complete fitting EVK1 framing. No SQL occurs in these codecs.

SQL operations preserve existing option/key/candidate validation priorities before
SQL. New errors are mapped to `SqlitePlanInputError` through added alternatives
`invalid_exact_valuation`, `unsupported_valuation_format`, `valuation_resource_limit`,
`valuation_allocation_failure`; untouched legacy cases keep current codes/order.
A snapshot conflict maps to an added storage reason `snapshot_identity_conflict`;
missing required snapshot maps to `missing_valuation_snapshot`. Healthy unknown
retained outer format maps to added storage reason `unsupported_valuation_format`;
known malformed/contradictory stored data remains invalid_stored_data. Selected
materialization overage is result_limit. Existing service mapping must preserve
unsupported as not_ready and resource/allocation/storage as their distinct known
failure sources with whole rollback; it must not manufacture a missing-rate hold.
The concrete API/stub stage must expose these distinct additions without renumbering
existing enum alternatives. Capability/wrong-thread/protocol/unknown work outcomes
remain their existing `SqliteWorkFailure`, not an input exception.

Preflight and check candidate/current-head/identity conflicts before making own
writes. Snapshot plus revision/children/head updates stage within ONE actual
caller session. Any later failure after staging rolls back the whole caller unit,
including new shared row, head, effects/claim/jobs/audit; existing durable intake
and old revisions remain. Return values are provisional until real COMMIT and
normal work completion. A snapshot conflict cannot overwrite old evidence.
Unknown commit stays unknown; idempotent lookup/reconciliation must establish the
actual operation before another award. Successful commit then body exception
retains committed data and no normal completion payload. No DTO decode error
itself persists a review hold.

RATES-009's pure bridge supplies exact effective rate and capture/codec/closure
checks. It does not invent a complete rule evaluator. For an already selected
rule/modifier request, callers multiply original exact amount×rate×exact factors×
unit scaling using checked cancellation and round ONCE per final requested effect
with CORE-001 half-even. Finalizer component resource refusal and signed64 range
overflow remain distinct. Range overflow holds the whole affected input through
the existing reviewed service workflow with no partial effect/job; a helper does
not create the hold. Historical sync/override/reporting edits never recalculate
committed requests/effects/residuals. Refunds retain original effect allocation and
rate provenance; no refund-date conversion is introduced.

## 8. Independent literals and executable baseline plan

These are independently derived proposed examples, not executed test evidence.
Framing literals below use the exact B/O/V notation in §5 as a compact byte recipe;
no expected value may be generated from candidate codecs.

| ID | Literal input and independently required result |
| --- | --- |
| EV01 | EUR→USD quote `1.20`; component EUR1/1,USD6/5 backed by quote ordinal1. Capture USD→EUR has N5,D6; quote spelling remains `1.20` after full/retained codec and reopen |
| EV02 | EUR→USD1.20 and EUR→GBP0.90, components EUR1/1,GBP9/10,USD6/5. USD→GBP=(9/10)/(6/5)=3/4; preserve both separate observations/times if from different providers |
| EV03 | Amount3 USD×5/6 EUR/USD×1 unit/EUR =5/2→2, residual+1/2; amount9→15/2→8,residual−1/2. No exchange-rate rounding |
| EV04 | EUR→EUR amount1.2000, identity1/1; no snapshot/manual. Equal arbitrary tokens do not certify supported currency. Same-currency manual1.00 is invalid_basis |
| EV05 | Enter EUR→USD `1.2000` as one-off manual for USD→EUR; effective5/6. Actor/decision/reason retained, no table pin/snapshot/provider facts |
| EV06 | `05/6`, `5/06`, `10/12`, `0/1`, `1/0`, `+5/6` rejected; a2467-byte string still exceeding8192 bits is resource_limit before gcd |
| EV07 | A valid available pair in an oversized bridge snapshot is resource_limit, not missing_pair. Full RATES-001 acceptance of that larger model remains unchanged |
| EV08 | Source-rate UTC absent/text present, fetch UTC0 and publication text present-empty survive distinctly; no timestamp borrowing |
| EV09 | Snapshot S canonical bytes identical→reuse; same key with `1.2000` replacing `1.20`→identity_conflict although numeric rate equals. Missing key or key/body mismatch refuses capture |
| EV10 | Change current sync/pin/reporting currency after capture→reopened ERV1 unchanged. Refund original effect uses its actual2/8 credited units, not a current rate |
| EV11 | Legacy effective decimal `1.2000` remains literal old bytes. New5/6 writes no effective_rate_decimal. Opaque legacy `unparsed` remains structurally readable, unavailable to exact adapter without distinct refusal |
| EV12 | Populated legacy1→exact2→legacy3 retains all row/key/FK/audit/source values; old binary catalog refuses upgraded store. No automatic old semantic upgrade |
| EV13 | Known EVK1 outer format+trailing byte→malformed; unknown healthy outer format→unsupported. Unknown basis byte in known EVK1→malformed, no guessed legacy dispatch |
| EV14 | Full result repeated shared snapshot bytes exceed1MiB→result_limit before result growth; one fitting selected revision still returns. Append uses structural paging, no materialization quota |
| EV15 | At input/store/statement/session exact bound succeeds if all other bounds fit; one byte/item/bit over fails before growing the corresponding output. Two fitting queries can exhaust cumulative session budget |
| EV16 | SQL/known rollback after staged snapshot/revision→no new row/head/effect/job. Unknown COMMIT→unknown,no payload/no blind second award. Fresh process resolves actual durable operation |
| EV17 | Normalized final request4097-bit component→resource refusal; exact final signed64 MAX+1→range overflow whole input hold, no partial award |
| EV18 | Exact plan resolution differs from captured ContributionValuation resolution→invalid exact valuation; matching decision bytes alone never authorize manual override |

Literal empty snapshot `snapshot_ref=S`, both optionals absent, all vectors empty:
`RSV1 B(S) 00 00 V0 V0 V0 V0`, exactly27 bytes
(4+5+1+1+4×4). It is valid model evidence but cannot back a foreign resolved pair.
Identity amount `1`,currency`U`,comparison`U`,resolution`r`,rate1/1 is
`ERV1 B(r) B(1) B(U) B(U) B(1) B(1) 00`, exactly35 bytes
(4+6×5+1). Its EVK1 form replaces only the four-byte magic and is also35.
Manual same input with originalcurrency`U`,comparison`V`,rate2/1,
enteredU→V`2`,decision`d`,actor`a`,reason`x`,timeabsent is
`ERV1 B(r) B(1) B(U) B(V) B(2) B(1) 02 B(U) B(V) B(2) B(d) B(a) B(x) 00`,
exactly66 bytes (35+6×5+1). Present UTC0 adds8 bytes beyond its already present
optional tag, yielding74. A decoder must reject U32 count/truncation/trailing
errors before allocating a vector from attacker-supplied counts.

RATESTEST-009 baseline revision2 is additive: old22 core plan methods,29 SQL plan
methods, all bundle/hold/claim/service/reference methods, shipped fixtures and
prefix1–8 freezes remain intact in their original owning suites. Add focused
exact factory/full+retained/snapshot codec literal tests, capture/revise mixed
history cases, real-SQL write/read/reopen/upgrade/FK/immutable shared closure and
selected/full/accounting/service regressions for the new alternative. Add source/
resource review for preflight/copy/streaming/closure, not allocator-count tautologies.
Expected bytes and fractions must be independently authored literal recipes,
including all source context/optional/order fields. Reuse existing synthetic
writer fixtures; no provider credentials or live contribution effects.

Before any minimal header/stub/build execution, obtain the complete independent
contract/API and fixture/oracle review cycle required by the workflow. Compile
valid setup, independently review tests, then observe intended behavioral stub-red;
compile/dependency/skip failures are not red. The migration prerequisite has its
own authored/reviewed real-SQL baseline and implementation cycle. RATES-009 then
implements the reviewed bridge baseline with separate implementation/source review
and relevant passing component/real-SQL/CI checks. Do not replay old historical red
or weaken assertions to fit the new design. No full provider/UI/release capability
is established by this document or those bounded component checks.
