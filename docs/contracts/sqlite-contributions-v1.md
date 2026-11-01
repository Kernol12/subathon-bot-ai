# Contribution evidence SQL adapter — revision 1

Task: STORAGETEST-014 / STORAGE-014. Authority: specification v0.45 §§3.2–3.5,
§14, DB-01, [slice brief](../tasks/EPIC-009/STORAGE-014.md),
[contribution eligibility/linkage](contribution-eligibility-v1.md),
[event/state boundaries](event-state-boundaries-v1.md),
[writer work](writer-transactions-v1.md),
[receipt SQL](sqlite-receipts-v1.md) and [retention](retained-context-v1.md).
This is an initial internal contract proposal, not implemented SQL, API, tests or
provider proof. Root owns exact additive migration3/DDL/order/projection/digests;
reviewed retention1/receipt2, existing interfaces/assertions and release scope remain.

## Preserve the existing service boundary

Implement the unchanged [ContributionRepository/Transaction](../../server/include/subathon/server/contribution_evidence.hpp)
for the existing [core values](../../core/include/subathon/core/contribution.hpp).
[ContributionEvidenceService](../../server/src/contribution_evidence.cpp) alone
chooses created/linked/duplicate/unresolved and promotes explicit confirmed
evidence. SQL supplies primitives, not correlation, classification, eligibility,
permission, settlement or reconciliation policy. The original
[21 service/core simulations](../../tests/acceptance/contribution_evidence_tests.cpp)
remain unchanged fake-backed evidence, not retroactive SQL proof.

`find_receipt(receipt_id)` looks up a **contribution observation** and its current
association, not a receipt-intake record. An existing receipt without an observation
returns absent here. `find_activity(identity)` looks up a contribution by the
exact caller-established activity key; `unresolved_activity(identity)` returns all
matching still-unassociated observations. No lookup guesses a link from a raw
payment/activity reference, event-type spelling, actor/name, money or timestamp.

| Primitive | Required storage effect/result |
| --- | --- |
| find_receipt | Owned exact observation/current optional association, or absent; no new observation |
| find_activity | Owned contribution/current state for exact verified four-tuple, or absent; never first/latest among ambiguous matches |
| unresolved_activity | Complete bounded collection with matching present verified key and SQL NULL association, in ascending canonical ReceiptId order |
| create | Allocate one ContributionId, anchor immutable first support receipt, store supplied optional activity identity and current state |
| append_observation | Insert one exact receipt observation, supplied evidence/role/proof/support conditions and optional association; no evidence merge |
| link_observation | Resolve an existing NULL association to a compatible existing contribution; same exact association is an idempotent no-op, another target is refused |
| update_state | Replace only that contribution's current state with supplied values; no original/observation change or implicit promotion |
| commit | Acknowledge actual physical session commit before the service returns |

The first support supplies immutable original facts/arrival and captured support
conditions, even if confirmation arrived first. Its observation preserves those
conditions forever; the contribution's separate current state may be changed by
explicit update_state calls. Existing service confirmation changes only payment
evidence; delayed unknown/unconfirmed input cannot overwrite confirmed proof or
first context. The SQL adapter must not implement that decision on the service's
behalf or derive it from references. Direct state writes are trusted primitive
operations, not evidence that an actor was authorized to make them.

## Receipt originals, committed preconditions and real references

ContributionReceipt means the immutable **original** receipt projection: ReceiptId,
four-part receipt identity, original facts and original arrival. Compare every
supplied projection field with the actual main.subathon_receipts original before
create/append accepts it. Resolve that original's real configuration and context
in their corresponding retained namespaces on the same session. Never substitute
an intake retry/conflict attempt, latest arrival, mutable current context, source
time for arrival time or an incomplete/default receipt. Growing intake attempts
remain receipt audit, not extra contribution observations.

