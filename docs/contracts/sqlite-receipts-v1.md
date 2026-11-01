# Identified-receipt SQL adapter — revision 1

Task: STORAGETEST-013 / STORAGE-013. Authority: specification v0.45 §§3.2–3.4,
DB-01, [slice brief](../tasks/EPIC-009/STORAGE-013.md),
[receipt intake revision1](receipt-intake-v1.md),
[event/state boundaries](event-state-boundaries-v1.md),
[writer transactions](writer-transactions-v1.md) and
[retained context](retained-context-v1.md).
This initial contract candidate defines an internal SQLite adapter and future
oracles. It implements nothing and changes no existing core type, service decision,
provider capability, wire schema or reviewed assertion. Root owns exact migration2
DDL/order/projection/digests and shared build/state integration before test freeze.

## Existing policy stays in ReceiptIntake

Implement the unchanged [ReceiptRepository / ReceiptTransaction ports](../../server/include/subathon/server/receipt_intake.hpp)
for [core receipt values](../../core/include/subathon/core/receipt.hpp).
[ReceiptIntake](../../server/src/receipt_intake.cpp) alone decides created/retry/
conflict by comparing every supplied fact with the immutable original. The SQL
repository supplies find, create, append_attempt and real commit. It neither
classifies duplicate facts nor turns create into an upsert, retry or replacement.
The original 15 [service tests](../../tests/acceptance/receipt_intake_tests.cpp)
remain fake-backed simulation evidence; their success is not new SQL proof.

Identity is the byte-exact four-tuple provider, stable source/account,
identifier namespace and opaque identifier. All four participate in a database
UNIQUE constraint. Connection reference is arrival provenance, never a uniqueness
component. No trimming, Unicode decoding, case folding, numeric parsing or
names/amount/time correlation is introduced. Caller-owned normalized inputs have
already passed their provider/transport/domain validation; no missing-identifier
fallback or new unidentified-provider capability is supported here.

Every attempt persists its own complete observed facts, arrival and supplied
known disposition. Equal arrival timestamps and identical retry/conflict
observations still append separate ordered attempts. Original identity/facts/
arrival never change. No adapter compares attempts instead of the original or
reclassifies the caller's disposition. Unknown enum values are invalid input;
known enum values are stored exactly, not used to infer financial truth.

## One real transaction and explicit adoption

There are two explicit construction modes for the future concrete repository:

- **Context mode:** hold the caller's revocable SqliteWorkContext. The port's
  single begin call maps to context.begin(), performing real BEGIN IMMEDIATE.
  The resulting transaction exclusively owns port-level commit/cleanup authority.
- **Adopt-active mode:** hold a caller-supplied SqliteWorkSession with an explicit
  adoption tag. The caller has already begun this same work's transaction and may
  have staged retention records. The single port begin transfers exclusive
  commit/cleanup authority over that existing unit to the transaction wrapper;
  it performs no BEGIN, savepoint, event-loop wait or fake deferred vote.

Constructing/destroying a repository value does no SQL or cleanup. In adoption
mode the caller owns cleanup until port begin actually transfers authority. The
caller must keep the real active owner session and request buffers valid, and
must not use another copy to commit/rollback or mutate that unit while its port
transaction owns it. A copied capability does not extend SQL/writer lifetime.
No private writer-state access or fake statement is added to validate adoption:
validity/owner/terminal failures are checked by actual session operations.
A second port begin on the same repository is a local protocol misuse and never
starts another transaction. Actual writer one-begin rules still apply.

The transaction's primitive calls execute synchronously on that session. Commit
calls the actual session.commit and returns only on acknowledged physical COMMIT.
In adoption mode this commits earlier staged retention plus receipt/attempt and
allocator changes together. ReceiptIntake then returns to the enclosing work;
publication still waits for normal committed work completion. No primitive opens
another connection, queues its own batch, pumps Qt events, retries or starts an
independent writer. No raw SQL handle escapes.

Transaction destruction is noexcept cleanup of its live active owner session.
Committed, rolled-back and expired tickets remain cleanup no-ops. Owner work
finalization remains the backstop if a wrapper is retained or abandoned. Failure
to confirm cleanup records outcome_unknown and fail-closes the existing writer;
it cannot become a destructor exception or an invented known rollback.

