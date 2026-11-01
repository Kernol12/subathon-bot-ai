# STORAGETEST-011 — migration contract authoring

Author: `/root/writer_transaction_reviewer`, explicitly reassigned as author,
2026-11-01. Independent contract/test reviewer: `/root/storage_work_code_reviewer`;
this author cannot approve the new contract or eventual tests.
Branch `task/STORAGETEST-011-author`, isolated worktree
`local-evidence/worktrees/storagetest011-author`, from claim
`9dba71dea7bd1cdeb14e8d7655f3afff1103a3db` after merged main
`88e86c12b3c0e089461f1df2eb970ed0a4b05c68` / PR 30.

## Scope and concrete decisions

Initial authorization was documentation only. This author then owned only
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

## Initial documentation validation and remaining gates

Checks actually run in this isolated author worktree:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit 0; 391 records, 122 decisions, local links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit 0; existing root-owned snapshot current |
| `git diff --check` | Exit 0 |
| `git status --short` | Only the two owned Markdown paths untracked before commit |
| `git diff 9dba71d -- server tests schemas CMakeLists.txt CMakePresets.json .github` | Empty |

In that documentation phase, no new C++, schema, migration, executable test,
build target or workflow was authored.
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

## MC15 oracle precision before test authoring

After actual contract approval, the coordinator authorized one narrow clarification
for different-reviewer approval: migration query counts/projection implementation
are not a public fixed-cost plan. MC15 now uses literal exact configured manifest
bounds plus a guaranteed oversized session result after prior DDL to establish
cumulative rollback. All diagnostic/catalog work remains charged; the existing49
cases retain their exact session-boundary assertions. No count is calibrated from
future production output, budget is reset, or existing assertion is weakened.
Specification requirements and migration atomicity/resource scope are unchanged.
This amendment precedes the actual new interface/test baseline review.

The actual early interface review also requested explicit wrapper policies before
freeze: default inspection/explicit advancement; invalid mode rejected; pure
request preflight before callback validation and delegated writer admission;
effective migration-ID bound min(configured,128) separate from fixed identity;
exact SQL/projection-only manifest UTF-8 accounting; limit/callback/uncertainty
reason mapping. Header and contract now pin those policies without private writer
state access or changes to existing admission behavior. Invalid requests can be
rejected on a foreign/closed/full writer before delegation; valid requests retain
existing writer errors. This precision remains subject to different-reviewer
assessment with the actual interface/test candidate.

## Actual interface, literal fixtures and Qt candidate

Contract `05c8568` / `1683461` received different-agent contract approval
`1bfdfa8`; MC15 precision `6eb420b` / `1dfadece` received `ecff4f5`; actual
early API/admission/limit precision `7ef527c` / `cbdea1a` received `112bf38`.
Those approvals permit baseline authoring, not approval of these new tests or
production behavior. Root explicitly expanded this author's ownership to the new
migration header, explicit stub, new Qt cases/environment probe, optional literal
JSON and this note. Shared build/presets/workflow/map/state remain root-owned.
Normal merges preserved the approval and shared harness history before compilation.

Actual candidate artifacts:

| Artifact | Git blob |
| --- | --- |
| `server/include/subathon/server/sqlite_migrations.hpp` | `02f83ce592b110176c1dac46f3e5ea4d93529fcd` |
| `server/src/sqlite_migrations.cpp` | `6c5073d4a97eedbdd34130ce48be51cf0aa4537a` |
| `tests/qt/sqlite_migration_tests.cpp` | `a85a6c057637f43201dfaca308fbec27748ce2dd` |
| `tests/qt/sqlite_migration_environment.cpp` | `832fe61656451390ff465d4411c7d0900c741bd6` |
| `tests/fixtures/migration_catalog_vectors.json` | `12280c8f403fd3902edaeff88e9a236f9c5b613b` |