A missing actual receipt is a known storage refusal on create/append, not a newly
invented receipt. A supplied projection differing from the existing original is
an explicit mismatch refusal, not an override/reclassification. A selected stored
original/dangling target with invalid type/shape/range fails as stored corruption.
Finds of existing contribution data validate their selected original/observation
projections and retained targets before returning owned records. The context's
optional known config link need not equal original arrival.configuration_ref:
receipt/retention contracts require each actual target to resolve, not that new
cross-field equality. Unknown known-link absence stays absent.

**Trusted upstream precondition:** each supplied receipt was committed before this
contribution transaction began, is non-conflicting normalized original input, and
has the caller-established support/confirmation classification. SQL projection
comparison and same-session visibility prove neither earlier commit nor absence
of a conflicting intake history. The present writer session exposes no committed-
when-began capability. This revision preserves that precondition instead of claiming
that a nonempty string, boolean, copied snapshot or current SELECT proves it.
Processing an intake newly staged in this same transaction is outside the supported
calling contract. Normal routing completes receipt intake and its committed work,
then uses a later contribution work item. No private writer introspection, second
writer, hidden read connection or nested transaction is introduced to fake proof.

Later implementation needs a private original-only lookup by ReceiptId on the
existing active session inside the new014 module, returning the existing
ContributionReceipt projection with strict receipt SQL decoding/target validation
and owned bytes. Test it through the public contribution adapter; no new public
lookup API is needed merely to expose this helper. Concrete reuse/decoder details
remain a separate API/implementation review gate. Existing13 public find is by
identity and loads full growing history, so wrapping
another ReceiptRepository/commit or using its find as a new original-only capability
is unsuitable. No current13 header/source/test is modified by this proposal.

## Identity, association and ambiguity

VerifiedActivityIdentity is exact provider, stable source/account, linkage namespace
and opaque activity ID. All four are required nonempty BLOB values when present;
all absent values are SQL NULL together. A database UNIQUE constraint protects
one contribution per present four-tuple. Another UNIQUE protects one contribution
per original support ReceiptId, including support without a verified activity key.
Receipt-origin equality and nonempty proof requirements already belong to the
service's validated observation shape; the adapter preserves those exact values.

`verification_ref` is immutable observation provenance and does not participate in
activity identity uniqueness. No SQL lookup treats its nonempty bytes as provider
verification/authorization. Different real origin/account/namespace/opaque tuples
stay separate, even with identical evidence and money. No-key support may create
its own contribution; no-key confirmation stays unassociated rather than matching
by resemblance.

One observation row per ReceiptId has a primary/unique key. A direct duplicate
append is a real constraint failure, not an upsert or storage-side duplicate
classification; the existing service handles exact duplicates via find_receipt.
If that service sees the same ReceiptId with a changed observation, its existing
std::invalid_argument behavior remains unchanged. Exact duplicate after linking
returns the now-associated ID, including present ContributionId0.

For a linked keyed observation, its verified identity must exactly equal the
contribution's present key. An unkeyed linked observation can only be that unkeyed
contribution's original support receipt. These are relational consistency checks,
not new provider-proof policy. Link requires both rows and the same compatibility;
it can move only NULL to the compatible ID or leave the same ID unchanged. Never
retarget an already associated observation or enrich an originally absent key/proof.
A mismatched key, incompatible no-key association or different existing target is
an explicit association conflict with rollback. Missing write targets fail rather
than silently succeeding. Stored ambiguous lookup matches or malformed associations
fail explicitly, never select one arbitrarily or repair data.

Unresolved lookup excludes every non-NULL association, including eight zero bytes.
It must return the full selected collection or a limit error, not truncate pending
confirmations. Link/update primitives do not apply that read materialization cap
as a write quota. Observations and first support snapshot remain immutable after
association/state changes; only association NULL-to-ID and current state are mutable.

## Full-range typed ContributionId and allocator

Independent searches of the agreed contribution/receipt/plan contracts, core/server
values/implementations and existing assertions found no ContributionId0 rejection.
PlanEvent uses optional ContributionId; its capture validation rejects ReceiptId0,
not ContributionId0. A fake starting1 alone defines no product domain restriction.
Therefore this proposal supports **ContributionId0 through UINT64_MAX**, including
allocated/stored zero. ReceiptId remains1..UINT64_MAX under its separate reviewed
plan/receipt rule. Never copy that nonzero check onto ContributionId.