Create stages one original and allocator advance. Append stages one attempt,
assigning its internal order without altering original facts. A new original must
have at least one appended attempt before port commit; otherwise commit rejects
and establishes rollback. This structural guard does not classify a supplied
attempt or change the service's created/retry/conflict policy. Find may see an
own newly staged original before its first append; an already persisted original
without attempts is corrupt. All original/first-attempt/allocator state commits
or rolls back as one unit. A known failed transaction may reuse its uncommitted
candidate ID; no committed allocated ID is reused by this revision.

## Typed uint64 codec and domain-zero reconciliation

The new pure codec accepts every std::uint64_t value 0 through 18446744073709551615.
Encode exactly eight bytes, most significant first: byte i is the low eight bits
of value shifted right by 8*(7-i). Decode requires an eight-byte sequence and
accumulates into uint64 without a signed intermediary. It has no endian/locale/
QString/decimal/REAL dependency. Raw SQL representation is strictly typeof blob,
length8; INTEGER, REAL, TEXT, NULL or a different length cannot impersonate an ID.
The pure codec's zero representation is valid and independently tested.

Allocated and persisted **ReceiptId domain values are 1 through UINT64_MAX**.
The already reviewed [application-plan Capture contract](application-plans-v1.md)
rejects a zero receipt ID, as do [the implementation](../../core/src/application_plan.cpp) (line19)
and [its reviewed assertion](../../tests/acceptance/application_plan_tests.cpp) (line262).
This is the compatibility basis; the disposable fake's initial ID1 alone would
not establish it. No completed plan contract/test is changed. A zero input to
append is invalid before SQL; a stored receipt/attempt ID0 is corrupt. Thus the
full numeric codec range and the existing nonzero domain rule are distinct;
there is no silent signed-range restriction or reinterpretation of provider IDs.

Propose one allocator row, singleton1, with nullable last_allocated BLOB8.
SQL NULL means no ID has been allocated, and the first ID is1. Non-NULL zero is
not an absence sentinel. A non-NULL canonical domain value produces value+1 only
when strictly below UINT64_MAX. UINT64_MAX can be allocated once; the next create
reports allocator exhaustion after known rollback, never wraps or searches for
an older gap. Update the row in the same transaction as original/attempt writes.
Uniqueness/failed append/commit rollback also rolls back that update. Malformed,
missing or duplicate allocator metadata is corrupt, not permission to reset it.
Before allocation, compare the typed current greatest stored receipt ID with
allocator state: NULL requires no originals; non-NULL must equal the greatest
original ID in this no-deletion revision. Reject inconsistency without repair.
Stored IDs selected by this check must pass strict BLOB8/domain validation.
No delete/update/current-pointer API is introduced; future authorized retention
must preserve allocator no-reuse policy before deleting receipt records.

| uint64 codec value | Required literal hex |
| --- | --- |
| 0 | 0000000000000000 |
| 1 | 0000000000000001 |
| 9007199254740993 | 0020000000000001 |
| 9223372036854775807 | 7fffffffffffffff |
| 9223372036854775808 | 8000000000000000 |
| 18446744073709551615 | ffffffffffffffff |

These are typed numeric oracles, additional to retention's opaque byte vectors.
The later actual ContributionId and CapturedPlanRevision codecs/zero policies
remain STORAGE-014/015 gates and are not certified by this document.

## Lossless row values and selected-data checks

Propose three main-schema tables: allocator, immutable originals and append-only
attempts. The originals contain domain ReceiptId BLOB8 primary key, four required
nonempty identity BLOBs with composite UNIQUE, and the complete facts/arrival
column families below. Attempts contain receipt_id BLOB8 FK, positive internal
ordinal INTEGER, facts/arrival families and disposition INTEGER with explicit
mapping created=0, retry=1, conflict=2. Their primary key is (receipt_id,ordinal).
The ordinal is storage ordering, not a new core AttemptId: it starts1 per receipt,
increments without gaps, retains equal-time deliveries and fails known-uncommitted
at signed64 exhaustion. No timestamp or content uniqueness replaces it.

