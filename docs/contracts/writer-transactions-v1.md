# Writer-owned repository transaction work — revision 1

Task: CONTRACT-004. Authority: specification v0.45 §2, §3.4, DB-01 and SERVER-01;
[event/state boundaries revision 1](event-state-boundaries-v1.md).
This is an additive internal contract for the future STORAGETEST-010/STORAGE-010
slice. It defines observable operations and outcomes, not final C++ layouts, SQL
schemas or an implemented capability. Existing [writer revision 1](sqlite-writer-v1.md),
its 22 acceptance cases and three ownership regressions remain unchanged.

## Why a second work form is needed

The existing writer executes a prebuilt SQL batch atomically. The existing
[receipt](../../server/include/subathon/server/receipt_intake.hpp) and
[contribution](../../server/include/subathon/server/contribution_evidence.hpp)
repository ports instead synchronously read, decide, write and commit. In
particular, `ReceiptIntake::ingest` returns only after its transaction commits.
Implementing each port call as a separate queued batch would lose that transaction;
waiting on those batches by pumping an event loop would permit reentrant state
processing. Opening another writer connection would violate DB-01.

The new form queues trusted in-process work on the same authoritative Qt owner,
using the same private connection and admission queue. During that work, a scoped
capability permits synchronous repository operations. `commit()` performs and
confirms the real SQL COMMIT before returning. There is no deferred outer commit,
commit vote or second connection hidden behind the synchronous port.

## Conceptual operations and ownership

These operation names describe semantics for the later public C++ test boundary;
they do not prescribe templates, exception class names or object layout.

| Operation | Required meaning |
| --- | --- |
| `enqueue_work(work, completion)` | Admit one trusted callable and its completion, asynchronously; return only admission status |
| `context.begin()` | Make this work's single transaction attempt using real BEGIN IMMEDIATE; return a usable session only after BEGIN succeeds |
| `session.execute(statement)` | Prepare/execute one permitted statement synchronously on the private writer connection and return owned rows; later calls can use earlier results |
| `session.commit()` | Confirm the real COMMIT synchronously, terminally close this session and return success only for known committed outcome |
| `session.rollback()` | Confirm real rollback synchronously and terminally close this session; expose no successful work result |
| work completion | Receive the final transaction outcome, a separate work error, and an owned logical result only when work returned normally after confirmed commit |

The writer never creates another state-processing thread. Construct, open, execute
and destroy it on the authoritative Qt command/state thread. Drogon/connector
threads marshal owned commands to that owner; foreign submission is rejected before
SQL access, as for existing batches. No database, driver, query or native SQLite
handle escapes. A repository adapter binds to the current work capability; it
cannot privately open a connection or start an independent writer.

Repository `begin()` maps to `context.begin()`. Its primitive methods map to the
active session, and its `commit()` maps to the real `session.commit()`. Existing
ReceiptTransaction/ContributionTransaction destruction without successful commit
must roll back while still on the owner. Their existing known-uncommitted storage
exceptions remain meaningful; an uncertain durable outcome must not be disguised
as one of those known-rollback exceptions.

Transaction-wrapper destruction and owner finalization are nonthrowing cleanup.
They inspect recorded phase without SQL: committed, already rolled-back or expired
sessions are cleanup no-ops. Only a live active owner transaction needs rollback.
Cleanup failure records outcome_unknown for the executor and fail-closes it; it
must not throw from a destructor or replace an earlier error with a redundant
ROLLBACK attempt. Explicit misuse of a terminal session operation remains an error.

The new work form is internal trusted code, not a SQL/callback endpoint for clients,
providers, user scripts or configuration documents. It grants neither permission
nor contribution eligibility. Those checks remain application responsibilities.

## Admission, FIFO and lifetime

Old batches and new work share one FIFO in admission order. At most one item is
executing. An item stays active until its work body returns and transaction
finalization finishes, even if it committed earlier. The next item's body/SQL cannot
run in the gap between this body's commit and return. There is one queued dispatch
per item; synchronous SQL methods never pump Qt events or wait on another queued
item. Work must not create a nested event loop, block on network/delivery, sleep or
wait for a later item.

The outstanding-item capacity is the existing `pending_batches` value (default64):
active batch/work plus queued batch/work items. With only old batches this retains
the old behavior. A null work or completion returns `invalid_work`; a closed writer
returns `not_open`, full capacity `queue_full`, foreign caller `wrong_thread`, and
terminal shutdown/fail-closed state `shutting_down`. Rejected admission runs neither
callable nor completion and executes no SQL. No implicit retry occurs.

