# Immutable configuration and arrival-context retention — revision 1

Task: STORAGETEST-012 / STORAGE-012. Authority: specification v0.45 §§3, 6, 7,
DB-01, [slice brief](../tasks/EPIC-009/STORAGE-012.md) and
[event/state boundaries](event-state-boundaries-v1.md). This is an internal
retention contract for later interfaces/tests, not implemented storage or a
configuration/HTTP/wire schema. Preserve the existing core/receipt/plan ports,
[writer/work contract](writer-transactions-v1.md), migration contract and reviewed
24 migration/49 writer-work cases and seven environment cases.

## Identity, content and known links

There are two record kinds: configuration document and captured arrival-context
document. Identity is `(kind, exact opaque reference bytes)` within this server's
database. A reference names one immutable version, not a mutable object whose
current revision can change. The existing `ReceiptArrival.configuration_ref` and
`modifier_context_ref` remain `std::string`; no numeric parsing or core change is
introduced. The same bytes in different kinds name distinct records.

References are nonempty bounded byte sequences. Store their full length as BLOB,
without QString conversion, UTF-8 validation, trimming, case folding, NUL
termination or decimal normalization. Caller validation determines whether a
particular reference is meaningful; storage does not invent its syntax. This
mechanism can preserve initial U+FEFF, embedded NUL or non-UTF-8 bytes without
promising that a future product/wire schema accepts them.

A document is a required, present owned byte sequence, including a valid present
empty sequence. Store it as BLOB, never parse/reserialize it or turn SQL NULL into
an empty document. At a QByteArray boundary, null input is invalid while a
non-null zero-length value represents present empty content. On reading, SQLite
`typeof(document)='blob'` and byte length are authoritative: a valid zero-byte
BLOB must survive even if a driver exposes an empty/null buffer representation.
Missing rows, SQL NULL and a present empty BLOB are distinct facts.

Configuration records have no link metadata. An arrival-context record has at
most one optional typed configuration reference. Absent means no known link was
supplied; it stays SQL NULL. A present reference must be nonempty and resolve to
the configuration namespace in the same transaction, including a configuration
inserted earlier in that session. A context with matching bytes cannot satisfy
that lookup. No guessed current configuration, zero/empty placeholder or inferred
source fact replaces absence. This optional mechanism does not make missing
required receipt context acceptable: the intake caller still validates the
complete arrival references before relying on them.
Dangling or incorrectly typed stored links are storage failures, not partial
records with guessed targets.

Payloads may retain rule/modifier/version/valuation facts in a caller-defined
document format, but this module neither interprets them nor proves their truth.
RulePlanSource/ModifierPlanSource and later format/resolver contracts still own
meaning and resolution inside such documents. Keeping opaque bytes alone does not
implement their evaluator or authorize an override. New meanings get new opaque
references; old references/documents/links are never rewritten or enriched.

## Primitive operations and transaction composition

Primitives bind to an already active existing `SqliteWorkSession`:

| Operation | Required observable result |
| --- | --- |
| Find configuration/context by its typed reference | An owned exact record or explicit missing; never a fabricated/default/current record |
| Retain a new configuration | inserted; original bytes retained |
| Retain a new context with all present links resolved | inserted; exact bytes and optional link retained |
| Retain the same identity with identical bytes and optional-link presence/value | already_present, no duplicate or mutation |
| Reuse identity with different bytes or link presence/value | conflict, original remains unchanged |
| Retain a new context with a missing known target | missing_reference, no context inserted |

Input/kind/bound validation happens before that primitive's SQL. Domain conflict
and missing-reference results make no writes of their own and do not poison a
healthy session. For an existing identity, compare its immutable record first:
different requested content/link is conflict, rather than an attempted retarget
or unknown-field fill. No hash-only comparison substitutes for exact equality.

