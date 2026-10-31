# Identified receipt intake — revision 1

Task: ACCTTEST-001; subsequent implementation: ACCT-001.
Authority: [specification v0.45](../specification.md) §§3.2–4,14,18 and the reviewed
[event/state boundary revision 1](event-state-boundaries-v1.md).
Status: authored baseline requiring independent review and intended red; no receipt
behavior is implemented. The C++ service throws `ReceiptIntakeNotImplemented`.

## Scope and preconditions

This contract covers a bounded local projection of an **already validated, identified
normalized observation**, recorded by the authoritative writer. It separates original
receipt facts from individual delivery attempts and returns a committed receipt ID
plus `created`, `retry` or `conflict`. It never creates a contribution, decides payment
eligibility, applies time/score, emits a delivery job or authorizes provider input.
It supplies no complete normalized-event wire schema, provider payload model or SQL
schema. Core owns only standard-library values; the server owns the intake service
and repository port. No Qt/network/database framework enters core.

The caller supplies owned values, valid normalized event type/schema, source money
as lossless decimal text when available, explicit UTC milliseconds, retained context
references and a fully populated identity whose source stability/namespace semantics
have been established separately. This slice defines no provider authorization,
decimal parser, untrusted-input size limit or empty/missing-ID fallback. Transport
validation and provider-specific policies must establish these preconditions before
routing input here. Missing identifiers are an explicit API-006/connector policy gate,
not an invitation to create IDs from names, amounts or timestamps.

## Identity, facts and attempts

`ReceiptIdentity` has four byte-exact, case-sensitive components:

1. Provider origin.
2. Stable configured source/account identity, scoped to the installation where needed.
3. The documented provider delivery/event identifier namespace.
4. The opaque identifier within that namespace.

All four participate in equality. Do not trim, lowercase, parse opaque IDs as numbers
or substitute a transport connection ID. Reconnecting the same configured source
retains the same identity scope. A separate source account or identifier namespace
can legitimately reuse the same opaque ID. The synthetic namespaces in tests prove
only these local semantics; they establish no real provider identifier capability.

`ReceiptFacts` is the immutable normalized intake projection: event type/schema,
optional source UTC timestamp, actor ID/display name, original decimal/currency,
and optional namespaced payment/activity references. These references are source
linkage evidence, scoped by the receipt origin. They never substitute for the
delivery/event identity. A received event and a confirmation event may share payment
evidence but have distinct receipt IDs. Whether they link to one contribution, and
when that contribution becomes eligible, belongs to ACCTTEST-002/ACCT-002.

Revision 1 chooses a conservative payload-consistency policy: **every field of this
normalized projection participates in equality**, including optional absence versus
presence, source decimal spelling/precision, actor display name and reference
namespaces. This is an audit consistency decision, not a claim that every change is
fraud or a new contribution. For example `12.3400` versus `12.34` retains a different
source precision observation and is a conflict; the existing original is not replaced.
Provider adapters must not include volatile transport metadata in these facts.
Any adapter-specific normalization/duplicate policy must receive its own documented
provider evidence and baseline; this contract does not guess it.

`ReceiptArrival` belongs to each attempt: server arrival UTC milliseconds, connection
reference, retained configuration reference and retained modifier-context reference.
Arrival/context changes do not create a new receipt or change the facts comparison.
The original record retains its first arrival and facts forever in this slice;
later context is recorded in its own attempt and cannot replace the original context.
Source time is never filled from arrival time. Context references must resolve to
retained versioned meaning; storing a pointer to mutable current settings is insufficient.

## Observable decisions

| Repository identity / incoming facts | Result after commit | Recorded evidence |
| --- | --- | --- |
| No matching identity | `created`, newly allocated receipt ID | Original identity/facts/arrival and one `created` attempt |
| Matching identity, every normalized fact equal | `retry`, existing receipt ID | Original unchanged; one additional `retry` attempt with its own arrival and observed facts |
| Matching identity, any normalized fact differs | `conflict`, existing receipt ID | Original unchanged; one additional `conflict` attempt retaining the full conflicting facts/arrival |

Facts always compare with the **original** receipt, even after a conflict. A later
retry of those original facts remains a retry. A repeat conflicting observation
remains traceable as another conflict attempt. Exact arrival timestamps can repeat;
the attempt collection must retain both deliveries rather than using time as identity.
The internal receipt ID is an opaque strong value allocated by the repository, not a
provider ID, contribution ID or transport connection ID.

Cross-provider/source/namespace inputs get distinct raw receipts. This does not
authorize duplicate credit from mirrored services: explicit source authority or
verified correlation remains required in API-006/connector baselines. Equal names,
amounts and timestamps, even exactly equal, do not establish correlation. This
baseline does not claim the §18 one-contribution/one-application scenario is complete;
it establishes the receipt/attempt boundary that ACCT-002 and atomic application use.

