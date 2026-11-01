# STORAGETEST-011 — migration contract authoring

Author: `/root/writer_transaction_reviewer`, explicitly reassigned as author,
2026-11-01. Independent contract/test reviewer: `/root/storage_work_code_reviewer`;
this author cannot approve the new contract or eventual tests.
Branch `task/STORAGETEST-011-author`, isolated worktree
`local-evidence/worktrees/storagetest011-author`, from claim
`9dba71dea7bd1cdeb14e8d7655f3afff1103a3db` after merged main
`88e86c12b3c0e089461f1df2eb970ed0a4b05c68` / PR 30.

## Scope and concrete decisions

Initial authorization is documentation only. This author owns only
[migration-catalog revision 1](../contracts/migration-catalog-v1.md) and this note.
Root owns shared schemas/migration order, build/workflow, state/HTML and checkpoint.
Read root/server/tests AGENTS, the slice brief, specification §16/DB-01/OPS-02,
ADR 0002, writer/work contracts, actual source
`023451987e40a9330fef91c6c0955f0e527f14e6`, its 49 existing cases and the latest
STORAGE-010 implementation/handoff evidence. Their completed results and reviews
are preserved; no previous red run is repeated.

The contract proposes these bounded internal decisions for separate review:

- One explicit inspection/advance request uses one existing work item and one real
  BEGIN IMMEDIATE. The entire missing prefix, bootstrap, DDL/data and catalog
  advancement commit together. Writer open never performs an upgrade.
- A trusted compiled manifest supplies consecutive immutable migrations, explicitly
  supported starting versions and exact expected schema projections. A logical
  singleton identity/format header and append-only version/ID/SHA-256 history are
  checked in full, not inferred from a maximum version. The integrator then fixed
  exact `main.subathon_catalog` / `main.subathon_migration_history` DDL in the
  documentation revision below, before minimal interface/test authoring.
- Content digests use specified length-prefixed UTF-8 framing, including exact SQL
  and expected schema projection; migration SQL has no unhashed bindings. Existing
  Qt Core SHA-256 suffices. Literal independent byte/digest vectors remain a test
  prerequisite. These internal counters do not narrow domain uint64 identifiers.
- Exact complete schema observations and separate full integrity/foreign-key
  diagnostics reject foreign/partial/changed/future state without repair or data
  loss. Conservative identity matching does not claim semantic SQL equivalence
  or hostile-file authenticity.
- Fixed read-only diagnostic TVF SELECTs use the existing private active session.
  No writer/header/allowlist/handle/thread change is needed. Strict result shapes,
  bounded output and unsupported-query refusal prevent unverified readiness.
- Explicit update initiation, quiescence, pre-update restorable backup and
  compatibility/downtime preparation are actual trusted caller prerequisites.
  No invented boolean/string proves them; operational orchestration remains open.
- Cooperative cancellation, known rollback and uncertain acknowledgment outcomes
  retain existing work semantics. No partial prefix, hidden retry, fabricated fault
  result, automatic repair or power-loss recovery claim is introduced.

Eighteen literal future examples cover fresh/prior/current inspection, foreign and
partial catalogs, duplicate/gapped/mismatching history, unsupported versions,
actual CHECK/FK violations, DDL failure and rollback lock release, cancellation,
busy, physical commit/rollback acknowledgment loss, exact resources and diagnostic
availability. These are proposed examples, not authored executable tests or red.

## Technical evidence and limits

Primary documentation inspected on 2026-11-01:

- [SQLite PRAGMA documentation](https://www.sqlite.org/pragma.html#pragma_functions):
  read-only result-bearing PRAGMAs have table-valued SELECT forms. Full integrity
  and foreign-key checks are separate. Unknown direct PRAGMAs may be ignored, so
  query support/result shape cannot be presumed.
- [SQLite transaction documentation](https://www.sqlite.org/lang_transaction.html):
  explicit transaction control covers CREATE and other writes; some error paths
  need explicit outcome resolution rather than assuming rollback.
- [SQLite schema table](https://www.sqlite.org/schematab.html): stored CREATE text
  receives SQLite normalization and can change after ALTER. Exact fixtures must
  inspect actual selected-driver results.
- [Qt QSqlDatabase](https://doc.qt.io/qt-6/qsqldatabase.html) and
  [QSqlQuery](https://doc.qt.io/qt-6/qsqlquery.html): connection/query ownership
  remains on the existing owner. No raw-handle workaround or new dependency.

An ephemeral Python `sqlite3`/`tempfile` probe ran against host SQLite 3.54.0.
Its first invocation failed with `no such table: main.1` when using
`pragma_integrity_check(1,'main')`; the integer is not the direct PRAGMA error-count
argument in this TVF form. The corrected probe (exit 0) observed no-argument full
integrity `[("ok",)]`, zero FK rows, transactional CREATE/INSERT removed by
ROLLBACK, and a literal child `(1,404)` with missing parent that still returned
integrity ok while FK check returned `("children",1,"parents",0)`.
The disposable directory was automatically removed. No repository code/schema/test
file was written by the probe, and neither result is acceptance or feature red.

The coordinator separately reported actual host Qt 6.12/existing-writer SELECT
probes with SQLite 3.54.0. This author does not recast that report as an independent
Qt run. Current-host SQLite facts do not prove selected official-Qt CI or platform
support. After contract review, the baseline needs a separate green Qt/SQL
diagnostic environment probe, before any known-red migration cases; root owns its
shared harness/CI integration. Missing TVFs or fixture errors cannot establish red.

## Validation and remaining gates

Checks actually run in this isolated author worktree:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit 0; 391 records, 122 decisions, local links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit 0; existing root-owned snapshot current |
| `git diff --check` | Exit 0 |
| `git status --short` | Only the two owned Markdown paths untracked before commit |
| `git diff 9dba71d -- server tests schemas CMakeLists.txt CMakePresets.json .github` | Empty |

No new C++, schema, migration, executable test, build target or workflow was authored.
No native suite, Qt probe, migration behavior, feature red, live provider or release
platform run is claimed in this authoring phase.

Different-agent review of the actual contract comes first. Only after approval
may root expand ownership to minimal versioned interfaces/stubs/tests using the
fixed metadata DDL. Those actual tests need independent review, successful compilation,
green diagnostic environment verification and intended missing-behavior red before
STORAGE-011 production implementation. Application schemas, retained versions,
receipts, contributions, plans, application bundles, backup/update/restore/recovery
and all seven integrations/three rate providers remain full release scope.

## Integrator-owned physical format clarification

After initial contract `6a92fce`, root fixed exactly two metadata CREATE statements:
`subathon_catalog(singleton INTEGER PRIMARY KEY, catalog_format INTEGER NOT NULL,
store_identity TEXT NOT NULL)` and
`subathon_migration_history(version INTEGER PRIMARY KEY, migration_id TEXT NOT NULL
UNIQUE, content_sha256 BLOB NOT NULL)`. The contract now includes their exact full
SQL and main-schema reservation. This simple DDL needs no new SQLite feature and
fits the existing single-statement writer interface. No physical schema file was
created. Actual stored types/ranges/cardinality remain explicit runner validation;
affinity and the minimal constraints are not claimed as proof of valid metadata.

An independent disposable Python SQLite 3.54.0 probe (exit 0) created these exact
tables and observed their SQL unchanged in sqlite_schema, plus the expected
`sqlite_autoindex_subathon_migration_history_1` UNIQUE index row with NULL SQL.
It also stored an extra header with textual format/BLOB identity and negative
history version/one-byte digest, confirming why explicit validation is required.
This was a technical fixture probe, not selected-Qt acceptance. Selected Qt/SQLite
fixture verification must still precede literal test freeze, including generated
index identity; no host-probe result is transferred to CI/platform support.

Digest framing is now an explicit pseudoformula: all object fields/NULL marker
are F-framed, absent SQL is omitted, present empty SQL remains distinct. Checked
length/counter arithmetic and deterministic unsigned UTF-8 byte tuple sorting
apply equally to state comparison and digest input. Reserved-table and external
side-effect restrictions are stated as trusted audited-manifest preconditions,
with no adversarial SQL-sandbox claim. Existing writer/API/tests remain unchanged.

The reviewer requested additional precise validity rules: N >= 1; unique ordered
starting versions within 0..N including 0/N; no adoption of header/empty-history
state as fresh; printable nonempty ASCII IDs; and strict round-trip UTF-8 for all
hashed/projected text, rejecting lossy QString conversion. The revised contract
states these explicitly, including raw BLOB comparison for stored SQLite text.
Generated catalog reads/writes qualify main to prevent TEMP-name redirection.

The reviewer also found that a bare TVF name could resolve to an ordinary shadow
table containing fake `ok`. A separate independent disposable SQLite 3.54.0 probe
confirmed bare-name fake success and explicit `pragma_integrity_check()` refusal
as not a function after creating that shadow. Both zero-argument function calls
returned the expected clean results without the shadow. Fixed contract queries
now require parentheses, retain LIMIT 2/1, and add shadow/unavailable negative
checks to MC16 and the later green selected-Qt probe. Root separately confirmed
this in actual host Qt probes; this author still claims only the Python technical
probe, not selected-Qt/CI acceptance or new feature red.

## Database-encoding correction before contract approval

Actual re-review of `cd25d32` found that CAST(TEXT AS BLOB) returns bytes in the
database encoding, not necessarily UTF-8. The integrator chose catalog-format-1
UTF-8 encoding explicitly. The contract now requires fixed
`SELECT * FROM pragma_encoding()` as the first inspection, strict one-row/one-TEXT
`UTF-8` result, before stored-byte/catalog/bootstrap trust and again before commit.
UTF-16/unknown/error/shadow results refuse without conversion, a setter or repair.
This is an internal file-format choice with full Unicode capacity, not an external
SQLite-import guarantee or an assumption about every Qt creation API.

Primary [SQLite CAST](https://www.sqlite.org/lang_expr.html#castexpr) and
[encoding](https://www.sqlite.org/pragma.html#pragma_encoding) documentation were
read. An independent disposable Python SQLite 3.54.0 probe created UTF-8,
UTF-16le and UTF-16be fixtures, each with stored `é🙂`. The exact function returned
each declared encoding; CAST bytes were respectively `C3A9F09F9982`,
`E9003DD842DE` and `00E9D83DDE42`. The probe changed encoding only during fixture
creation, not in runner code. No selected-Qt result is claimed by this author.

A separate standard-library Python canonical-framing/SHA-256 calculation produced
the contract's literal valid `m1` content vector, including exact metadata/schema
objects, two SQL strings and multibyte text: 636 payload bytes, digest
`3d1e0f776ae0a3f84c9036c099b0a1333ea735eb88e22fc666f5f3f72bf972bc`.
This independent reference is not future production code. MC17/MC18 now require
real UTF-16 refusal and frozen UTF-8 byte/payload/digest oracles. The future green
selected-Qt environment probe must verify creation encoding/raw bytes and negative
UTF-16/shadow behavior before any intended migration red can count. Existing49,
writer/header/allowlist and executable behavior remain unchanged.
