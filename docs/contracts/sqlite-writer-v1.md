# Qt SQL single writer — revision 1

Task: STORAGETEST-001 / STORAGE-001. Authority: specification v0.45 §2, DB-01,
and [event/state boundaries revision 1](event-state-boundaries-v1.md).
This internal infrastructure boundary does not finalize an application schema,
repository adapter, migration, accounting transaction or external API.

## Thread and connection ownership

Construct `SqliteWriter` on the authoritative Qt command/state thread. It owns a
Qt SQL SQLite connection and schedules batches through that thread's existing
Qt event loop. It never creates another state-processing thread. Drogon/connector
threads must marshal commands to this owner; directly calling its methods from a
foreign thread returns `wrong_thread` before inspecting a Qt SQL handle. Construction,
destruction and QObject lifetime management remain owner-thread responsibilities.

`open` accepts a file-backed local database path, configures and verifies WAL,
`synchronous=FULL`, foreign keys ON and SQLite busy timeout zero. It returns success
only after all setup succeeds. Empty paths, `:memory:` and nonpositive limits are
`invalid_options`; unavailable files/drivers or failed PRAGMA setup are `open_failed`.
A failed open closes/removes any partial connection and retains no writer lease.
An already open object returns `writer_already_open`. Exactly one writer object
may own a canonical file path in this application process, including equivalent
relative/absolute paths; a second returns `writer_already_open` until shutdown.
This is process-local ownership, not a claim that other processes cannot open SQLite.

`SqliteReader` opens an existing file with `QSQLITE_OPEN_READONLY` and query_only ON,
and has its own connection on its construction thread; open must use that same
thread. Repeated open without close returns `writer_already_open` (the shared
connection-already-open error), and successful close permits reopening. Empty or
`:memory:` paths are `invalid_options`; unavailable/nonexistent files are
`open_failed` without creating a file, and read before open is `not_open`.
Read/close calls on
other threads return `wrong_thread`. All connection/query handles are private and
closed/removed on their owner, after active query handles have been destroyed.
Reader destruction is also an owner-thread responsibility. A read-only flag is
required because query_only can be changed; disabling it cannot permit database
writes. An external test connection may hold a read snapshot or write lock to
exercise SQLite; it is test infrastructure, never a product second state writer.

## Admission and bounded trusted SQL

`enqueue` copies a nonempty batch plus completion into the owner queue and returns
`none` on admission, before running SQL. A missing completion or invalid/oversized
batch returns `invalid_batch` without invoking the completion. A closed writer is
`not_open`. Shutdown takes precedence over later admission (`shutting_down`).
The `pending_batches` limit includes the currently executing batch until its
transaction has committed/rolled back, plus waiting batches. A full queue returns
`queue_full`; rejected work is never implicitly retried. Accepted work runs FIFO,
one complete batch per queued dispatch, on the owner thread.

Limits are positive internal engineering defaults, configurable by trusted callers;
they are not product contribution/input limits or a future user-facing setting:

| Resource | Default | Count |
| --- | --- | --- |
| Outstanding batches | 64 | Active transaction plus admitted waiting batches |
| Statements per batch | 64 | Each prepared statement |
| UTF-8 SQL bytes per statement | 65536 | Full original SQL text |
| Bound value bytes per batch | 1048576 | Each value counts 8, plus QString UTF-8 or QByteArray payload bytes; this includes empty/null values |
| Result rows per batch | 1024 | Sum across all statement results |
| Result value bytes per batch | 1048576 | Same value accounting as bound values |

Bound variants accept null, QString, QByteArray, bool, signed/unsigned integer and
double only. Other QVariant types are `invalid_batch`. Exact money/accounting
representation is the caller's responsibility; this infrastructure does not select
REAL for lossless decimals. SQL text is trusted internal code, with positional
bindings, not a transport endpoint. Each statement is prepared/executed separately.
SQL text containing any semicolon is rejected before admission, including a
semicolon in a SQL literal/comment; use a positional binding when data contains
semicolons. This narrow rule avoids using a partial SQL parser as an ownership
boundary. The permitted leading SQL commands
(case-insensitive after whitespace) are SELECT, WITH, INSERT, UPDATE, DELETE,
REPLACE, CREATE, DROP and ALTER. Leading SQL comments are intentionally outside this
small interface. Read-only `PRAGMA journal_mode`, `PRAGMA synchronous`,
`PRAGMA foreign_keys` and `PRAGMA busy_timeout` are also supported for actual
connection observations, with
no arguments/assignment/schema prefix. All other PRAGMAs, BEGIN/END/COMMIT/ROLLBACK,
SAVEPOINT/RELEASE, ATTACH/DETACH and VACUUM are `invalid_batch` before any work in
that batch runs. This keeps transaction/connection ownership inside the writer.
Schema definition SQL remains trusted, and does not grant a migration policy.

