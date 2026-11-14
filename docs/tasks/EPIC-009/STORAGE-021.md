# STORAGE-021 — Controlled parent-table rebuild migration capability

Parent: [EPIC-009](../EPIC-009.md).
Specification v0.45 §16, OPS-02, NUM-04, FX-06. Planned and unassigned.

The [concrete exact valuation proposal](../../contracts/exact-valuation-storage-v1.md)
requires an honest exact alternative in the plan revision table while existing
operation/effect/hold/reference foreign keys retain their target names. Migration4
requires decimal fields for resolved legacy rows. The writer enables foreign keys
and permits only narrow read-only PRAGMAs; an arbitrary parent rebuild or deferred
constraint mechanism is not a supported capability. Do not fabricate decimal fields,
weaken foreign keys, change shipped migration bytes or silently alter old formats.

Use a fresh implementation session only after STORAGETEST-021 is independently
reviewed and intended red is observed. Implement the approved controlled
migration capability AND the concrete version-N exact-valuation schema step, with the contract's concrete SQL/port/resource/ownership and
failure semantics. Preserve old migration prefix bytes and all existing reviewed
assertions. No exact valuation producer/plan adapter/provider/cache or application
behavior is included; RATESTEST-009/RATES-009 retain that separate responsibility.

Obtain independent Claude initial code/resource review, Codex fixes/separate
verification and Claude closing approval. Run the reviewed real-SQL acceptance plus
relevant writer/catalog/plan/bundle/hold/reference/account regressions and selected
CI. Show real populated upgrade/rollback and reopen, never infer them from pure
simulation. Protected merge and current-head checks remain required; OPS-02 operational
backup/update/recovery and complete platform/release evidence stay separate.

Coordinator owns shared writer/catalog/schema/build/state integration; exact scoped
paths and baseline revision must be recorded when this task is selected. This split
is planning only and is not authorized for execution by RATECONTRACT-002's one-task
owner directive. Preserve all seven integrations and all three FX providers.

This task ships the concrete MigrationStep (snapshot table plus honest retained-name
plan-revision rebuild), exact_valuation_repository_manifest() factory and complete
expected-schema projection frozen by STORAGECONTRACT-003 and tested by
STORAGETEST-021. It owns passing populated legacy upgrade/rollback/reopen and
old-catalog-refusal acceptance (EV12 schema portion), including actual renamed
schema text/FKs. RATESTEST-009 consumes this shipped schema. RATES-009 separately
owns plan/bundle/hold/reference exact read-write dispatch; no competing migration
step or implicit lifecycle/automatic production upgrade is supplied there.
