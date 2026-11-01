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
