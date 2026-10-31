# Contribution eligibility and confirmation linkage — revision 1

Task: ACCTTEST-002; subsequent implementation: ACCT-002.
Authority: [specification v0.45](../specification.md) §§3.2–4,5,14,18 and reviewed
[event/state boundaries](event-state-boundaries-v1.md) and
[identified receipt intake](receipt-intake-v1.md), both revision 1.
Status: authored contract/tests awaiting independent review and intended red. Core
assessment and server linkage throw explicit `…NotImplemented` exceptions.

## Bounded responsibility

This baseline has two pieces: a pure core eligibility assessment over independent
dimensions, and a server service that associates committed receipt observations
with a contribution using explicitly verified same-source activity identity. Neither
piece applies effects, emits jobs or advances application/reversal markers. An empty
blocker set permits only further planning within this projection. Authorization,
clock/lifecycle eligibility, current-state caps, revision validation and atomic effect
application remain later gates; no timer owner/policy question is answered here.

Core contains standard-library values and the assessment interface. The server owns
the framework-free writer-side service and primitive repository port. The fake is
disposable transaction storage, not a database or provider. No complete contribution
ledger, wire schema, Qt queue, SQL uniqueness, remote acknowledgement, live provider
capability or process-restart durability is supplied.

## Independent dimensions and provenance

`ContributionState` keeps payment evidence separate from `ContributionConditions`:

| Dimension | Representation and preserved meaning |
| --- | --- |
| Payment evidence | `confirmed`, `unconfirmed`, `unknown`, `not_applicable`; never inferred from review or valuation |
| Valuation | Explicit monetary requirement, optional lossless original money, optional retained resolution reference, independent missing-rate flag |
| Ordinary input review | `not_required`, `pending`, `accepted`, `rejected`; accepting this dimension cannot decide insufficient-evidence admission |
| Evidence admission | Captured §14 policy reference and optional separately scoped evidence-review decision/actor references |
| Application | `not_applied`, `applied`; this slice never applies or resets it |
| Reversal | `none`, `partial`, `full`; this slice never calculates, performs or resets a reversal |
| Settlement gate | Explicit caller-supplied `clear` or `unresolved`; no default assumption of financial settlement |

Value-initialized/default state is conservative: unknown evidence, pending ordinary
review, evidence-review policy without a decision, unresolved settlement and an
unresolved monetary requirement with absent money/resolution. The missing-rate flag
defaults false because an omitted rate is not a known rate failure; unresolved
valuation still blocks. Standalone zero-valued evidence/policy/review/settlement enums
also choose those conservative states. Explicit resolution is required to advance.

A monetary input with absent source money is not a zero-valued or nonmonetary input.
A supplied valuation reference cannot manufacture its missing amount or clear the
independent missing-rate flag. The reference identifies a retained resolution with
rate/manual-valuation provenance where required, including when/how it was resolved;
it does not claim the rate existed at arrival. Parsing money/rates and calculating
valuation belong to their own exact numeric/planning contracts. Nonmonetary inputs
explicitly declare that valuation is not required; absence alone cannot select that.

The first support observation supplies captured conditions, original receipt facts
and arrival/context. Policy and decision references resolve to retained versioned
meaning. Ordinary input-review operations, evidence-review authorization and later
valuation changes are outside this service; they require their audited writer paths
(ACCT-003/005 and API permission/revision contracts). The assessor observes the current
supplied state without mutating any dimension or reference.

## Insufficient evidence and §14 policy

These are captured local values, not a promise about any provider's evidence quality:

| Captured policy | Insufficient evidence (`unknown` / `unconfirmed`) | Confirmed / not applicable |
| --- | --- | --- |
| `accept_provider_report` | Admitted with original evidence label retained | Admitted |
| `require_evidence_review`, no scoped decision | `evidence_review_required` | Admitted |
| Same policy, explicit `await_confirmation` decision | `payment_evidence_required` | Admitted |
| Same policy, explicit `accept_labelled_report` decision | Admitted with original evidence label retained | Admitted |
| `record_only` | `record_only_policy` | `record_only_policy` |

An ordinary `ContributionReview::accepted` is never the scoped evidence decision.
The evidence decision has its own retained decision/actor references. This permits
the §14 reviewed provider-report option without relabeling uncertainty as confirmation.
Other ordinary review/valuation/settlement holds still apply. Record-only remains
the captured policy even if evidence later confirms; a retrospective policy override
needs its own audited contract and is not silently selected by this baseline.

## Assessment result