A work body may enqueue a later item if capacity permits, but that item cannot run
until this body ends. Its completion runs on the owner after the item leaves the
pending count, so a completion can enqueue at capacity1. External completion
exceptions are contained and cannot undo a transaction or stop later healthy work.
The implementation must not access destroyed writer state after a completion that
destroys the writer, consistent with the old completion boundary.

The caller keeps the writer and owner event loop alive through the body and entry
to its completion; the body must not destroy it or release its last owner. A
completion may destroy the writer after the terminal outcome is fixed. Graceful
shutdown additionally keeps it alive through every completion. Early destruction
is not graceful shutdown and does not promise delivery of queued completions.

### A revocable session capability, not a borrowed SQL handle

Each admitted work has a distinct identity/generation. Work-context and session
capabilities may be retained as inert values, but retaining one never extends its
validity or keeps the connection alive. Every operation checks owner thread,
writer lifetime, matching
active work and transaction phase before touching SQL. After commit, rollback,
body exit, fail-close or writer destruction, a retained capability returns a local
`session_closed`/`stale_session` error and cannot affect a later work's transaction.
Calling a dangling C++ reference is not made safe by this contract; the eventual
interface must provide an ownable, revocable capability for retained-value tests.

Foreign capability calls reject as `wrong_thread` without SQL or mutation of the
owner's transaction. An expired capability likewise cannot poison another active
item. Repository transaction wrappers themselves remain owner-thread objects.

## One transaction attempt and synchronous terminal outcomes

The executor does not BEGIN before invoking work: a service may reject invalid
domain input before calling repository `begin()`. The first `begin()` attempts real
BEGIN IMMEDIATE, with the writer's busy timeout0. A busy/failed BEGIN starts no
session and is never retried. Any second begin attempt within that work is a
protocol error, even after a failed BEGIN or successful commit. Savepoints/nested
transactions and multiple transactions per item are outside revision1.

An active session supports repeated read/decision/write calls in one transaction.
Results returned to trusted work are speculative until commit. No transport reply,
provider acknowledgment, published state update or external delivery may claim
success from those rows. The service may use them internally to make its decision.
Only terminal work completion authorizes the caller to publish the logical result.

`commit()` has two observable facts: it returns only after confirmed SQL COMMIT,
and no later framework action commits or rolls back that already committed unit.
Thus a synchronous service can return its result to its enclosing work after real
commit, while the enclosing work still cannot publish it externally before completion.
No SQL operation or second transaction is allowed through that session afterward.

If work returns without a successful commit, the framework finalizes it without a
success payload: an active transaction is rolled back; an explicitly rolled-back
transaction stays rolled back; no begun transaction stays not_started. Failure to
establish known rollback becomes outcome_unknown and fail-closes the writer.
Adapters must not rely solely on wrapper destruction: the owner finalizer checks
the transaction even when trusted code retains or abandons a wrapper.

### Errors are sticky within a live owner session

An owner-session SQL error, prohibited statement, exhausted budget or illegal
transaction operation prevents later commit. The framework synchronously rolls
back before exposing a known-uncommitted failure to the repository port; if it
cannot establish that fact, it exposes a distinct uncertain-outcome failure.
Catching a framework session error inside work cannot make the item successful or
allow another BEGIN. No partial row result from a failed statement escapes.

The first framework failure is retained even if work catches its exception and
causes another error. An uncertain transaction outcome always takes precedence;
otherwise the first framework failure takes precedence over a later body exception.
With no framework failure, an uncaught body exception is `work_exception`; with no
exception but no confirmed commit, use the matrix's missing/uncommitted category.

An uncaught work exception before commit causes rollback and discards the logical
result. An exception after confirmed commit cannot undo it: completion says
committed with `work_exception`, has no normal success payload and is not a reason
to retry. Similarly, a prohibited second transaction after commit reports a
committed/protocol-error outcome; it never relabels committed changes rolled back.
Foreign/stale rejected calls are not sticky errors in a different owner session.

### Completion outcome matrix

The transaction outcome and work error are separate fields; error spelling is
conceptual, while these distinctions are contractual.