The new API supplies owned DTOs, explicit Inspect/Advance mode (default Inspect),
readonly cancellation checkpoints, separate admission/work/migration outcomes,
optional owned report and two pure codec boundaries. Actual new source remains
explicitly unimplemented: enqueue returns feature_not_implemented; payload/digest
functions throw that category. No preflight, serialization, hashing, migration,
SQL, queue, callback or catalog algorithm has been implemented. Codec tests catch
that exception and report a clean explicit missing-feature assertion if run.

The JSON contains ten frozen independent payload/digest vectors, including base
prior-version steps, valid multibyte m1, a codec-only NULL-versus-present-empty
variation, an unsigned UTF-8 ordering edge (U+E000 before U+10000, opposite QString
UTF-16 order), and valid historical CHECK/FK/shadow profiles. The complete m1
payload remains 636 bytes/hash3d1e0f; the base manifest's raw SQL/projection byte
sum is literal1127. These literals were generated by the independent Python
reference before any UUT algorithm exists, then length/SHA values were checked
using standard-library hashlib. Tests do not obtain historical digests or expected
bytes from the new UUT. The empty-SQL variant tests only framing; it is explicitly
not a valid reserved catalog schema.

The fixture creates known historical states through independent SQL and literal
history digests. Snapshots include schema plus stored types/raw BLOB hex, preserving
invalid UTF-8/type distinctions. Real CHECK/FK violations and all three diagnostic
shadow profiles have matching literal schema/history, so an earlier unrelated
schema mismatch cannot masquerade as the requested diagnostic refusal. The
resource case's oversized SELECT references a real newly created/populated table
and requires the second-statement checkpoint/result_limit; earlier metadata
exhaustion cannot satisfy that oracle. No expected internal query count is inferred.

| Contract oracle | Actual authored coverage |
| --- | --- |
| MC01 | `freshAdvanceCommitsWholePrefix`: observer sees no bootstrap between version steps; literal rows10/20, exact history IDs/digests |
| MC02 | `supportedOldAdvancesOnceAndCurrentDoesNotReplay`: preserve prior row10, apply only missing row20, current repeat makes no changes |
| MC03 | `inspectionReportsFreshOldAndCurrentWithoutAdvancement`: default Inspect, fresh/old/current reports without DDL or advancement |
| MC04 | `foreignPartialAndMismatchingCatalogsAreRefused`: sentinel, each half catalog, empty history and wrong identity preserved |
| MC05 | `futureAndUnsupportedOldVersionsAreRefused`: newer/unsupported prior retained without repair |
| MC06 | `invalidManifestsAreRejectedBeforeAdmission`, exact-boundary case: invalid mode/versions/starting lists/IDs/text/limits/duplicates, no SQL/callback |
| MC07 | `historyAndSchemaMismatchesAreRefused`, `storedTypesRangesAndCardinalityAreValidated`: changed content, gaps, extra/missing objects and wrong stored types/shape/range |
| MC08 | `checkConstraintIntegrityFailurePreservesData`: real invalid checked row, matching schema/history, no repair |
| MC09 | `foreignKeyFailureCannotPassIntegrity`: literal missing parent despite real integrity ok |
| MC10 | `failedMigrationRollsBackDdlHistoryAndPriorWrites`: reached prior DDL/write, real SQL failure, independent lock release, reopen/retry explicitly initiated |
| MC11 | `cancellationRollsBackWholeBootstrapAndPrefix`: pre-BEGIN, after DDL, pre-commit and throwing-probe variants with full old-state preservation |
| MC12 | `busyMigrationDoesNotReplay`: completion while external lock remains held; release does not replay; new explicit request succeeds |
| MC13 | `lostCommitAckFailsClosedWithPhysicalTarget`: real complete target/digests, unknown outcome, ordered queued old/new failure/no invocation |
| MC14 | `lostRollbackAckOverridesSqlFailure`: first SQL error plus lost physical rollback acknowledgment yields uncertainty, old state and ordered failed drain |
| MC15 | `cumulativeBudgetsAndManifestLimitsHaveExactBoundaries`: literal2/13/4/1127 bounds, exact ID128/config256 cap, one-over refusals, result65537 versus65536 after real DDL |
| MC16 | `diagnosticShadowsAreRefusedWithoutRepair` and green environment probe: actual function errors and strict supported shapes; no fictional driver result injected |
| MC17 | `utf16StoresAreRefusedWithoutConversion`: both real UTF-16 fixture encodings/data preserved |
| MC18 | `canonicalPayloadAndDigestMatchLiteralVector`, `canonicalNullUnicodeOrderingAndInvalidText`: complete payload/digests, NULL/empty, multibyte ordering and invalid surrogates |
| Composition | `ownedRequestReportAndAdmissionRemainBounded`, `shutdownAndCompletionLifetimePreserveWriterRules`: owned values, combined admission precedence, capacity, drain, throwing/destructive completion |