`assess_contribution_eligibility` returns an unordered-in-meaning set containing every
applicable `ContributionBlocker`. There is no precedence that hides another hold:

- Evidence-policy blockers follow the table above.
- For monetary inputs, absent original money adds `missing_source_amount`; an explicit
  missing-rate flag adds `missing_rate`. When the resolution reference is absent and
  the rate is not marked missing, add `valuation_unresolved`. A reference is not
  sufficient to clear either separately stated source-amount/rate hold.
- Pending/rejected ordinary review adds `review_pending`/`review_rejected`.
- Applied input adds `already_applied`, including after partial/full reversal.
- Unresolved settlement adds `settlement_review_required`. A never-applied input
  with partial/full reversal also adds that blocker even if its supplied settlement
  flag says clear: this slice cannot select an award after refund-before-application.

Refund-before-apply, unsettled/contradictory financial evidence, pending refund policy
and any subsequent permitted award/reconciliation are explicit ACCTTEST-006/ACCT-006
gates. This baseline holds them; it neither invents net-payment eligibility nor treats
a refund as authority to reopen application. Caller-provided clear settlement must
be justified by its own evidence/policy. Partial/full reversed applied input still
cannot become an initial application candidate.

## Receipt observations and verified linkage

`ContributionReceipt` is the immutable committed receipt projection: internal receipt
ID, identity, original facts and original arrival. It deliberately excludes intake's
growing transport-attempt collection. Repeated transport delivery remains traceable
in receipt intake; it must not become a different contribution observation. The caller
must supply a real committed, non-conflicting normalized receipt and correct source
classification. No connector writes authoritative contribution state directly.

`ContributionObservation` carries the receipt projection, explicit support/confirmation
role, actual evidence label, optional `VerifiedActivityLink` and support conditions.
Primary support requires conditions; supplementary confirmation has none and must
carry explicit confirmed evidence. This role is a caller-validated normalization,
not guessed by parsing event-type strings, payment references or a moderation state.

`VerifiedActivityIdentity` is provider + stable source/account + documented linkage
namespace + opaque activity ID. Equality is byte-exact across all four components.
Its origin must match the receipt origin in this bounded same-source contract.
`VerifiedActivityLink::verification_ref` records the caller's retained capability/
correlation evidence; it does not participate in identity equality. A raw
`ReceiptLinkReference`, equal name/amount/time, or merely nonempty key is **not**
proof. Real construction/authorization and cross-provider correlation/source authority
remain API-006 and connector gates; the public C++ value itself cannot establish them.

The caller must reconcile contradictory monetary facts and settlement evidence before
presenting observations as the same verified activity. That provider-specific policy
is not guessed here. Missing verified linkage can still create one support-receipt
contribution, but no supplementary confirmation may be attached to it by resemblance.

| Observation / repository state | Committed result and state |
| --- | --- |
| First support receipt, no matching verified activity | Create one contribution anchored to that support's original facts/arrival/conditions; append linked observation |
| New support/confirmation with matching verified activity | Append linked observation, return existing contribution; preserve original support snapshot and all conditions |
| Explicit confirmed evidence for that contribution | Set payment-evidence projection to confirmed; no other dimension changes |
| Delayed unknown/unconfirmed support after confirmation | Retain its actual evidence label in its observation; do not erase confirmed evidence or original snapshot |
| Confirmation without matching support, including missing verified linkage | Append unresolved observation, no contribution or planning/application request |
| Support later arrives with the matching verified key | Create its contribution and link all retained matching unresolved confirmations in the same transaction; evidence becomes confirmed |
| Exact same receipt observation submitted again | Return existing association (or still unresolved), disposition `duplicate`; do not append a second identical observation |
| Same receipt ID submitted with different immutable projection/classification/key/conditions | `std::invalid_argument`; preserve all committed history; it is not an override path |

The distinct disposition for an initially unassociated confirmation is `unresolved`;
for first support it is `created`; new observations attached to an existing
contribution return `linked`. Repeated receipts return `duplicate` even when unresolved.
Every distinct confirmation receipt stays in the observation audit. A repeat of a
previously unresolved receipt after linking returns its now-existing contribution ID.

Local choice for out-of-order supplemental evidence: a confirmation alone does not
establish a support contribution or its award context. When support arrives later,
its own captured arrival/context is the anchor, even though confirmation evidence
was retained earlier. Both receipt arrivals remain visible. This does not replace
arrival by provider source time or pretend confirmation arrived with the support.
More general provider recovery/import source-role semantics require connector baselines.