| Observation | Transaction outcome | Work error / logical result | Writer afterward |
| --- | --- | --- | --- |
| Body begins, commits, then returns normally | committed | none / owned returned result | Healthy |
| Domain validation throws before BEGIN | not_started | work_exception / absent | Healthy |
| BEGIN is busy | not_started | busy / absent | Healthy; no replay |
| Body returns without BEGIN | not_started | missing_transaction / absent | Healthy |
| Body explicitly rolls back, then returns | rolled_back | not_committed / absent | Healthy |
| Body returns or throws with active transaction | rolled_back if confirmed | uncommitted or work_exception / absent | Healthy only after known rollback |
| Statement/constraint/budget error, known rollback | rolled_back | original SQL/limit error / absent | Healthy |
| COMMIT fails and no commit plus rollback are established | rolled_back | commit error (including busy) / absent | Healthy |
| Confirmed COMMIT, then body throws | committed | work_exception / absent | Healthy; never automatically repeat |
| Illegal second begin/terminal-session use in same body | Preserve committed/not_started outcome, otherwise known rollback | protocol error unless an earlier framework error exists / absent | Healthy if outcome known |
| Commit/rollback outcome cannot be established | outcome_unknown | uncertain_outcome / absent | Fail-closed, terminal |
| Item queued behind fail-closed work, never invoked | not_started | writer_unavailable / absent | Terminal; no replay |

Known-uncommitted repository error means no effect from that transaction became
durable. Merely observing a COMMIT error is not universal proof of that assertion.
The implementation must establish the outcome or use outcome_unknown; an injected
lost acknowledgment after physical commit must never be mapped to rolled_back.
An uncertain outcome closes admission and the connection on its owner, revokes all
session capabilities and prevents this writer object reopening automatically.
Already admitted items that have not started receive ordered not_started failures;
they are not dropped or executed on a replacement connection. Old queued batches
receive their existing error/no-result completion without executing SQL.

Operation/reversal deduplication, a durable outcome-resolution record, recovery
holds and deciding whether a new writer may resume belong to later application/
storage contracts. Releasing the process-local path lease does not authorize the
application to ignore an unresolved transaction. This contract promises no remote
exactly-once delivery or power-loss outcome proof.

## Trusted inputs and bounded SQL work

Moving/copying a callable owns the callable object, not arbitrary pointees captured
by it. Before admission the caller supplies trusted code whose request data remains
owned and immutable until the body ends: use owned request values, not references,
views, raw-buffer Qt values or transport objects that can expire/change. The caller
also supplies an owning logical result and keeps any completion dependencies alive.
The writer cannot inspect arbitrary captures or measure all closure/result heap
allocation. Future typed application adapters must enforce their own request bounds.

This precondition does not weaken old `enqueue(batch,completion)`: that path still
deep-owns accepted SQL and QString/QByteArray bindings, including raw-backed values,
and its original25 tests remain required. For the new path, every synchronous SQL
call snapshots its statement/bindings for execution and returns owned result rows;
it retains no caller buffer or Qt SQL query after that call. A returned row remains
valid after session/writer closure. Application callbacks remain trusted code.

Each work uses the writer limits captured on admission. Counts are cumulative
across that work's SQL calls, not reset for each repository method:

| Resource | Existing default / rule |
| --- | --- |
| Outstanding items | 64 active-plus-queued old/new items |
| SQL statement attempts | 64; count each session execute before preparation |
| SQL text bytes per statement | 65536 UTF-8 bytes |
| Total bound value budget | 1048576; each value charges8 plus QString UTF-8/QByteArray payload |
| Total result rows | 1024 across all calls |
| Total result value budget | 1048576 with the same per-value charge |

BEGIN/COMMIT/ROLLBACK are framework control operations, outside the SQL statement
count; revision1 permits only the one begin attempt and its terminal resolution.
Before SQL, validate text, bindings and remaining budgets. Exceeding a limit within
an active transaction rolls it back; earlier speculative rows are not published as
a successful work result. Previously returned internal rows cannot be retracted,
which is why publication before terminal completion is prohibited.

Reuse the old supported scalar/null/QString/QByteArray types and trusted SQL
allowlist: SELECT/WITH/INSERT/UPDATE/DELETE/REPLACE/CREATE/DROP/ALTER and read-only
journal_mode/synchronous/foreign_keys/busy_timeout diagnostics. Preserve the
semicolon prohibition. SQL transaction control, ATTACH/DETACH, VACUUM, mutating or
other PRAGMAs are rejected; only session methods own transaction control. Future
migration/integrity capabilities need their own additive reviewed contract.

These are SQL payload/count/result budgets and zero lock waiting, not a hard
arbitrary-SQL execution deadline or total-memory bound. Trusted work must terminate
and avoid blocking operations. Application backpressure and process shutdown
deadlines remain API-001/integration gates.

## Shutdown and compatibility