## Atomic repository port

`server::ReceiptIntake` owns the deduplication/consistency decision and uses one
`ReceiptRepository` transaction per call. The transaction exposes primitive `find`,
`create`, `append_attempt` and `commit`; it does not classify retries or conflicts.
The receipt ID, original snapshot and first attempt commit together. A retry/conflict
attempt commits before its successful result can return. The repository contract
requires lookup to see the transaction's consistent authoritative state, stable
receipt IDs and no partial writes visible on rollback. Destruction without successful
commit rolls back. Known transaction failures throw `ReceiptStorageError`; this
baseline's injected failures have a known uncommitted outcome. Retry rechecks the
repository rather than depending on a particular service object's cache.

The production adapter must enforce matching database uniqueness and atomicity,
run only on the owned Qt SQL writer connection/thread and handle SQLite commit/error
outcomes correctly. Those claims require STORAGE-002/STORAGETEST-002 and Qt/API
integration evidence; the in-memory fake proves none of them. An indeterminate durable
commit outcome must be resolved against durable identity before reporting/retrying;
it must never be treated as proven rollback merely because the caller saw an error.
No acknowledgement durability or remote exactly-once claim follows from this port.

The disposable fake copies storage into a transaction, exposes committed snapshots,
allocates internal IDs, looks up by exact identity and stages primitive writes. It
does **not** enforce unique identities, compare facts or decide dispositions, so
an implementation that always inserts cannot pass retry tests because of fake policy.
Injected exceptions occur after a staged create/append or before commit publishes
the staged state. The tests check rollback from observable committed data, including
preservation of already committed evidence and a later successful retry. This is
simulation evidence about the service/port contract only.

## Specification-derived acceptance examples

All inputs are synthetic; expected decisions below are manually derived from the
identity/context invariants and the explicitly chosen consistency policy above.
The [15 acceptance cases](../../tests/acceptance/receipt_intake_tests.cpp) have one
primary home in GoogleTest; no sleeps, credentials, provider calls or live contributions.

| Cases | Sequence / required observable result | Authority |
| --- | --- | --- |
| FirstArrival…, ExactRetry… | Arrival then same facts/key: one receipt, same returned ID, two attempts, source precision and both arrivals retained | §§3.2–4,18 same input twice |
| Reconnect… | Same stable source/key over a new connection/configuration/modifier context: retry; first context retained and new context recorded separately | §3.4 arrival context; reviewed event/state identity boundary |
| ProviderAccount…, EqualNames… | Change each identity component separately, or only event ID with equal actor/amount/times: distinct raw receipts; IDs are byte-exact | §3.2 identity/correlation |
| SharedPayment… | Received and confirmation event IDs share payment/activity evidence: distinct receipts; no contribution/eligibility decision made here | §§3.2–3 |
| UnknownOptional…, NewlySupplied… | Missing fields stay absent; later facts under the same ID conflict and preserve the unknown original | §§3.2,14; revision-1 consistency policy |
| ConflictRetains…, ChangedNormalized… | Amount/type/version/time/actor/linkage/presence/precision differs: conflict with complete observation retained; later original facts still retry | §§3.2–4; revision-1 consistency policy |
| NewService… | New service over the same fake repository returns existing receipt and adds attempt | §18 retry identity; no disk/restart claim |
| FirstArrivalWriteFailure… | Exception after staged create or initial attempt: no committed receipt or attempt; later retry creates one | §3.4 atomic durable intake boundary |
| RetryWriteFailure…, ConflictWriteFailure… | Failed new attempt leaves existing committed original/history untouched; later retry commits exactly one additional observation | §3.4 atomic intake boundary |
| CommitFailure… | Known failure before commit for each disposition: no success result and no new committed data; retry re-evaluates and commits correct disposition | §3.4; DB-01, event/state transaction boundary |

## Gates and downstream scope

Baseline progression remains authoring → different reviewer approval of exact
artifacts → successful build → real runtime red caused by `ReceiptIntakeNotImplemented`.
Compilation/dependency failures and skipped cases cannot satisfy red. Production
ACCT-001 starts only after that gate, preserves reviewed assertions and requires its
own separate code review and passing relevant acceptance/regression checks.

The coordinator owns build/presets/pins, acceptance-map/progress/HTML, CI assessment
and publication. The intended dedicated `receipt-intake` CTest lane remains separate
from passing product acceptance until implemented; no failure inversion, `WILL_FAIL`
or blanket continue-on-error. Stable passing reviewed receipt cases must later join
relevant PR/main CI. No provider capability, live operation, SQL recovery/concurrency,
Qt writer, complete contribution accounting or full normalized-event schema is
validated here. All seven integrations, three rate providers and other agreed release
scope remain required.