The typed ContributionId codec uses exactly eight big-endian bytes, full uint64
accumulation and strict input length. Actual SQL values must be typeof BLOB/length8,
never TEXT/INTEGER/REAL, even if conversion yields equivalent bytes. The existing
full unsigned byte codec may be reused internally, but the new actual typed SQL
boundary and zero/optional handling need separate reviewed tests/red. Optional
absence is SQL NULL/std::nullopt; BLOB0000000000000000 is present ID0. No truthiness,
nonzero predicate or unsigned-to-signed narrowing may erase it.

| ContributionId value | Required literal BLOB hex |
| --- | --- |
| 0 | 0000000000000000 |
| 1 | 0000000000000001 |
| 9007199254740993 | 0020000000000001 |
| 9223372036854775807 | 7fffffffffffffff |
| 9223372036854775808 | 8000000000000000 |
| 18446744073709551615 | ffffffffffffffff |

Propose one allocator singleton1 with nullable last_allocated BLOB8. NULL means no
contribution allocated; first allocation is0. Thereafter zero is a real last ID,
so next is1. Check canonical bytes/domain/metadata cardinality and consistency
against the greatest actual contribution ID in this no-deletion revision: NULL
requires no contributions; non-NULL equals the greatest ID. Allocate MAX exactly
once, then fail known-uncommitted without wrap, searching gaps or reusing a committed
ID. Update allocator/new contribution/observations/links/state in the same unit.
Failure rolls back the candidate ID; an uncommitted candidate may be reused.
Malformed/missing/inconsistent allocator is corrupt, never reset to NULL/zero.
Future authorized deletion must preserve no-reuse before deleting any contribution.

## Proposed logical rows and exact independent values

Propose three main-schema tables: allocator, contributions and contribution
observations. Contributions hold typed ID, optional activity tuple, original
support ReceiptId FK and full current ContributionState. Observation ReceiptId FK
identifies the original projection, plus kind/evidence, optional verified tuple/
verification reference, optional captured support conditions and nullable associated
ContributionId FK. Receipt originals are already immutable: an original-only join
can reconstruct facts/arrival without duplicating a growing transport history.
Root chooses actual physical layout after reviewing these invariants; no actual
DDL, migration factory, source codec or new core ID is authored here.

A newly created contribution must have its original support observation linked to
it before port commit. This structural complete-unit guard prevents a committed
anchor without evidence; it does not classify facts, promote evidence or replace
support conditions. Earlier unresolved confirmations can be linked in the same
unit. No source deletes or rewrites an observation because support arrived later.

Store std::string values as exact-length BLOB, including NUL/FF/initialEFBBBF,
without QString/UTF decoding, decimal parsing, trimming or case conversion. A
present empty string is nonnull BLOB length0; optional absence is SQL NULL. Pair
optional money decimal/currency together; optional verified link and optional review
decision are all-present groups, not partially NULL structures. Optional support
conditions are a complete conditions block or entirely absent. Required strings
such as policy_ref preserve even an empty supplied value; storage must not invent
a new semantic nonempty rule absent from the existing service/core contract.

All enum/boolean SQL representations require actual INTEGER storage class and
explicit known values. Proposed fixed enum mapping follows the existing declarations:

| Dimension | Stored values |
| --- | --- |
| Observation kind | support0, confirmation1 |
| PaymentEvidence | unknown0, unconfirmed1, confirmed2, not_applicable3 |
| ContributionReview | pending0, not_required1, accepted2, rejected3 |
| EvidencePolicy | require_evidence_review0, accept_provider_report1, record_only2 |
| EvidenceReviewChoice | await_confirmation0, accept_labelled_report1 |
| ContributionApplication | not_applied0, applied1 |
| ContributionReversal | none0, partial1, full2 |
| SettlementGate | unresolved0, clear1 |
| monetary / missing_rate | false0, true1, independently |