Shutdown immediately rejects new old/new admissions, lets the current trusted body
finish, drains every accepted item in FIFO order, closes/removes the connection and
then calls shutdown completions on the owner. A work may request shutdown while
active and still finish its already admitted transaction. Nested work submitted
after shutdown is rejected. Fail-closed drain instead completes queued items as
not_started/errors; it never resumes them. Repeated/empty-queue shutdown and the
old null-completion/open-after-shutdown rules remain as in writer revision1.

This is additive: existing batch/read-only interfaces, SQL restrictions, same-thread
connection ownership, WAL/FULL/foreign_keys/busy_timeout0, raw-buffer batch ownership,
canonical single-writer lease and old25 acceptance/regression assertions remain.
No migration/domain row shape, uint64 ID encoding, money/rate serialization, wire
schema, provider behavior or actual repository adapter is finalized here.

## Independently derived examples and STORAGETEST-010 map

The following are future test oracles, not authored tests or observed results.
Use real Qt SQL with disposable files, literal rows and independent read connections;
inject classified faults only through a reviewed harness. Keep old25 assertions.

| ID / sequence | Required observable oracle |
| --- | --- |
| WT01: old batch inserts10; work reads10, writes20 and commits; later batch reads sum | FIFO completion1,2,3; final literal sum30; no next item starts inside work2 |
| WT02: work inserts row1, reads it, conditionally inserts row2 | Same session sees its row1; external reader sees neither before commit, both afterward |
| WT03: a primitive repository calls commit inside work | An independent connection sees both rows immediately after commit returns, before the body returns; no later outer commit occurs |
| WT04: body returns or throws after writes without commit | Neither row persists; rolled_back outcome/no success payload; following healthy work succeeds |
| WT05: later statement fails after an earlier insert | Whole transaction rolls back; catching the session error cannot commit or begin again |
| WT06: external write lock stays held during first begin | Busy/not_started completes without releasing that lock; lock release alone never replays the item |
| WT07: deferred constraint makes COMMIT fail with known rollback | No child row persists; commit never returns success; next independent item can commit |
| WT08: real commit is followed by a body exception | Row persists exactly once; committed/work_exception, no normal result and no automatic replay |
| WT09: controlled fault loses outcome acknowledgment after physical commit | outcome_unknown, no rolled_back claim; connection/admission close; queued items not_started and no replay |
| WT10: rollback cannot establish a known outcome | Same terminal fail-close; no successful completion or new connection takeover |
| WT11: retain a session then use it after return/commit, during another work or after writer destruction | stale/closed error before SQL; it never modifies the later transaction or extends connection lifetime |
| WT12: foreign thread invokes capability operations | wrong_thread/no SQL; legitimate owner session remains usable; no foreign connection cleanup |
| WT13: nested/second begin or explicit SQL COMMIT | Rejected without nested SQL transaction; before commit rolls back, after commit preserves committed fact with protocol error |
| WT14: cumulative statement/binding/result limits cross a boundary on a later call | Whole work rolls back; no successful partial payload; exact-boundary work is accepted |
| WT15: capacity2 contains one active and one queued item | Third admission queue_full; completion can enqueue after its slot is freed; no hidden retry |
| WT16: work requests shutdown then commits | Accepted work commits, new admission rejected, prior queued items drain, connection closes before shutdown completion |
| WT17: an external completion throws or destroys its writer after a completed item | No event-loop exception/invalid implementation access; already fixed transaction is unchanged; graceful lifetime preconditions remain explicit |
| WT18: queued trusted work owns a copied request and returns rows/result | Caller may discard its original owned request; returned values survive session close; no claim that arbitrary borrowed captures were normalized |
| WT19: same synchronous service storage port receives an uncertain commit failure | Its commit does not return success or throw the known-rollback storage-error category; outer completion remains outcome_unknown |
| WT20: normal body returns a value without ever opening a transaction | not_started/missing_transaction; no logical success value and no provider/application acknowledgment authorized |

STORAGETEST-010 must author these bounded assertions against a versioned minimal
interface, obtain a different reviewer's approval, then observe intended failures
from explicit missing behavior. Only then may STORAGE-010 implement it with separate
code review and real passing acceptance. Deterministic fault injection proves the
specified classification path, not real disk/power-failure or provider support.

## Evidence and CI boundary

CONTRACT-004 changes documentation only. Project/link/progress/tooling checks apply;
it adds no executable target, schema, dependency or workflow. Existing stable writer
and domain CI must remain green, but does not implement these new guarantees.
The storage split must preserve the original STORAGE-002 requirement scope and
actual final baseline, with no circular dependency through ACCT-004 and no invented
aggregate-red result. Real migration/repository/application/recovery/live/platform
gates, all seven integrations and all three rate providers remain release scope.
