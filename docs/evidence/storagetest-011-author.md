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
  checked in full, not inferred from a maximum version. Root must fix physical
  metadata DDL/names before minimal interface/test authoring.
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

Sixteen literal future examples cover fresh/prior/current inspection, foreign and
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
may root expand ownership to minimal versioned interfaces/stubs/tests and fixed
metadata DDL. Those actual tests need independent review, successful compilation,
green diagnostic environment verification and intended missing-behavior red before
STORAGE-011 production implementation. Application schemas, retained versions,
receipts, contributions, plans, application bundles, backup/update/restore/recovery
and all seven integrations/three rate providers remain full release scope.