The caller owns BEGIN, real COMMIT, abandonment/rollback and the enclosing command's
atomic policy. Retaining config and context can share one work/session with later
receipt primitives; there is no new connection, nested transaction, enqueue per
lookup or autonomous commit. A caller that wants an all-or-nothing unit must
propagate refusal or roll it back; a conflict status does not imply earlier
independent writes in that session were undone. A read-only committed work may
publish a conflict/missing diagnostic without claiming insertion success.

Returned records/status are speculative until the enclosing session's actual
commit and normal terminal work completion. No caller may acknowledge durable
retention or publish captured state from an uncommitted return. Uncaught failures
and abandoned work follow existing finalization. SQL/budget/capability failures
retain sticky rollback/uncertainty categories; no adapter maps uncertain durability
to known-uncommitted success/failure or automatically retries. A malformed stored
row is an explicit storage failure, not missing/default data; establish rollback
before claiming it known uncommitted, otherwise preserve outcome_unknown and
fail-closed behavior. Preserve foreign/stale rejection and owned result lifetimes.

No update/delete/merge/latest-pointer API belongs to revision 1. Later authorized
retention/deletion must preserve resolvability for still-retained histories.
These are trusted storage primitives, not a sandbox against arbitrary direct SQL.

## Proposed physical format and bounds

For integrator and independent review, propose two main-schema tables, with all
runtime reads/writes explicitly qualified to main. Each is created only by a
reviewed migration, never lazily by find/retain/open:

```sql
CREATE TABLE subathon_retained_configurations(reference BLOB NOT NULL PRIMARY KEY CHECK(typeof(reference)='blob' AND length(reference)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'))
CREATE TABLE subathon_retained_contexts(reference BLOB NOT NULL PRIMARY KEY CHECK(typeof(reference)='blob' AND length(reference)>0), document BLOB NOT NULL CHECK(typeof(document)='blob'), configuration_ref BLOB CHECK(configuration_ref IS NULL OR (typeof(configuration_ref)='blob' AND length(configuration_ref)>0)) REFERENCES subathon_retained_configurations(reference))
```

Integrator proposal for actual review: these exact two statements form application
store migration version 1, immutable ID `retain-config-context-v1`, in that order.
Supported starts are exactly 0 and 1. Its expected projection contains both tables,
their NULL-SQL primary-key indexes `sqlite_autoindex_subathon_retained_configurations_1`
and `sqlite_autoindex_subathon_retained_contexts_1`, and all three unchanged reserved
catalog objects. No other object is omitted. Later receipt/contribution/plan slices
append migrations; they must not rewrite this version's SQL, ID or projection.
These root-owned choices still require separate actual contract review and exact
independent fixture/digest freeze; selected official-Qt fixture validation precedes
feature red. Root host existing-session probes confirmed both DDL statements and
these generated NULL-SQL indexes; that is not official bundle or feature proof.
[Probe evidence](../evidence/storagetest-012-probe.md).

Root owns actual DDL acceptance, manifest identity/order and exact schema/digest
fixtures before interface/test freeze. Use the verified migration/catalog mechanism;
no second catalog, IF NOT EXISTS adoption, repair or disabled foreign keys. Existing
unrelated schema/data/catalog history remain intact. No receipt/contribution/plan/
effect/job table or document-format layout is chosen here.

Uniqueness and foreign keys enforce the same identities/known links as primitive
checks. Read actual stored types/ranges, not column affinity or QVariant nullness
alone; reject invalid rows without rewriting them. Primitives are synchronous:
callers keep input buffers valid/immutable through the call, the adapter snapshots
their exact lengths for SQL, and returned records own their bytes beyond session
closure. No borrowed view is retained after return. This does not normalize unsafe
borrowed captures in an enclosing queued work item; its existing ownership
preconditions still apply.

Proposed positive trusted engineering defaults: reference limit 4096 bytes and
document limit 262144 bytes. Count raw bytes, not characters; optional present
references obey the same bound. These are configurable implementation limits, not
a new product format restriction. Every primitive query/binding/result still
consumes the existing cumulative session budgets. Do not reset/raise them or
fragment transactions. Input within these bounds may still exhaust the remaining
session budget and must fail safely. No hard SQL/callback deadline is promised.

