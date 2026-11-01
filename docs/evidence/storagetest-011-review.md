# STORAGETEST-011 independent contract and test review

Reviewer: `/root/storage_work_code_reviewer`, 2026-11-01. Contract/test author:
`/root/writer_transaction_reviewer`; shared integration author: `/root`.
The reviewer owns only this evidence in isolated worktree
`local-evidence/worktrees/storagetest011-review`, branch
`task/STORAGETEST-011-review`, from claim `9dba71d` after merged PR30/main
`88e86c1`. This reviewer authored no migration contract, production implementation,
schema, executable tests or expected-result fixtures.

Authority: specification v0.45 §16, DB-01/OPS-02, the STORAGE-011 brief, ADR0002,
existing writer/work contracts and the actual implemented source `023451987`.
Root/server/tests instructions, coordinator/workflow instructions, architecture,
test strategy and task state were read. The existing49 writer/work assertions and
their valid reviews/red/pass evidence remain preserved; those gates are not rerun.

## First actual contract candidate and requested changes

The reviewer normally merged actual author
`6a92fce7e86799152395ae9a48995bd3d019fc6a`, integrated at `1d221ae`. The contract
blob is `cf627ed5722afb62f73bf8bf770393362a92e90d`; author evidence is
`d81bd60f3b236937e80d9220734a2241e74af926`. Both actual documents and the
coordinator's diagnostic probe note were inspected. This is a documentation-only
candidate, with physical metadata names/DDL explicitly pending at that revision.

One whole-chain writer work/transaction, schema-plus-content identity, explicit
unsupported-state refusal, separate physical/FK verification, first-error and
uncertain-outcome composition, and the bounded failure record are coherent with
the existing work contract. The OPS-02 preconditions accurately require real
caller preparation while leaving backup/maintenance/update authorization as later
product gates. No new writer API or SQL allowlist expansion is needed.

**Initial verdict: request changes; no contract/test/red approval yet.** The
following concrete points were sent to the author/coordinator before interfaces:

- Freeze the actual two reserved metadata relations and their expected schema
  objects with the integrator. Validate stored types/ranges/cardinality explicitly;
  SQLite affinity alone does not establish the logical catalog contract.
- Define every digest field, including the null marker, through F framing and fix
  deterministic unsigned UTF-8 byte ordering for the schema tuple. The first
  candidate can be read as either raw or F-framed null markers and leaves sorting
  unspecified.
- Require a nonempty manifest with N>=1, reject catalog-plus-empty-history as
  partial, and specify supported-starting-version validation and exact fresh/0
  Inspect/Advance semantics. Otherwise an empty manifest/catalog head0 conflicts
  with version0 meaning strictly fresh.
- Define nonempty ASCII-ID/control/NUL policy and lossless valid UTF-8 handling for
  all hashed/projected text. QString surrogate replacement must not silently
  change content identity.
- Force actual zero-argument diagnostic function calls with parentheses and refuse
  shadowing/unavailable functions, as demonstrated below. Keep the existing SELECT
  boundary and add a negative diagnostic oracle/probe before baseline freezing.

## Independently reproduced diagnostic shadowing

The initial fixed query `SELECT * FROM pragma_integrity_check LIMIT 2` can read an
ordinary table with that name. This is unsafe as a diagnostic contract even when
complete-schema validation also rejects unexpected objects: the first contract
does not prohibit that name in a trusted expected projection. Explicit function
syntax gives a fail-safe refusal when the function is shadowed.

The reviewer ran an ephemeral Python `sqlite3` in-memory probe on host SQLite
3.54.0, process exit0, without writing repository code or a new test. Before
shadowing, both integrity query forms returned `[("ok",)]` and
`SELECT * FROM pragma_foreign_key_check() LIMIT 1` returned `[]`. After:

```sql
CREATE TABLE pragma_integrity_check(value TEXT)
INSERT INTO pragma_integrity_check VALUES ('ok')
```

the bare-name query still returned the table's `[("ok",)]`; the explicit
`SELECT * FROM pragma_integrity_check() LIMIT 2` raised SQLite OperationalError
`'pragma_integrity_check' is not a function`. Both fixed diagnostics therefore
need explicit empty parentheses, with strict shape/type checking and refusal on
errors. This is a real counterexample to the first query contract, not migration
acceptance, a new feature red or proof about the official bundled Qt driver.