Preserve every ContributionConditions field: valuation monetary, optional original
money, optional resolution_ref, independent missing_rate; ordinary review;
evidence policy/policy_ref/optional decision choice+decision_ref+actor_ref;
application, reversal and settlement. Missing original money is not zero/nonmonetary;
a present resolution reference does not clear missing_rate. State/default combinations
remain separate stored facts; no eligibility normalization is performed. A supplied
state update can preserve rejected/applied/full-reversed/unsettled markers while
changing only evidence, exactly as the existing service does.

Policy/decision/actor/verification/resolution references have semantic retained-
meaning and authorization preconditions in their owning upstream contracts. This
adapter retains their exact known/unknown values but has no finalized SQL namespace
or resolver proving that meaning. Do not reinterpret them as configuration keys,
create fake retained rows, or claim permissions from nonempty bytes. Actual arrival
config/context resolution is physical SQL proof; those other versioned semantic
resolution/authorization gates remain explicit unfinished product work.

ContributionValuation contains no fetched/effective rate, rate source/time or
quote layout. Preserve its actual original_money spellings (for example20.0000,
20.00, signed/long exact decimals) and resolution reference without conversion.
Actual HistoricalValuation rate/source/timestamp/manual provenance belongs to
[application-plan capture](application-plans-v1.md) and STORAGE-015; numeric/valuation
policy remains its agreed scope. No unused rate column or omitted downstream proof
is introduced. Hydrated receipt values retain reviewed uint32/signed64/optional
representations; no new zero/time/version or monetary parser rule is added.

## Transaction ownership, limits and failures

Use the same controlled SqliteWorkContext/Session mechanism as receipt SQL.
Context-mode port begin performs one real context.begin; explicit adopt-active mode
transfers commit/cleanup authority over an already active unit only after wrapper
allocation succeeds, without nested BEGIN. Repository values are inert/single-use/
owner-thread values; the wrapper owns its session/options/state independently of
repository lifetime. A caught second port begin rejects locally and leaves the
first unit healthy. The adopted true-owner/active state is a trusted caller
precondition until actual session operations check it; no writer-state escape.
Construction/allocation failure preserves the caller's cleanup authority or cleans
an already begun unit exception-safely. Wrapper destruction is existing noexcept
cleanup; owner finalization remains the backstop. No hidden queue, SQL handle,
second writer, nested wait or retry appears behind synchronous primitives.

Successful observe returns only after the wrapper's actual session.commit; external
publication still waits for normal committed work completion. In adoption mode all
earlier valid staged work in that unit commits or rolls back with the contribution.
Adoption does not waive the already-committed-receipt upstream precondition.

Propose positive configurable reference_bytes4096, value_bytes262144 and
unresolved_per_lookup1024. Reference bytes cover identity/ref/proof/policy/decision/
resolution strings and the evidence decision's actor_ref. Value bytes cover both
original receipt actor strings, event/money strings, valuation money strings and
selected retained document payloads, preserving receipt13's field-bound roles. The limits
bound exact raw bytes/whole results, not product observation quotas or history
expiry. Selected well-typed larger values/results fail as result/resource limits,
not missing/corrupt/truncated. Counts/conversions/lookahead are checked for overflow;
concrete representable bounds and error priority need separate API precision review.
All original-only/target queries share cumulative existing session budgets, never
reset or split. No fixed internal query-count plan or SQL runtime deadline is promised.

Pure invalid input/options are pre-SQL input failures, not claims of known rollback;
caught input errors do not undo earlier caller writes. Actual malformed selected
rows/targets/allocator, ambiguity, missing write targets/receipts, projection mismatch,
association conflict, allocator exhaustion or incomplete-new-support guard require
confirmed active rollback before ContributionStorageError. Existing known SQL/busy/
resource/result failures retain actual outer outcome/error while mapping to the port
only when known uncommitted. Foreign/stale/terminal/protocol errors remain actual
SqliteWorkFailure. Unknown commit/cleanup stays SqliteWorkFailure(uncertain_outcome),
outranks all local reasons and is never relabelled known ContributionStorageError,
normal success or permission to replay. No automatic outcome resolution/reopening
or financial reconciliation is supplied. Direct SQL uniqueness prevents primitive
upserts; trusted arbitrary SQL is not sandboxed by this adapter.