| Column family | SQL values / exact reconstruction |
| --- | --- |
| Facts event_type | Required BLOB containing exact std::string bytes |
| Facts schema_version | INTEGER in 0..4294967295, reconstructed as uint32 without narrowing; no new nonzero policy |
| Optional source_timestamp_utc_ms | SQL NULL or signed64 INTEGER, including zero and both signed extrema; never filled from arrival |
| Optional actor_id / actor_display_name | Independently SQL NULL or BLOB; present empty BLOB differs from absent |
| Optional money | original_decimal and currency both NULL or both BLOB; exact bytes/spelling, never a numeric SQL value |
| Optional payment_reference / activity_reference | Each namespace/opaque-id pair both NULL or both BLOB; scopes preserved independently of delivery identity |
| Arrival server_timestamp_utc_ms | Required signed64 INTEGER, including both extrema; no floating/date conversion |
| Arrival connection_ref | Required exact BLOB, including present empty if supplied; not an identity key |
| Arrival configuration_ref / modifier_context_ref | Required nonempty exact BLOB foreign keys to the corresponding retained namespace |

All std::string fields preserve exact byte lengths, embedded NUL, FF and initial
EFBBBF; no text codec or Qt binding may consume them. Empty std::string maps to a
nonnull zero-byte BLOB, never a null QByteArray binding. Optional absence maps to
SQL NULL; present empty maps to BLOB length0. For grouped optional values, partial
NULL/wrong-type groups are corrupt rather than absent or a guessed value. The
adapter does not parse source decimal syntax or normalize precision/case.
`12.3400` and `12.34` remain different observations; a long exact spelling such as
`9007199254740993.00000000000000000001` is preserved literally. Present-empty source
values remain distinct storage facts; semantic acceptability remains upstream.

