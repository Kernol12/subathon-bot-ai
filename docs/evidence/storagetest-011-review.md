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