The reviewer also read primary [SQLite PRAGMA functions](https://www.sqlite.org/pragma.html#pragma_functions),
[schema-table](https://www.sqlite.org/schematab.html) and
[transaction](https://www.sqlite.org/lang_transaction.html) documentation. These
support the read-only function mechanism, exact stored-schema/null-index treatment
and explicit transaction/error-resolution boundaries. Actual selected-driver
support must still be demonstrated by the later green Qt probe before intended
migration stub-red can count.

No actual migration interface, tests, production behavior or new intended red has
been supplied or approved at this point. Final contract review requires the actual
corrected document; later executable-test approval requires its actual artifacts.

## Actual corrected contract and encoding review

The reviewer inspected actual author `cd25d32f1fbed54f0aacb8616453e66432373c47`,
integrated at `f963147`, contract blob
`6a8f31ffdfd16888c7e63bb5f3bd59e5da1c217a`. It resolves the original findings:
exact integrator-owned metadata DDL and generated index/NULL SQL; explicit stored
types/ranges/cardinality and main-qualified catalog operations; nonempty N>=1,
ordered unique starting versions including 0/N and strictly fresh/partial rules;
complete F framing and unsigned UTF-8 tuple order; printable nonempty ASCII IDs
and strict Unicode validity; and mandatory diagnostic function parentheses with
both shadow-refusal oracles. The recorded initial request-changes history remains
valid rather than being rewritten as an earlier approval.

One additional precision issue remained in that actual revision: its proposed
CAST(TEXT AS BLOB) comparison assumed UTF-8 bytes without a database-encoding
policy. The reviewer read primary [SQLite CAST](https://www.sqlite.org/lang_expr.html#castexpr)
and [encoding](https://www.sqlite.org/pragma.html#pragma_encoding) documentation:
the cast depends on database encoding, whose creation default depends on the
opening API. A separate reviewer-run ephemeral Python SQLite3.54.0 probe, exit0,
stored the same TEXT `é` under UTF-8, UTF-16le and UTF-16be. The fixed
`pragma_encoding()` query returned each exact label; CAST bytes were respectively
`C3A9`, `E900` and `00E9`. This confirmed the issue without writing repository
code or claiming selected-Qt/migration acceptance.

The integrator chose explicit UTF-8 for the new catalog format. The reviewer then
normally merged and inspected actual author
`05c85681c088812d56c651e9eb77b956d5845b3e`, integrated at `748f0c3`. Final artifacts:

| Artifact | Actual reviewed Git blob |
| --- | --- |
| Migration catalog contract | `1683461ed5172b1c8bf968e6020f89c3c37a0bb2` |
| Contract author evidence | `051d89772405c1b5955935f7dbcdd2d0066069e2` |

The final text requires `SELECT * FROM pragma_encoding()` as the first inspection
inside the work transaction, before trusting stored text/catalog data or bootstrap.
It accepts exactly one TEXT `UTF-8`, repeats the check before final commit, and
refuses UTF-16/unknown/error/shadow results without conversion, a PRAGMA setter or
repair. UTF-8 remains full Unicode support; arbitrary external SQLite encodings
were never an implemented catalog/import capability. MC17/MC18 now require real
encoding refusal and byte/digest oracles. The later green selected-Qt probe must
verify actual creation encoding, multibyte raw bytes and negative shadow/UTF-16
behavior; host Python facts cannot substitute for that gate.

## Contract-only validation and verdict

The reviewer independently encoded the specified literal `m1` vector with standard
Python bytes/hashlib, using the exact four schema tuples, metadata CREATE text,
ordered SQL and F formula from the document. It produced exactly 636 bytes and
SHA256 `3d1e0f776ae0a3f84c9036c099b0a1333ea735eb88e22fc666f5f3f72bf972bc`.
The independent F(U(`é🙂`)) result was `363ac3a9f09f9982`. Sorted objects were index,
sample table, catalog table, history table, as required by unsigned UTF-8 tuple
ordering. This is an independent contract-oracle check, not code from the future
implementation or an executable migration acceptance result.

Whole-chain/bootstrap atomicity, fresh/old/current inspection, refusal without
repair, separate physical/FK/encoding checks, bounded failure information,
underlying sticky/uncertain outcomes, cooperative precommit cancellation and the
OPS-02 caller/product boundary were rechecked against the final actual text. The
18 examples give concrete future oracles without inventing a domain schema or
claiming process-kill/power-loss, backup/update orchestration or provider support.
No unresolved contract finding remains.

Commands/checks actually run for this contract review:

| Command/check | Actual result |
| --- | --- |
| `python3 tools/check_project.py` | Exit0; 391 records/122 decisions, links/presets/current HTML valid |
| `python3 tools/progress.py render --check` | Exit0; root-owned snapshot current |
| `git diff --check` | Exit0 |
| `git diff --exit-code 9dba71d HEAD -- server tests schemas CMakeLists.txt CMakePresets.json .github` | Exit0; no executable/schema/build/workflow changes |
| Independent Python shadow, encoding and literal framing probes described above | Exit0; actual technical results recorded, no feature-red claim |

**Verdict: approved the actual migration contract at blob
`1683461ed5172b1c8bf968e6020f89c3c37a0bb2`, integrated `748f0c3`, for minimal
interface/stub/test authoring only.** This is neither test-baseline approval nor
STORAGETEST-011/STORAGE-011 completion. The actual C++ boundary, deterministic
fixtures, literal payload/digest assertions, green local/selected-Qt environment
probe, test/harness review and genuine intended missing-behavior red remain later
gates before production implementation. The original49 writer/work assertions,
API/allowlist semantics and full downstream storage/product scope remain required.

## MC15 contract-oracle amendment

The reviewer normally merged and inspected actual author
`6eb420becc7275d9ff858d63afc244d6a901920c`, integrated at `037bb4f`, on 2026-11-01.
Contract blob `1dfadece85eeabe0355c4640519b198ba7d328d1` differs from approved
`1683461` in exactly the MC15 row; the author evidence adds a separate rationale.
No interface, production source, schema, executable test or build/workflow changed.

The amendment uses exact configured manifest-boundary and one-over literals, plus
an oversized session result after prior DDL requiring whole-chain rollback. This
avoids inventing an implementation-specific count of internal queries/projection
columns or calibrating expected thresholds from future production output. The
normative requirements still charge every diagnostic/catalog/migration statement,
binding and result against the same cumulative session budgets. No reset, hidden
increase, transaction fragmentation or scope change is allowed; the original49
cases retain their exact session-budget assertions.

This is a justified clarification of the future integration oracle, before any
new reviewed assertion exists. A later case must establish that its prior-DDL
stage was reached; an earlier metadata-budget failure would not demonstrate that
scenario. An oversized single result alone does not prove every internal overhead
charge, which remains subject to the unchanged session contract, existing exact
assertions and later actual implementation review. No fixed query plan is added.

Independent `python3 tools/check_project.py`, progress render-check and whitespace
checks passed; the exact server/tests/schemas/root-build/workflow diff from
`9dba71d` remains empty. No native or new migration body was run for this doc-only
review.

**Verdict: approved only the actual MC15 contract amendment at blob
`1dfadece85eeabe0355c4640519b198ba7d328d1`, integrated `037bb4f`.** All other
approved contract rules remain unchanged. This does not approve the forthcoming
minimal API, stubs, executable cases, green environment harness or intended red;
those require a separate review of their actual authored artifacts.

## Early interface and admission/resource precision

The reviewer read actual first interface/stubs author `496d901`, integrated
`fe72a14`, and shared harness `3df1dd2`. The private-free static runner, owned DTOs,
unchanged embedded SqliteWorkResult, separate reason/report and explicit codec/
admission missing-feature stubs fit the approved boundary. No migration algorithm
or SQL handle is present. The reviewer requested safe default inspection, invalid
mode/limit precision, explicit preflight-versus-writer admission priority and
limit/callback/uncertainty mappings before test freeze.

Actual author `7ef527cb19d9a213fd2c1c218db4ffe68766c46f`, integrated `e99c955`,
was then normally merged and inspected. The actual header is blob
`86b74e94186ffa2713d334b7e936bbf1f2a3e3e8`; contract precision is blob
`cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61`; explicit stub remains
`6c5073d4a97eedbdd34130ce48be51cf0aa4537a`.

The request now defaults to inspect. Pure mode/manifest/positive-limit validation
precedes nonempty-completion validation, then valid requests delegate to the
existing writer. Invalid requests return invalid_options/invalid_manifest even
for closed/foreign/full writers; valid null completion returns invalid_work/none.
This is an explicit additive wrapper priority and requires no private writer-state
probe. Existing low-level writer admissions and their49 assertions are unchanged.

The effective migration-ID cap is min(configured,128), separate from equality with
the fixed21-byte store identity. Manifest payload accounting counts every SQL and
projected type/name/table-name/present-SQL UTF-8 occurrence across all steps,
including repeated projections; IDs/identity, counts, frames and absent SQL are
excluded from this particular limit. Actual tests can therefore use independently
counted literal bounds. Work resource_limit/result_limit map to migration
resource_limit. Callback exceptions, first-failure preservation and uncertainty's
override of both work and migration fields are fixed by header/contract text.
Normal reports still require normal committed work and a normal owned work value.

The shared harness keeps old49 automatic, separates a mandatory green existing-
writer diagnostic executable with seven required slots, and lists22 intended-red
migration slots separately. Known-red bodies require explicit manual selection
with normal failure propagation. Pinned actionlint and project checks pass, but
the actual seven/22 source bodies and vector fixture are still being authored at
this review point; no compilation, selected-Qt CI or new body execution is claimed.

**Verdict: approved the actual narrow contract/interface precision at `e99c955`
for completing minimal baseline authoring.** This is not full interface/test/
harness-baseline approval. Actual cases, literal vectors, successful compilation,
green local/selected-Qt execution and separate test/harness review must still be
examined before any intended migration red or production implementation.

## Full actual interface, fixture, test and harness review

The reviewer inspected actual full author candidate
`fa161831b2a1f001356d0d33bfcfd3fc6349bfb5`, integrated `b67e81b`, followed by
the requested author correction `cae0267f6d033b21317cb219f8d8ceae45a54970`,
integrated `4fa008f`. Both revisions were normally merged into this isolated
review branch. The reviewer read all22 migration bodies, their helpers, all seven
green environment bodies, the complete literal JSON, current header/stub, author
evidence and actual shared CMake/preset/workflow changes.

| Artifact | Exact approved Git blob |
| --- | --- |
| Migration header | `02f83ce592b110176c1dac46f3e5ea4d93529fcd` |
| Explicit missing-feature source | `6c5073d4a97eedbdd34130ce48be51cf0aa4537a` |
| Corrected22 migration cases | `a85a6c057637f43201dfaca308fbec27748ce2dd` |
| Seven green environment cases | `832fe61656451390ff465d4411c7d0900c741bd6` |
| Ten literal payload/digest vectors | `12280c8f403fd3902edaeff88e9a236f9c5b613b` |
| Contract including approved precision | `cbdea1a48d2989a9c8bd17edf0539bdf7c1bce61` |

The header differs from the previously reviewed interface only in generic exception
message text. Source is still17 lines of explicit missing-feature stubs: codecs
throw the named category and admission returns it. It implements no hashing,
validation, queueing, SQL, catalog, callback or migration result. The existing
writer source/header/fault seam, old49 assertions and original writer/work
contracts have an empty exact-path diff from claim `9dba71d`.

Expected historical state is prepared by independent fixture SQL and frozen JSON
digests, without calling a migration codec or runner. The reviewer independently
reframed all ten JSON vectors using standard Python bytes/hashlib: complete payload
hex, literal lengths and SHA256 values all match. The raw base-manifest SQL/
projection sum independently equals1127. A separate disposable Python schema
observation confirms that all five CHECK/FK/shadow profiles exactly match their
frozen expected schema/history profiles. This avoids an unrelated mismatch being
mistaken for the intended diagnostic refusal. Snapshots include stored types and
raw BLOB hex, preserving invalid UTF-8 and type distinctions.

MC01–MC18 are represented by concrete assertions: one complete fresh prefix with
external invisibility before commit; retained prior state/current no replay;
fresh/old/current inspection; foreign/partial/future/unsupported state refusal;
invalid manifests and stored metadata; actual CHECK/FK failures; late SQL failure,
pre-BEGIN/intermediate/precommit cancellation and throwing probe; busy/no replay;
physical lost-ack classification and ordered failed drain; exact manifest bounds;
late result exhaustion; all three function shadows; UTF-16 refusal; and complete
NULL/multibyte/unsigned UTF-8 codec vectors. Failure helpers require absent normal
work values and reports. Inspection/readiness and migration/work outcomes remain
separate. Admission ownership, explicit advancement, capacity, shutdown, throwing
completion and completion destruction are also asserted.

The late result-budget case reaches the before-second-statement checkpoint after
`CREATE TABLE scratch AS SELECT 10 AS amount`. Its literal `zeroblob(65537)` SELECT
reads the actual new populated table against a65536-byte budget. Missing DDL would
cause SQL failure, and empty output would not meet result_limit; earlier catalog
exhaustion cannot satisfy the reached-stage assertion. Exact manifest boundaries
use literal2/13/4/1127 and128-byte ID values, without inferring internal query cost.

One focused coverage finding was corrected before approval. The first actual
candidate changed application objects or metadata row values, but did not isolate
reserved metadata DDL identity. Author `cae0267` retains every previous variant and
adds a current-version fixture with an extra catalog column, unchanged required
header/history rows and clean physical integrity. It requires schema_mismatch,
known rollback, no value/report, released lock and an unchanged raw snapshot.
The reviewer inspected that actual correction and rebuilt the changed target.
An implementation that ignores reserved-table projections cannot pass this oracle.
No baseline assertion was deleted or weakened; the suite still has22 slots.

The environment executable includes no migration header/UUT and links only the
existing writer and Qt Test. Its actual green cases establish fixed function()
support, UTF-8 creation/raw multibyte bytes, exact metadata SQL/NULL autoindex,
all three shadow failures, real UTF-16 observations and distinct integrity/FK
results. It does not claim the migration stub enforces those policies. The actual
shared harness `3df1dd2` keeps old49 unconditional, verifies all seven green slots
and executes them verbosely before migration discovery. New22 remain separate
known-red bodies; manual execution propagates ordinary failures. There is no
skip, failure inversion, WILL_FAIL or blanket continue-on-error.

### Independent commands and actual results

Environment: Linux x86-64, GNU16.2.1, Qt6.12.0/host SQLite3.54.0,
CMake4.4.5, Ninja1.13.2 and Python3.14.8. No personal preset or live data.

| Command/check | Actual result |
| --- | --- |
| `cmake --preset sqlite-migrations-debug` | Exit0 at `b67e81b` |
| `cmake --build --preset sqlite-migrations-debug` | Exit0 initially; corrected acceptance target rebuilt successfully at `4fa008f` |
| `ctest --preset sqlite-migrations-debug -R '^scaffold.sqlite_migration_environment$' --verbose` | Exit0; all seven actual environment cases passed, zero skips; runtime Qt6.12.0/SQLite3.54.0 logged |
| `ctest --preset sqlite-migrations-debug -L 'sqlite-writer\|sqlite-work'` | Exit0; all49 unchanged old cases passed |
| Compiled `-functions` versus CTest JSON comparison | Exact22 new,24 work,22 writer and3 ownership names/counts; one unfiltered environment CTest with seven slots;76 total entries; repeated new22 discovery after correction |
| Independent Python vector, manifest-byte and fixture-projection checks | Exit0; all ten payload/length/SHA256 vectors,1127 bytes and five diagnostic profiles match |
| Project consistency, progress render-check, exact old-artifact comparison and whitespace checks | Exit0 at corrected candidate |
| `/tmp/subathon-actionlint-1.7.12/actionlint .github/workflows/sqlite-writer.yml` | Exit0 |

Only the acceptance source and author note changed in the correction. Its unchanged
green environment and old49 binaries were not needlessly rerun. No new migration
test body has been executed by this reviewer, and no intended migration red or
passing migration behavior is claimed. Compilation/discovery and old green tests
are not substitutes for that new red.

**Verdict: approved the actual corrected interface,22 test bodies, independent
fixtures and shared harness at `4fa008f` for the STORAGETEST-011 baseline.** No
blocking artifact finding remains. Before the first accepted intended-red run,
the coordinator must also observe these unchanged seven green environment cases
in the selected official-Qt CI, then run the exact approved22 against these explicit
stubs and record their genuine missing-feature failures. That external dependency
gate is still pending at this approval; host results are not transferred to it.
STORAGETEST-011 completion metadata/publication and STORAGE-011 implementation,
code review, passing unchanged acceptance and automatic execution remain later
gates. No domain schema, update/backup authority, recovery, power-loss, provider
or release-platform capability is approved.
