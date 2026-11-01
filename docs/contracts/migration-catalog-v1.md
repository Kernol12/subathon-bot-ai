# Internal migration catalog and runner — revision 1

Task: STORAGETEST-011 / STORAGE-011. Authority: specification v0.45 §16,
DB-01/OPS-02, [slice brief](../tasks/EPIC-009/STORAGE-011.md) and
[storage decomposition](../decisions/0002-storage-executable-slices.md).
This proposes the bounded catalog mechanism before interfaces, tests or production
implementation. It retains [writer revision 1](sqlite-writer-v1.md),
[work/session revision 1](writer-transactions-v1.md), all existing signatures and
49 acceptance/regression cases. It defines no receipt/configuration/effect/job,
wire, backup, restoration or provider schema.

## Explicit operation and operational prerequisites

Opening a writer never runs migrations. A trusted installation/startup/maintenance
caller explicitly requests either inspection or advancement to the supplied
manifest's latest version. Both use the existing owner's FIFO and private writer
connection; the runner submits exactly one ordinary work item. It creates no
connection/thread, exposes no SQL/native handle, pumps no event loop and does not
split a migration chain into separately committed batches.

Before advancement, the caller must actually arrange explicit operator initiation,
the required pre-update restorable backup, compatibility/downtime information and
quiescence of normal state processing. OPS-02 forbids unattended active-event
upgrades. Installation/startup must keep normal store use closed until inspection
of the required current schema succeeds. The caller retains that exclusion through
the operation and any failure resolution; merely using the writer FIFO does not
establish an installation-wide maintenance mode.

These are trusted caller preconditions, not capabilities proved by a string,
boolean, manifest field or this runner. The slice does not implement backup,
permissions, active-event detection, release selection, application maintenance
gates or code/database rollback orchestration. Those product requirements remain
mandatory later integration gates. A diagnostic success does not authorize an
update, downgrade, restore, outbound action or automatic retry.

Conceptual operations, with concrete C++ layouts deferred to the reviewed baseline:

| Operation | Meaning |
| --- | --- |
| Inspect | Validate manifest and actual catalog/schema/integrity without DDL or catalog writes; describe fresh, supported-old-needing-update or current state |
| Advance | Validate the same facts, apply the entire missing prefix and commit its DDL/data/catalog together; an already-current store is verified without reapplying anything |
| Completion | Separate underlying transaction/work outcome, migration refusal/failure reason and optional owned report; readiness requires confirmed commit, no error and actual current-schema verification |

Admission retains owner-thread, open/shutdown, shared capacity/FIFO and rejected
callback rules of `enqueue_work`. Invalid/oversized manifests are rejected before
admission with an explicit manifest-admission reason, no SQL and no completion.
For an accepted item, inspection of fresh/supported-old/current state can commit
its read-only transaction and carry a normal owned inspection report. Only the
verified current state is a normal-store schema condition; fresh/old reports do
not claim it. Refusals and failed advancement carry no normal report/value.

The runner owns a separate bounded failure record for each admitted request,
captured through work and completion. It records a migration reason, then throws
after active rollback cleanup; terminal completion may deliver this error
information alongside the unchanged `SqliteWorkResult` without populating its
normal value. With no earlier framework error, custom refusal/cancellation maps
to `work_exception` and rolled_back/not_started as appropriate. Sticky SQL/budget/
busy errors and uncertain durability keep their underlying work categories. This
is error information, not an exception to the work contract's success-value rule.
Only terminal completion permits external publication; an internal report is not
a durable application operation result or operational authorization.

## Trusted manifest and bounded catalog identity

The integrator owns the catalog format and global migration order. One compiled,
reviewed manifest uses exact internal store identity `subathon.sqlite-store`,
catalog-format revision 1,
at least one migration, consecutive versions 1 through N (N >= 1) and the exact
supported starting versions. The starting-version list contains unique integers
in increasing order within 0 through N and must include both 0 and N; duplicates,
out-of-range values or an empty manifest/list are invalid before admission.
Version 0 means strictly fresh, without either catalog table or any schema object:
Inspect reports fresh without readiness or writes; Advance bootstraps and applies
1 through N. A catalog header with empty history is partial, never version 0.
An earlier positive prefix is supported only when listed explicitly. No downgrade or
arbitrary target selection is supplied. Schema versions are positive signed-32
engineering counters, not domain identifiers; full uint64 domain IDs/revisions
and lossless money/rate representations remain downstream requirements.