This bounded service only promotes the aggregate evidence projection to confirmed
from explicit confirmed observations. Unknown/unconfirmed supplementary observations
remain in the audit and cannot supersede the original projection or confirmed proof.
Other evidence supersession/conflict/financial settlement requires the reconciliation
gates above. It never applies effects or emits a candidate/job, so repeated confirmation
cannot itself award twice. The assessor separately prevents planning already-applied
contributions; ACCT-004 must still enforce one actual application transaction.

## Atomic port, input errors and simulation limits

Each observation uses one `ContributionRepository` transaction. The port offers
receipt/activity lookups, unresolved-observation lookup, primitive create/append/link/
state-update and commit. Service code owns identity/idempotency/linkage/evidence decisions.
Original contribution, observation, pending associations and evidence update commit
together; successful results return after commit. Known uncommitted write/commit
failures throw `ContributionStorageError` and expose no partial result or writes.
Destruction without commit rolls back. Indeterminate durable commit outcomes require
adapter resolution against durable identity, not an assumed rollback.

Malformed local observation shapes (support without conditions, confirmation with
conditions or without confirmed evidence), mismatched verified-key origin, missing
identity/proof reference, or contradictory receipt-ID reuse throw `std::invalid_argument`
without changing committed state. Other callers' authorization and provider payload
validation remain separate preconditions. This error type is a local API choice,
not a finalized transport response or source reconciliation policy.

The fake copies primitive storage for staging, performs exact lookups/allocation,
and writes exactly the values supplied. It does not enforce uniqueness, merge evidence,
classify retries or correlate references. Failure points are after staged create,
observation append, pending link or state update, and before publication at commit.
The test seed method represents a separately committed conditions update, not a fake
application implementation. Tests compare committed snapshots and retry outcomes.
No SQLite durability/concurrency/uniqueness, crash safety, writer-thread ownership,
live service, credentials or side effects are exercised. Those remain STORAGE/API gates.

## Acceptance examples and handoff gate

The [21 authored GoogleTest cases](../../tests/acceptance/contribution_evidence_tests.cpp)
use literal synthetic inputs and manually derived blocker/association expectations:

| Cases | Required result / authority |
| --- | --- |
| DefaultState… | Omitted source/conditions retain unknowns and all independent unresolved holds; §§3.2–3,14 |
| MultipleHolds…, RecordedValuation… | Review acceptance/valuation resolution leave other holds and labels intact; §§3.3–4,5 |
| ProviderReport…, EvidenceReview…, CapturedRecordOnly… | §14 policies admit labelled evidence only through explicit policy/decision, preserving independent review and record-only |
| HiddenSourceAmount…, NonMonetary… | Unknown amount is absent, not zero/ready; nonmonetary/not-applicable stays distinct; §§3.2–3,14 |
| ApplicationReversal… | Applied never reopens; never-applied reversal or unsettled input holds for ACCT-006; §§3.3–4,8 |
| FirstSupport…, LaterConfirmation… | First support facts/context/conditions and decision refs retained; confirmation changes evidence only and keeps one contribution; §§3.2–4 |
| RepeatedReceipts…, ConfirmationBeforeSupport…, DelayedUnconfirmed… | Idempotent receipts, distinct audit observations and out-of-order linkage never create another contribution or erase proof/context; §§3.3–4,18 |
| RawPaymentReference…, VerifiedActivityIdentity… | No inferred correlation; correct provider/account/namespace/opaque scope; §3.2 and API-006 gate |
| ConfirmationPreserves… | Confirmation cannot alter review/application/reversal/settlement or produce another planning candidate; §§3.3,5,8 |
| FailedInitialSupport…, FailedConfirmation…, FailedPendingLink… | Known injected transaction failure leaves no partial contribution/link/evidence; retry preserves existing data; §3.4 |
| InvalidEvidence… | Explicit local validation and immutable receipt consistency cannot fabricate confirmation or replace policy history |

Independent review must examine exact contract/header/stub/fake/test artifacts before
the first runtime red. Successful compilation/discovery alone is not red; intended
red must come from the explicit core/service `…NotImplemented` exceptions. ACCT-002
production starts only after that gate and preserves the reviewed assertions.
The coordinator owns CMake/presets/pins, progress/HTML/map/checkpoint, CI and PRs.
The separate `contribution-evidence` lane must not enter a passing merge gate while
intentionally red or use skipping/failure inversion to hide failures. Once implemented,
stable reviewed suites must run on relevant PR/main CI under the workflow policy.
All seven integrations, three exchange providers and full release scope remain required.
