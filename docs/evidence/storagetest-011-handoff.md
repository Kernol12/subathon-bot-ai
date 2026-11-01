# STORAGETEST-011 — migration catalog contract stage

Coordinator `/root`, author `/root/writer_transaction_reviewer`, different reviewer
`/root/storage_work_code_reviewer`, 2026-11-01. Branch
`task/STORAGETEST-011-migration-catalog` from merged main88e86c1 after PR30.
Specification v0.45 §16, DB-01/OPS-02; [slice brief](../tasks/EPIC-009/STORAGE-011.md).

Author6a92fce integrated1d221ae supplies the actual logical
[migration contract](../contracts/migration-catalog-v1.md), blob
cf627ed5722afb62f73bf8bf770393362a92e90d, and
[author evidence](storagetest-011-author.md), blobd81bd60. This is documentation
only, not independently approved yet. Proposed16 future oracles are not executable
tests or observed red. Existing source023451/49 assertions/interfaces/contracts
remain unchanged and their valid review/pass evidence is preserved.

Root owns shared schema ordering/build/presets/CI/state/HTML/checkpoint. Author owns
only the new contract and author evidence at this stage. Independent reviewer has
the actual logical draft in storagetest011-review; author is resumed for root's
coordinated physical catalog DDL/names and unambiguous digest framing before final
contract approval. No C++/schema files/test targets/workflow changes authorized yet.

Root chose two reserved metadata relations: subathon_catalog(singleton,
catalog_format,store_identity) and subathon_migration_history(version,migration_id,
content_sha256), with INTEGER primary keys, NOT NULL metadata, unique migration ID
and logical stored-type/range/cardinality validation. Plain CREATE only; no domain
table, IF NOT EXISTS, STRICT or new driver capability. Exact DDL belongs in the next
authored contract revision before test/interface freeze. Expected schema projection
must include engine-generated index rows with SQL null distinct from empty.

[Root technical probe](storagetest-011-probe.md) and author independent SQLite probe
support using existing SELECT TVFs for physical integrity and separate foreign-key
checks, without new writer API/allowlist. Numeric TVF(1) is not the direct PRAGMA
limit form; its failed probe is not acceptance/red. Actual selected official-Qt
support must be verified by a focused green environment probe before eventual red.

Pending contract review covers whole-prefix transaction atomicity, complete
identity/history/schema verification, fresh/foreign/partial/future refusal, exact
framing/limits, failure/cancellation/uncertain-outcome precedence, normal report
versus error information, and OPS02 explicit updates/quiescence/real backup and
compatibility prerequisites. No fictitious proof field establishes that operational
authority. Catalog verification is structural, not application/recovery readiness
or authenticity. Full backup/update/restore/application gates remain separate.

Author/root project391/current HTML/diff checks pass for docs. No new native suite,
migration behavior, observed red, live/provider/platform or production schema is
claimed. After actual contract approval root may expand ownership to minimal
interfaces/stubs, green diagnostic probe and focused real Qt SQL cases. Those
actual tests need different review, successful compilation/environment and genuine
new missing-behavior red before STORAGE-011 production. No owner question.

Initial different contract reviewd60021e integratede11d150 requests changes to
actualcf627: physicalDDL, unambiguous framing/text/0 policy and explicit diagnostic
function calls. It grants no test/red/implementation approval. Reviewer found a
real shadow-table counterexample; root actual Qt confirmed () succeeds healthy and
rejects shadows where bare names fake ok/empty. [Review](storagetest-011-review.md).
Author is working the corrections; root/probe history remains preserved.