Each entry has a unique immutable migration ID of 1 through 128 printable ASCII
bytes 0x21 through 0x7e (no spaces, controls or NUL), an ordered list of trusted
SQL statements, and the exact expected schema projection after that version.
Migration entries contain unbound SQL text only, with valid UTF-8 and no embedded
NUL; catalog operations use bound metadata values. Statements retain the existing
single-statement/no-semicolon SQL boundary. Binding-bearing migration entries or
other SQL capabilities need an explicit additive contract, not an unhashed argument.
Migration SQL is compiled trusted code, never a user/provider/configuration script;
it may not alter catalog records directly, attach databases, use TEMP objects,
change connection PRAGMAs or perform external side effects. Actual release
migrations and their data-preservation rules require their own reviewed fixtures.
Respect for reserved tables and side-effect restrictions is a precondition on
trusted, audited manifests; this SQL boundary is not an adversarial SQL sandbox.

## Exact reserved metadata format

The integrator reserves these two relations in `main`, using exactly these
CREATE statements, without semicolons, IF NOT EXISTS, AUTOINCREMENT or STRICT:

```sql
CREATE TABLE subathon_catalog(singleton INTEGER PRIMARY KEY, catalog_format INTEGER NOT NULL, store_identity TEXT NOT NULL)
CREATE TABLE subathon_migration_history(version INTEGER PRIMARY KEY, migration_id TEXT NOT NULL UNIQUE, content_sha256 BLOB NOT NULL)
```

The runner binds values into its own catalog writes. It validates the actual
stored SQLite types, ranges and cardinalities explicitly, not assumed input types
or column affinity. Generated catalog reads/writes explicitly qualify `main` so
a same-named TEMP object cannot redirect them; the exact CREATE text stays as above:

- `subathon_catalog` contains exactly one row: INTEGER `singleton=1`, INTEGER
  `catalog_format=1` and TEXT `store_identity='subathon.sqlite-store'`. Read all
  bounded rows; a lookup restricted to singleton 1 cannot prove no extra header
  row exists. Missing/additional/mistyped/mismatching values are refused.
- `subathon_migration_history` contains INTEGER `version` values in 1 through
  2,147,483,647, TEXT migration IDs obeying the same nonempty printable-ASCII rule,
  and BLOB `content_sha256` values of exactly 32 bytes. IDs compare exactly,
  including case. Rows must be the complete consecutive prefix 1 through the
  actual head with unique IDs and matching digests. There is no mutable head
  counter, and MAX(version) alone is not validation.

The minimal DDL intentionally does not encode all these rules as constraints;
extra header rows, negative versions, mistyped fields and wrong digest lengths
must be detected, not coerced/repaired by the runner. Validate stored values even
when SQLite affinity or an input binding has already converted them.

Every expected versioned projection includes both exact table rows plus the
engine-generated UNIQUE index row
`('index','sqlite_autoindex_subathon_migration_history_1',
'subathon_migration_history',NULL)`. Its name and normalized table SQL must be
verified against the selected Qt/SQLite fixture before literal test freeze.
No index row is expected for the INTEGER PRIMARY KEY alone. Root pages are never
part of identity. These catalog objects may not be omitted from, changed by or
hidden behind a migration's expected projection. No domain table is chosen here.

## Canonical migration content and schema projection

History identity includes actual compiled content, not merely an ID string. Use
existing Qt Core SHA-256 over the exact framing below and compare to independently
derived literal digests in later tests. No hashing dependency is added.

Let F(b) be the ASCII decimal byte length of b, followed by `:` and the exact bytes;
decimal lengths/counters have no leading zeros (`0` represents zero). All lengths,
counts and framing-size arithmetic are checked before allocation/hashing; overflow
or an invalid value is an invalid manifest, not a truncated/wrapped frame. In this
pseudoformula `||` means byte concatenation, U means exact valid UTF-8 bytes and
A means canonical ASCII decimal:

```text
Object(type, name, tbl_name, null) =
    F(U(type)) || F(U(name)) || F(U(tbl_name)) || F("0")
Object(type, name, tbl_name, sql) =
    F(U(type)) || F(U(name)) || F(U(tbl_name)) || F("1") || F(U(sql))
Payload = F("migration-catalog-v1") || F(A(version)) || F(U(id))
       || F(A(sql_count)) || concatenation(F(U(sql_i)), in execution order)
       || F(A(object_count)) || concatenation(Object(object_j), in sorted order)
Digest = SHA256(Payload)
```