## Full-range numeric preservation without invented IDs

The brief's lossless-range/codec requirement is preserved explicitly. Actual
configuration/context and rule/modifier references in current core ports are
opaque strings, not numeric version-key types. Revision 1 therefore introduces
no numeric surrogate ID/revision, counter, ordering or unused uint64 SQL codec.
Its reviewed codec is exact reference/document bytes to BLOB and back.

Decimal spellings and binary representations covering all uint64 values, including
zero and 18446744073709551615, must round-trip unchanged as opaque content/reference
bytes; leading zeros or distinct binary sequences are never conflated. This does
not certify a numeric SQL conversion or make zero a legal domain ID. Actual
ReceiptId, ContributionId and CapturedPlanRevision are uint64 fields: their typed
lossless SQL codec/zero/range gates remain mandatory in STORAGE-013, STORAGE-014
and STORAGE-015 respectively, before those fields persist. Never bind them through
signed INTEGER/REAL by assumption. The integrator must link this written scope
reconciliation from the briefs; no downstream or release requirement is dropped.

## Literal future acceptance examples

These are future independent oracles, not authored tests, red or runtime results.
Use disposable files, literal SQL/BLOB observations and the existing private writer.

| ID | Input and required result |
| --- | --- |
| RC01 | Retain config `config-v1` with bytes `00ff4100`, context `ctx-v1` with distinct bytes and known link to it, commit once: both resolve exactly, including after writer/reader reopen |
| RC02 | Present empty document persists as typeof blob/length0; missing lookup is absent; SQL NULL document is rejected, never returned as empty |
| RC03 | Payload/reference UTF-8 `efbbbf61`, binary00/ff and embedded NUL preserve exact lengths/bytes; no decoder or text binding consumes initial U+FEFF |
| RC04 | Exact duplicate identity/bytes/link returns already_present; record count/data stay unchanged. Changed payload, absent-to-present link or changed link under the same identity conflicts without rewriting old state |
| RC05 | Context/config use the same reference bytes and remain distinct. A context-only target does not satisfy a present configuration link; missing/empty known link cannot create an orphan context |
| RC06 | Absent optional configuration link remains absent/SQL NULL through reopen. No empty/zero/current placeholder or unknown monetary/rate/actor fact is manufactured |
| RC07 | Add `config-v2` and `ctx-v2`; old `ctx-v1` still points to immutable `config-v1`. Later edits/expiry/caller-buffer mutation cannot rewrite either historical meaning |
| RC08 | Owner session inserts config then linked context before commit; same session resolves them, independent observer sees neither. Abandon/throw rolls back both; actual commit makes both visible before durable success |
| RC09 | Return conflict after an earlier insert: the caller explicitly rolls back for an all-or-nothing command. A separately chosen read-only committed diagnostic is not reported as a successful insertion |
| RC10 | Literal refs/payloads for `0`, `1`, `9007199254740993`, `9223372036854775807`, `9223372036854775808`, `18446744073709551615`, and binary `0000000000000000`/`8000000000000000`/`ffffffffffffffff` all round-trip without numeric coercion; distinct spellings remain distinct |
| RC11 | Exact configured reference/document byte bounds pass input validation; one over refuses without primitive writes. Cumulative session resource failure after an earlier retain rolls back the enclosing unit with no partial success |
| RC12 | SQL/busy failure, stale/foreign session and lost commit/rollback acknowledgment retain existing truthful outcomes/no retry; no retention report converts uncertainty to a normal durable result |

Actual contract review precedes minimal interfaces/stubs/tests. Those need different
test review, successful build and intended missing-behavior red before STORAGE-012
implementation. SQL durability/reopen checks do not establish backup/recovery,
actual configuration semantics/evaluation, permission/update authority, providers
or release-platform support. All seven integrations and three rate providers remain.