SQLite affinity alone is insufficient for these checks: actual stored types
must be validated. SQLite documents signed INTEGER and exact byte BLOB storage,
including affinity conversions that can lose numeric precision; the proposal
avoids those conversions. [SQLite datatypes](https://www.sqlite.org/datatype3.html).
Foreign keys enforce referenced existence with compatible stored BLOB keys; the
existing writer already enables enforcement. Runtime selected-record validation
still rejects missing or malformed stored targets. [SQLite foreign keys](https://www.sqlite.org/foreignkeys.html).
No pragma policy setter or disabled-FK path belongs to this adapter.

A create and every append resolve their supplied arrival configuration and context
through the existing retention primitives in this same session. Find validates
the selected original and returned attempts plus their retained targets. A stored
context's optional known configuration is checked by retention itself, but **need
not equal that arrival's configuration_ref**: existing ports promise resolvability,
not that cross-field equality. The opaque context may name historical definitions.
An absent known link remains unknown; neither validation nor retry enriches it.
No new semantic constraint is silently imposed on existing valid input.

Validate selected row shape, exact storage classes, numeric ranges, enum values,
optional grouping, ID/reference equality and ordered attempt sequence before
returning an owned ReceiptRecord. Selected dangling links, malformed allocator,
IDs or row groups fail explicitly. A missing identity returns std::nullopt; it
never means selected malformed data was silently discarded. Do not claim a
whole-database audit: only relevant allocator/extreme-ID checks, selected receipt,
its ordered attempts and their retained targets are validated here. Existing
startup catalog/integrity/FK inspection remains a caller responsibility.

ReceiptFacts has no exchange-rate field. Its original_decimal/currency are stored
here exactly. Actual HistoricalValuation effective/fetched rate spellings and
quote/source/timestamp provenance remain in [application-plan history](application-plans-v1.md)
and STORAGE-015, with contribution valuation requirements in STORAGE-014. This
written reconciliation of the brief's money/rate wording neither invents an unused
receipt rate column nor drops downstream lossless rate proof.

## Bounds, errors and durability

Propose positive configurable reference_bytes=4096, value_bytes=262144 and
attempts_per_find=1024. Reference bound covers all four identity fields, arrival
connection/configuration/context references and optional linkage components;
value bound covers event type, optional actor strings and original decimal/
currency. Raw byte lengths count, not characters. Attempt bound is a per-find
materialization limit, not a deletion/drop/expiry policy or product retry quota.
Find returns the entire ordered collection or fails known-uncommitted; never
truncates. Checked lengths/counts reject overflow. Underlying writer cumulative
statement/binding/result budgets remain independently charged across all calls,
including retained-target checks; none is reset, raised or fragmented internally.
A larger valid selected value/history is a result/resource-limit failure, not
corruption or missing. Actual API review will pin concrete exception categories
and checked representable option limits before new test authoring.

Pure invalid options/input (empty required identity/retained refs, oversized
fields, ID0, unknown disposition enum) are rejected before that primitive's SQL
as input errors, not a claim of known rollback. They do not undo prior caller
writes if caught; an escaping exception still triggers owner finalization.
No rate/money/provider normalization or new schema-version/time-zero rule is added.
Valid inputs preserve actual capability errors without a private-state probe.

For an actual local storage failure (missing retained target, corrupt selected
data/allocator, exhausted allocator/order or selected-value limit), perform active
cleanup and establish known rollback before exposing ReceiptStorageError.
A confirmed failed/busy BEGIN is known not_started and may use that port error.
Existing SQL/constraint/budget errors already carry sticky rollback status; only
a known-uncommitted one can map to ReceiptStorageError. Preserve its existing
outer WorkError and transaction outcome. Foreign/stale/terminal/protocol errors
stay their original SqliteWorkFailure; never mutate another work to manufacture
an adapter error. Unknown commit/cleanup stays SqliteWorkFailure(uncertain_outcome),
outranking local reasons and never a ReceiptStorageError, normal result or replay.
A lost acknowledgment can follow a real physical commit; observing an error does
not establish absence of durable data. No automatic recovery/resolution/retry is
introduced. No success escapes ReceiptIntake before real commit or is published
before the normal terminal committed work result.

## Literal future acceptance and remaining gates

These examples are future oracles, not authored tests, review or runtime results.

| ID | Required independent observations |
| --- | --- |
| SR01 | Codec literals above round-trip exactly; 7/9-byte, TEXT, INTEGER, REAL and NULL stored IDs reject. Zero succeeds pure codec and rejects domain append/stored receipt; first allocated ID1 |
| SR02 | Seed allocator/records consistently at 7fff... and ffff...fe using independent raw fixtures; next IDs are 8000... and ffff..., then explicit exhaustion/no wrap and snapshot unchanged |
| SR03 | Initial input creates one immutable original/created attempt. Same facts with changed arrival retries with same ID. Changed `12.3400` to `12.34` conflicts; later original facts retry; equal timestamps still yield separate ordered attempts |
| SR04 | Change each of four identity components independently, including case/NUL/FF/BOM edges: distinct originals. Same key over another connection remains same receipt. Equal actor/amount/times or payment/activity evidence cannot merge distinct identities |
| SR05 | Absent optional facts stay SQL NULL; known-empty actor stays BLOB length0. Optional groups, uint32 max/zero, signed64 extrema, exact decimal and original/attempt snapshots round-trip including reopen and caller-buffer mutation |
| SR06 | Every attempt retains its own config/context. Missing/wrong namespace target rejects and rolls back. Known context->config different from arrival config remains valid if both resolve; absent link stays absent |
| SR07 | Caller begins one session, retains new config/context, adopts it into existing ReceiptIntake, stages original/attempt and commits once. Observer sees nothing before commit and complete data before service return; abandonment rolls back all three state families |
| SR08 | Direct duplicate primitive create fails real SQL uniqueness without upsert. Failure after proved create or append stages rolls back original/attempt/allocator together; retry allocates correctly. Commit without first attempt refuses |
| SR09 | Local selected corruption/dangling target/limit failure proves synchronous rollback with an independent BEGIN IMMEDIATE probe inside catch. Unrelated corrupt data is not silently globally repaired/audited; full returned history never truncated |
| SR10 | Exact configured bounds pass, one over fails pre-SQL without undoing prior writes; independent nonpositive limits reject. Literal cumulative SQL budget failure after a post-create/append stage flag rolls back whole unit |
| SR11 | Foreign/stale/terminal tickets preserve inherited errors and owned result lifetimes. Context-mode second begin does not start another unit; explicit adoption does not consume another writer begin |
| SR12 | Lost physical COMMIT acknowledgment persists rows but yields unknown/no normal service result/failclosed. Lost cleanup acknowledgment after proved staging/corrupt read outranks local error; physical rollback does not authorize known-rollback classification. No automatic replay |

Root supplies the actual [physical migration2 proposal](sqlite-receipts-schema-v1.md)
with exact ID/DDL/order/projection and independent payload/digest, preserving
migration1 bit-for-bit and supporting the reviewed catalog prefix. Different
contract review must accept that actual companion before minimal API/stubs; actual
tests/fixtures/harness receive their own different review, successful build and
selected official Qt environment proof before intended red. STORAGE-013 remains
unimplemented until that gate. Backup/update/recovery authority, power loss,
provider/source capability, complete normalized wire formats, contributions,
evaluation, effects/jobs/application bundle and release platforms remain separate
requirements; all seven integrations and three rate providers remain in scope.

## Concrete API precision for separate review

The new [sqlite_receipts.hpp](../../server/include/subathon/server/sqlite_receipts.hpp)
provides encode_receipt_uint64(uint64), decode_receipt_uint64(QByteArray), pure
receipt_repository_manifest(), SqliteReceiptLimits and the concrete noncopyable,
nonmovable SqliteReceiptRepository implementing the unchanged ReceiptRepository.
Its context constructor and explicit adopt_active_receipt_session tag select the
two modes above. No concrete transaction type or Qt SQL handle enters the public
port. Repository construction stores only capability/options and the construction
QThread identity; its destructor releases values without SQL or cleanup.

Options are fixed for the wrapper's lifetime. Reference/value byte options must
be positive qsizetype values (their type supplies the upper representable bound).
Attempts_per_find must satisfy 1 <= value < numeric_limits<qsizetype>::max(),
reserving a representable count+1 sentinel; checked conversions must also fit
SQLite signed64 and std::vector size. Current targets use signed64 qsizetype.
No artificial statement-count/binding-cost ceiling is added to these options.
Limits default to4096/262144/1024 as above. Value_bytes also bounds selected
retained documents when resolving targets: pass the same reference/value limits
to the existing retention facade, rather than silently applying unrelated defaults.
A well-typed larger retained document maps to result_limit, not corrupt/missing.

The concrete input failures derive from std::invalid_argument and expose
SqliteReceiptInputError. They make no SQL/state/cleanup change when caught:

| Operation | Ordered pre-SQL checks / category |
| --- | --- |
| decode_receipt_uint64 | Null or length other than8: invalid_codec; every canonical eight-byte value including zero decodes |
| Repository begin | Invalid options: invalid_options; then construction-thread guard; then consumed single-use guard: already_begun |
| Transaction find | Identity: each component required nonempty and bounded: invalid_identity |
| Transaction create | Identity, then facts byte bounds: invalid_facts, then arrival ref byte bounds/required config/context: invalid_arrival |
| Transaction append_attempt | ID0: invalid_receipt_id; facts; arrival; then unknown disposition enum: invalid_disposition |

Facts byte validation includes all present grouped values without parsing them:
optional linkage component bounds use reference_bytes, event/actor/money fields
use value_bytes. Arrival connection may be empty but is bounded; required config/
context refs cannot be empty. All timestamps and uint32 values are already typed,
including extrema/zero; no extra input rule rejects those. Present-empty optional
std::string values remain valid storage representations. Valid pure codec encode
has no domain-zero check; stored SQL type checks belong to row decoding, not a
QByteArray-only codec's invented QVariant conversion.

For repository begin, invalid options precede foreign-thread/local-use rejection.
Compare construction-thread identity before reading or mutating the single-use
guard. A foreign call throws SqliteWorkFailure(wrong_thread) without touching
that guard or SQL; a later valid owner call remains possible. This local identity
is not a promise to inspect a weak ticket's true owner/phase: constructing a
repository itself is required on its authoritative owner, and actual context/
session operations enforce their own owner/lifetime/protocol checks. Adoption's
true-owner/active precondition remains the caller's responsibility until then.

A caught second PORT begin throws SqliteReceiptInputFailure(already_begun), not
an invented sticky writer protocol failure or ReceiptStorageError. It leaves the
first transaction usable and neither commits nor cleans it, whether that first
wrapper is active, terminal or already destroyed. A foreign caller receives its
thread rejection before this guard. The repository is single-use after a real
first BEGIN attempt even if that attempt is busy/failed; underlying writer second
begin behavior is not altered. No mutable guard is shared across threads.

Construction/allocation failure before a wrapper owns anything leaves adoption's
session/cleanup authority with the caller and does not consume the local guard.
Mark adoption consumed and transfer authority only after successful allocation/
construction; returning the owning unique_ptr must not allocate again. In context
mode, arrange allocation before BEGIN or use an exception-safe guard that cleans a
newly begun session if later construction fails. Never leave an active transaction
because std::bad_alloc interrupted wrapper construction. Once returned, a wrapper
owns its copied session/options, owner identity and pending-first-attempt set
independently of repository lifetime; it must not retain a dangling repository
reference. Its destructor only calls the existing noexcept cleanup operation.

Transaction primitive input checks above precede thread/session access. For valid
input, reject a foreign construction-owner call before reading/mutating pending
state, then let actual session SQL/commit enforce ticket validity and sticky
terminal semantics. Commit has no field preflight; thread rejection precedes its
pending-first-attempt guard. Every ordinary commit actually invokes session.commit;
there is no outer pending vote. A pending original without an append uses the
known incomplete_receipt failure only after active rollback. Trusted callers may
not independently commit/rollback the adopted session behind the wrapper.

Find materializes the complete selected history within attempts_per_find or fails
result_limit after known cleanup. Append instead validates order via an owned
single-row COUNT/MIN/MAX aggregate (and selected original/target checks), not by
calling full-history find. Under the required UNIQUE positive ordinal key,
count=N/min=1/max=N establishes contiguous order; zero attempts are permitted only
for this wrapper's newly pending original. Validate aggregate types/ranges before
checked max+1. Appending does not apply attempts_per_find as a write quota or
silently discard old attempts. SQL execution still consumes the same work budgets;
no hard query-runtime promise follows from a small aggregate result.

Known-uncommitted SqliteReceiptStorageFailure derives from the existing
ReceiptStorageError and exposes SqliteReceiptStorageReason:

| Cause | reason / required outcome |
| --- | --- |
| Confirmed busy BEGIN, or known rollback after busy SQL/COMMIT | busy; preserve actual not_started/rolled_back outcome |
| Underlying known-rollback SQL/constraint failure | sql_failure; preserve actual outer sql_failure |
| Underlying resource_limit / result_limit | Same named reason; preserve actual sticky outer category |
| Missing candidate retained target or absent append receipt | missing_reference after known rollback |
| Malformed selected row/history/target/allocator or inconsistent allocator | invalid_stored_data after known rollback |
| Valid selected field/history/retained document beyond configured bound | result_limit after known rollback |
| Last allocated UINT64_MAX or signed64 ordinal exhaustion | allocator_exhausted / attempt_order_exhausted after known rollback |
| New original lacks its first appended attempt at commit | incomplete_receipt after known rollback |

Known nonbusy BEGIN SQL failure uses sql_failure with not_started. Underlying
invalid_statement (an adapter/trusted-SQL defect) or capability/protocol errors
propagate their original SqliteWorkFailure instead of inventing a port reason.
Pure input, allocation and explicit stub exceptions do not claim known rollback.
Map retention's already-known invalid_stored_data/result_limit to the corresponding
port reason after its cleanup; do not call missing_reference for a malformed
stored target. Any cleanup failure/unknown durability propagates actual
SqliteWorkFailure(uncertain_outcome), overriding a local reason and never deriving
from ReceiptStorageError. Existing outer outcome/error/value rules remain intact.

The initial [source](../../server/src/sqlite_receipts.cpp) contains four explicit
SqliteReceiptsNotImplemented logic-error throws: codec encode, codec decode,
manifest and repository begin. Constructors only capture inert values/current
construction-thread identity; no actual transaction wrapper/SQL/allocation codec
or manifest implementation exists yet. The marker is distinct from known rollback
and therefore cannot falsely assert that earlier adopted-session writes vanished.
No test body/init/discovery is implied by these declarations. Actual different API/
stub/precision/harness review is required before new test authoring permission.