Each type/name/table-name/null marker is F-framed. NULL SQL has no following SQL
field; present empty SQL has F("1") followed by F(empty), so it differs from NULL.
Every hashed/projected text field must be valid Unicode with an exact strict UTF-8
round trip, with no embedded NUL. Reject unpaired UTF-16 surrogates or invalid,
overlong or replacement-decoded stored UTF-8; never hash repaired text. Actual
SQLite text can be inspected alongside its `CAST(... AS BLOB)` bytes and typeof
through existing SELECT, so loss in Qt decoding cannot masquerade as identity.
Auxiliary validation columns count against the same result budgets. Compare the
actual raw bytes with the strict encoding rather than accepting a lossy QString.
The complete manifest remains owned/immutable through completion; callers supply
no raw-backed strings or borrowed mutable transport data.

The expected schema projection is the sorted sequence of
`(type,name,tbl_name,sql)` from `main.sqlite_schema`, including reserved catalog
objects and SQLite-generated index rows, excluding no unexpected application
object. Root-page numbers are excluded. Object fields and SQL are compared
exactly, with null preserved. Sort both actual observations and expected objects
by unsigned UTF-8 byte-lexicographic tuples `(type,name,tbl_name,null-marker,sql)`:
compare the first differing field, put a proper byte prefix before the longer
field, use ASCII `0` for NULL and `1` for present SQL, and omit the SQL field only
for NULL. No locale, case folding, QString UTF-16 ordering or implicit SQL
collation chooses the order. Duplicate expected object tuples are invalid.
Revision 1 defines one expected projection per
version; semantic equivalence is not inferred, and no SQL normalization or repair
is attempted. SQLite itself normalizes some stored CREATE text and changes it
after ALTER; expected observations therefore require real selected-driver fixtures,
not an assumed canonicalizer. Supported release artifacts must produce their
reviewed projection. [SQLite schema table](https://www.sqlite.org/schematab.html).

## Fresh, prior, foreign and inconsistent stores

All database observations occur inside the item's one real BEGIN IMMEDIATE, so
validation and subsequent changes share the same snapshot/write exclusion.

- Fresh means the main schema has no objects and neither catalog relation exists.
  A partially initialized catalog or an unrelated table is not fresh.
- An existing store must have the exact catalog structure/header, a complete
  well-typed history prefix and matching IDs/content digests for every applied
  known version, plus the expected complete schema projection at its head.
- Unknown store identity/format, duplicate or missing versions/IDs, altered prior
  content, wrong types/digest lengths, missing/extra objects, or an unsupported
  prior head are refused. A newer head is refused as unsupported-future; no history
  is truncated to fit the current executable.
- Missing catalogs are never reconstructed from apparent tables. No `IF NOT EXISTS`
  bootstrap, dropping/recreating, rewriting checksums, automatic repair or forced
  initialization conceals inconsistency.

Refusal preserves logical schema, catalog and user data. It does not promise
byte-for-byte file/WAL identity: the existing writer's open/setup and normal
SQLite journaling already precede this runner. No file, WAL, backup or prior
installation data is deleted. Unreadable/not-SQLite input is an inspection/open
failure, not permission to create replacement state.

## Integrity through the existing writer boundary

Use fixed internal SELECT diagnostics through the existing active session:

```sql
SELECT * FROM pragma_integrity_check() LIMIT 2
SELECT * FROM pragma_foreign_key_check() LIMIT 1
```

Integrity succeeds only for exactly one row/one column containing TEXT `ok` and
zero foreign-key rows. Any violation, unexpected shape/type, prepare/fetch error
or unavailable function refuses verification; truncated error output is never
interpreted as clean. These result-bearing read-only table-valued PRAGMAs are
documented SQLite functionality; integrity_check does not cover foreign keys and
quick_check omits some checks. [SQLite PRAGMA documentation](https://www.sqlite.org/pragma.html#pragma_functions).

No writer API or execute allowlist expansion is needed. Do not substitute direct
administrative PRAGMAs, turn foreign keys off, change journal/synchronous policy,
write sqlite_schema or obtain a native handle. The TVF argument `1` must not be
used to mean an error-count limit: it can select a table named `1`. The fixed
queries above have no arguments or caller-provided identifier. They inspect main
under the existing no-ATTACH contract.
The empty parentheses are mandatory: bare `FROM pragma_integrity_check` can read
an ordinary shadow table with that name and return a fabricated `ok`. Function
call syntax must reject such a shadow as not a function; it must not fall back to
an ordinary-table query. Include this negative check in the diagnostic probe.

Run complete integrity/schema/history validation before changes and again for
the final target before commit; validate each intermediate schema projection
before advancing that version's history row. Successful checks establish the
observed database condition in this transaction, not authenticity against hostile
file editors, future correctness, backup validity or recovery authorization.

Selected-driver support must be demonstrated independently of stubbed migration
behavior. The later baseline needs a small green Qt/SQL environment probe using
the existing writer/session and fixed diagnostics, run in local and selected
official-Qt CI before new intentionally red cases. Missing TVFs, wrong query
shapes or fixture initialization cannot count as intended feature red. Current
host probes do not establish future platform or bundled-driver compatibility.

## Atomic progression, cancellation and outcomes

After prevalidation, Advance begins once, checks the current state, bootstraps
catalog metadata only if strictly fresh, then executes every missing migration in
order. DDL/data changes, each matching history append and bootstrap belong to this
single transaction. It verifies the final target and calls the session's real
synchronous COMMIT. No intermediate version becomes durable, even when several
versions were applied. An independent observer must see either the complete old
state or the complete new state, never DDL paired with an old/new partial catalog.
SQLite documents CREATE and other writes within explicit transactions; error
handling still requires confirmed rollback. [SQLite transactions](https://www.sqlite.org/lang_transaction.html).

A trusted nonblocking cancellation probe may request cooperative cancellation:
before BEGIN, between statements, after a migration's DDL before its history
append, and immediately before final commit. A request observed before commit
rolls back the whole chain. Probe exceptions follow the same failure cleanup.
Cancellation is a control request, not backup/update authority. It is not an
asynchronous SQL interrupt or a hard deadline; completed commit is never undone
because a later cancellation request arrives.

| Observation | Required result |
| --- | --- |
| Invalid manifest or cancellation before BEGIN | No transaction/effects; precise refusal/cancel reason, no readiness |
| BEGIN busy | not_started/busy, no replay; caller still holds maintenance exclusion |
| Identity/history/schema/integrity refusal | No migration effects; confirmed rollback before known-uncommitted failure, no repair/readiness |
| DDL/DML/history write, inspection, budget or cancellation fails before commit | Entire chain/bootstrap rolled back if confirmed; preserve first failure and no readiness |
| Confirmed commit | Whole target durable; readiness only after successful final checks and normal completion |
| Lost commit/rollback acknowledgment or outcome cannot be established | outcome_unknown/uncertain_outcome overrides earlier error, fail-close writer, no readiness or automatic retry |
| Work fails after confirmed commit | Preserve committed fact with work failure/no normal report; do not relabel rolled back or replay |

Underlying work error and transaction outcome are retained alongside a bounded
migration-specific reason; diagnostic error information is not a normal successful
work value. Custom refusals/cancellation must finalize active SQL nonthrowingly and
establish rollback before claiming known-uncommitted state. Existing sticky error,
uncertain outcome, revoked-capability, ordered failed drain, completion exception,
shutdown and lifetime guarantees remain. Lost acknowledgments use the already
reviewed physical-control fault seam; they must not be simulated by fabricating a
catalog result. A later independent inspection can observe old or new state but
does not itself authorize an operational recovery/retry.

Default engineering manifest bounds: at most 256 versions, 128 ASCII bytes per
identity/ID, 256 schema objects per projection, and 1 MiB total manifest SQL and
projection UTF-8 payload. Limits are positive trusted configuration, not product
settings or proof of elapsed-time bounds. Invalid duplicate/gapped manifests and
oversized data are rejected before SQL. Metadata/history/schema reads are bounded
with overflow detection; excess rows/bytes refuse verification, never truncate to
a claimed valid prefix.

Every inspection, catalog read/write and migration statement consumes the existing
session's cumulative statement/binding/result budgets, including fixed integrity
queries. BEGIN/COMMIT/ROLLBACK retain their existing control exemption. No hidden
budget increase or transaction fragmentation makes an oversized plan succeed.
Expected framework overhead must be accounted for in the authored baseline. A
manifest within its own bounds can still exceed the caller's session limits and
must then fail safely. Full database scans/trusted SQL can take time; no hard SQL,
total-memory, shutdown deadline or arbitrary callback bound is claimed.

## Literal future acceptance examples

These are manually specified future oracles, not authored tests or observed red.
Use synthetic tables, real disposable Qt SQL files, independently built catalog
fixtures and independent observers. Later tests must first fix physical catalog
DDL/names and derive literal canonical-byte/digest vectors with a separate oracle.

| ID | Sequence and required observable result |
| --- | --- |
| MC01 | Strictly empty store; migration 1 creates `sample(id INTEGER PRIMARY KEY, amount INTEGER)` and inserts `(1,10)`; migration 2 adds `(2,20)`. One commit yields exact history 1/2 and sum 30, including bootstrap; no observer sees intermediate history |
| MC02 | Supported version-1 fixture has literal row `(1,10)` and its reviewed catalog/schema. Advancing applies only version 2; row 1 remains 10, sum 30, exactly two history rows. Reinspection/current advance does not execute either migration again |
| MC03 | Inspect supported version 1 against target 2: reports update needed, no readiness/DDL/history/data change. Inspect current 2 validates actual identity/schema/integrity before reporting ready |
| MC04 | No catalog plus unrelated `sentinel(7,70)`, header-only/history-only/empty-history partial bootstrap or wrong store identity: refuse without creating/repairing catalog or changing sentinel |
| MC05 | Valid catalog format with newer head 3 against target 2, or explicitly unsupported old head: refuse without dropping row/history/schema or rewriting version |
| MC06 | Empty manifest, duplicate/gapped/out-of-order versions, invalid starting-version list/IDs, invalid UTF-8 or length overflow and manifest bounds: reject before SQL and completion execution; exact configured bounds accepted |
| MC07 | Matching highest version but altered earlier SQL digest/ID, missing version 1, duplicate/mistyped history or missing/extra schema object: refuse; catalog head alone cannot pass |
| MC08 | Valid identity/history/schema but `CHECK(n>=0)` fixture contains literal `n=-1` inserted with checks disabled by fixture setup. Real integrity returns non-ok; runner refuses without repairing the row |
| MC09 | Structurally valid parent/child fixture contains child `(1,404)` with no parent, inserted with foreign keys off only in fixture setup. Integrity can return ok, FK check returns a violation; runner must refuse and preserve the row |
| MC10 | From supported 1, migration 2 creates a new table/writes `(2,20)`, then an actual duplicate-key/DDL error occurs. After synchronous failure and reopen, old row10/history1 remain, new table/row/history2 are absent; independent BEGIN IMMEDIATE proves rollback released the lock |
| MC11 | From fresh, cancel after successful DDL before first history append; or from version 1 cancel during version 2/before commit. No bootstrap/partial version survives; old state remains, no success report or automatic replay |
| MC12 | External BEGIN IMMEDIATE holder remains locked until busy completion. No DDL/history/body replay occurs after release; a newly explicitly initiated operation can succeed |
| MC13 | Lose acknowledgment after physical final COMMIT: observer sees complete target sum30/history1/2, result remains unknown/fail-closed, queued old/new work is not executed, no readiness or retry |
| MC14 | First migration SQL failure triggers rollback whose acknowledgment is lost: uncertainty overrides SQL error, no known-rollback claim; queued work fails in order and connection/admission close |
| MC15 | Cross a cumulative statement/result/byte limit after prior DDL; rollback removes all new DDL/catalog changes. Exact-boundary run succeeds with framework diagnostic/catalog overhead included |
| MC16 | Fixed diagnostic TVF unavailable, ordinary shadow table named `pragma_integrity_check` containing `ok` or empty `pragma_foreign_key_check` shadow, malformed result, observer/catalog row limit exceeded or unparseable database: fail inspection without a clean/readiness assertion; green environment probe failure is not feature red |

Physical process kill, disk/power loss, backup/restore, installation updates,
code/schema rollback and actual supported release-platform verification require
separate evidence. Default tests must not exercise live credentials/providers or
initiate an update/backup. All seven integrations, three rate providers and the
full migration/lossless/provenance/application scope of the downstream storage
packages remain required. Contract review precedes minimal interfaces/stubs/tests;
independent actual test review and intended red precede STORAGE-011 production.