## Literal future SQL acceptance examples

These are specification-derived future oracles, not authored tests/approval/runtime.
All receipt fixtures must be separately committed original records unless explicitly
exercising unsupported preconditions diagnostically; none may claim current SELECT
proves prior commitment.

| ID | Required independent observations |
| --- | --- |
| SC01 | Typed six literal IDs above, including first allocated0 and present optional0; raw typeofBLOB/length8, no signed/REAL path. Wrong7/9-byte/type and content-equivalent TEXT allocator reject |
| SC02 | NULL allocator/no rows ->0, then1; consistent literal signedMAX/MAX-1 seeds cross2^63/MAX once, exhaustion no wrap/reuse, unchanged snapshot on failure |
| SC03 | Real service first support creates one immutable anchor/conditions/observation; explicit confirmation links/promotes evidence only, preserving independent review/valuation/admission/application/reversal/settlement and original support context |
| SC04 | Confirmation-before-support stays unresolved without contribution; later matching support anchors its own committed original and links all matching pending observations atomically, including contribution0 |
| SC05 | Repeated receipt is duplicate, not another observation. Distinct confirmation receipts retain distinct proof/context audit; changed same-receipt observation throws existing invalid_argument, immutable history preserved |
| SC06 | Each verified four-tuple component and binary/NUL/FF/BOM branch remains exact; proof_ref changes provenance only. Raw payment/actor/money/times or no-key confirmation never imply linkage |
| SC07 | Find_receipt absent observation remains absent despite existing receipt; missing actual original/write target fails later. Supplied latest-attempt/conflict facts/arrival cannot replace original; correct original plus actual typed retained refs succeeds |
| SC08 | Full-state/default combinations, optionalNULL/presentempty/grouped money/proof/decision/support fields, exact long decimal and uint64zero survive reopen/owned results. No invented rate/policy authorization value appears |
| SC09 | Direct duplicate activity/original/observation insert is SQL uniqueness failure; same-target link no-op, retarget/mismatched key/missing row refuses. Corrupt ambiguous matches reject rather than choosing first |
| SC10 | Fail after observed +1 contribution/observation row, actual link NULL-to-ID, or actual state update: raw originals/observations/association/state/allocator snapshot unchanged after rollback; retry recomputes from durable data |
| SC11 | Adoption over an already active unit commits earlier valid staged work plus contribution before observe returns; abandonment/failed complete-support guard rolls back. It proves composition, not that a same-unit newly created receipt had previously committed |
| SC12 | Exact configured limits pass, one over/preflight priority refuses; complete unresolved read over cap never truncates and link/update have no read quota. Two individually fitting results exhaust cumulative budget only after external post-stage flags prove prior writes |
| SC13 | Selected malformed groups/types/enum/IDs/target and well-typed target/document limit map correctly; independent BEGIN IMMEDIATE inside catch proves synchronous rollback while wrapper alive, with exact errors/no normal payload/raw snapshot unchanged |
| SC14 | Foreign/stale/terminal/local-second-begin and repository/wrapper/result lifetimes preserve actual owner/capability/error rules; worker NotImplemented diagnostic must rethrow on owner without uncaught thread exception |
| SC15 | Real physical commit-ack loss leaves possible durable data but unknown/no observe result/failclosed; real cleanup-ack loss after proved staging outranks local reason. One body call/no auto-replay or fabricated rollback proof |

Root next supplies exact additive migration3 and full projection/literal digest,
without rewriting prefixes1/2. Actual different contract review precedes minimal
API/explicit stubs, then separately reviewed tests/fixtures/harness, successful
compile/selected official Qt green setup and intended red before STORAGE-014.
The old21 simulations and completed receipt gates are preserved, not repeated.
No effects/jobs/application bundle, permission/evidence authority, semantic namespace
resolution, reconciliation, backup/update/recovery/power-loss or platform/provider
release gate is completed. All seven integrations and three rate providers remain.