The green environment target includes no migration header/UUT and links only the
existing writer/Qt Test. Its seven cases exercise actual same-session fixed
functions, literal UTF-8/raw bytes, exact metadata/index NULL SQL, all three
ordinary shadow failures, UTF-16 observations and integrity/FK separation. Actual
local output reports Qt6.12.0/SQLite3.54.0 and all seven cases passed (nine including
init/cleanup), zero skips. The environment result is not new migration acceptance.
Malformed hypothetical TVF return injection and real disk/power-loss faults are
not claimed; function support/shape must stay a green environment prerequisite.

Commands actually run for this candidate on Linux x86-64, GNU16.2.1,
Qt6.12.0/SQLite3.54.0, CMake4.4.5, Ninja1.13.2 and Python3.14.8:

| Command/check | Actual result |
| --- | --- |
| `clang-format -i` on the owned C++ header/stub/test/probe paths | Completed |
| `cmake --preset sqlite-migrations-debug` | Exit 0 |
| `cmake --build --preset sqlite-migrations-debug` | Exit 0; all new and old targets compile; final edited targets rebuilt |
| `ctest --preset sqlite-migrations-debug -R '^scaffold.sqlite_migration_environment$' --verbose` | Exit 0; exact7 actual environment cases passed, repeated after probe safety/version-reporting edits |
| `ctest --preset sqlite-migrations-debug -L 'sqlite-work\|sqlite-writer'` | Exit 0; all49 unchanged writer/work/ownership cases passed |
| Compiled `-functions` versus CTest `--show-only=json-v1` Python comparison | Exact22 migration names/count, exact7 environment names, 76 total registered; no migration body executed |
| Independent JSON payload-length/SHA-256 consistency check | Ten frozen vectors matched; literal636/1127 and Unicode ordering edge checked, no UUT called |
| `python3 tools/check_project.py`, `python3 tools/progress.py render --check`, `git diff --check` | Passed; 391 records/122 decisions, current root-owned HTML |
| Diff against main88e86c1 for existing writer source/header/fault seam and original49 test files | Empty |

No new migration test body was executed by this author. Actual interface/test/
fixture/harness review by a different agent, selected official-Qt green environment
CI and intended missing-feature red remain required. Root owns final integrated
checks/publication/red recording. STORAGE-011 still has no production algorithm.
This baseline does not prove real backup, operational update authority, arbitrary
external SQLite import, domain row encoding, recovery, live providers or release
platform support. All downstream and full-release requirements remain intact.

## Actual test-review correction: reserved catalog DDL

Different reviewer `/root/storage_work_code_reviewer` inspected actual `fa16183`
and requested an isolated reserved-table DDL mismatch oracle. The existing
`historyAndSchemaMismatchesAreRefused` slot now also seeds valid current version 2,
adds `extra TEXT` to `subathon_catalog` through the independent fixture, and
explicitly checks unchanged required header/history rows and clean physical
integrity. The runner must report schema_mismatch with known rollback, no normal
value/report, released write lock and unchanged raw snapshot. An implementation
that validates only row values/application objects can no longer satisfy the slot.

All previous variants/assertions remain. There are still exactly22 slots; no
contract, API, stub, literal vector, green probe, old source or original49 change.
Rebuilt with `cmake --build --preset sqlite-migrations-debug` successfully and
rechecked exact compiled22/CTest discovery, project/render and diff consistency.
No migration body ran; updated actual independent test approval remains pending.