The value charge is a deterministic admission budget, not a claim about QVariant's
heap layout. The nonzero base charge also bounds empty/null binding counts.
The limits bound admitted payloads, statement/result counts and SQLite lock waiting;
they do not establish a hard execution deadline for arbitrary trusted SQL. Query
complexity, application command budgets, cancellation/interruption and performance
remain later integration/release work. Do not run blocking/network work in this queue.

## Transaction and error ownership

Each admitted batch uses one `BEGIN IMMEDIATE`, ordered statements and one COMMIT.
Result sets contain owned QVariant rows in statement order, including empty sets for
non-query statements. No result is delivered before commit. A successful completion
returns `none` plus exactly one result set per statement; it runs once on the owner
thread and can submit subsequent work after its batch has left the pending count.

A BEGIN/statement/COMMIT lock failure is `busy`, with no hidden retry or replay.
Any other SQL/constraint failure is `sql_failure`. Any result bound exceeded is
`result_limit`. Errors roll back the entire batch; no partial result sets are exposed.
After known rollback, later admitted work can continue. A failure to establish a
known rollback closes admission and the connection; recovery/indeterminate durable
commit resolution is not supplied by this low-level helper and must remain an
explicit STORAGE-002/application gate. Completion exceptions must not escape the
Qt event loop or undo an already committed batch; callers should use nonthrowing
completion handlers. This baseline does not claim crash-safe exactly-once delivery.

`SqliteReader::read` returns owned rows for one prepared statement with positional
bindings and a positive maximum row count (default 1024). Excess rows produce
`result_limit`, SQL errors `sql_failure` (locks `busy`), and no partial rows escape.
It never grants the writer connection. Its actual SQLite read-only flag protects
persistent state even if trusted diagnostic SQL changes connection-local query_only.

## Graceful shutdown

Owner-thread `shutdown` immediately closes admission, drains every already-admitted
batch in FIFO order, then closes/removes its database connection, releases the writer
lease and invokes shutdown completion(s) once on the owner. Empty-queue shutdown
still completes asynchronously. Repeated shutdown calls are safe and each completion
runs; no second transaction/result is created. A null shutdown completion returns
`invalid_options` without changing admission. Shutdown before open is safe,
asynchronous and terminal; after shutdown starts this writer object's open/enqueue
return `shutting_down`, even after drain. A new writer object can acquire the released
path lease. No admitted work is silently dropped.
The QObject and Qt event loop must remain alive until drain completes. Destruction
before graceful shutdown may discard queued work and is not a graceful operation.
Application lifecycle coordination and a process shutdown deadline remain API-001.

## Independently derived examples and baseline boundary

- Insert literal rows `(1,10)` and `(2,20)` in one batch: completion sees both, sum30.
  A later duplicate primary-key insert after `(3,30)` rolls back row3 as well.
- An external `BEGIN IMMEDIATE` holder causes admitted insert `(4,40)` to report
  busy. Releasing that lock does not replay row4. A new `(5,50)` batch succeeds.
- With capacity2, two batches admitted before dispatch fill the queue. A completion
  callback may enqueue after its committed batch frees that slot. The active-batch
  count invariant additionally requires code review; this owner-thread interface
  has no test-only reentrant hook inside synchronous SQL execution.
- A WAL reader holding a preexisting snapshot still sees sum10 while the writer
  commits another row; after reader transaction end, a fresh read sees sum30.
- A deferred foreign-key error at COMMIT leaves no inserted child row. A subsequent
  valid parent/child batch succeeds, demonstrating transaction reset.
- Shutdown after two accepted batches rejects a third, commits the first two,
  then removes only the writer connection and releases its path lease.

Tests use disposable files and real Qt SQL, with literal SQL observations and
deterministic event/callback synchronization. They establish neither a final schema
nor crash/power-loss durability, backup/recovery, migrations, app-level atomicity,
actual deployments, provider behavior or full server integration. Intended red
comes only after independent review of the actual authored revision. STORAGE-001
implementation remains blocked until that reviewed red is recorded.
