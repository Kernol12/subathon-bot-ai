# STORAGETEST-021 — Review and demonstrate red: controlled parent-table rebuild migration

Parent: [EPIC-009](../EPIC-009.md). Status, owner and dependencies: progress/state.json.
Specification v0.45 §16, OPS-02, NUM-04, FX-06. Planned and unassigned.

The [concrete exact valuation proposal](../../contracts/exact-valuation-storage-v1.md)
requires an honest exact alternative in the plan revision table while existing
operation/effect/hold/reference foreign keys retain their target names. Migration4
requires decimal fields for resolved legacy rows. The writer enables foreign keys
and permits only narrow read-only PRAGMAs; an arbitrary parent rebuild or deferred
constraint mechanism is not a supported capability. Do not fabricate decimal fields,
weaken foreign keys, change shipped migration bytes or silently alter old formats.

Author independent real-Qt-SQL acceptance tests from the separately approved
STORAGECONTRACT-003 concrete mechanism/DDL and exact valuation target. Review test
oracles and resource/source requirements through the full Claude/Codex/Claude cycle,
then observe intended behavioral red with valid Qt>=6.12/SQLite setup. Compile failures,
skips and dependency failures are not red. Minimal inert interfaces/stubs are allowed
only after their own review gate; do not implement the mechanism in the baseline.

Cover populated legacy rows and operation/effect/hold/reference FKs, exact whole-prefix
catalog bytes, atomic data/schema/history rollback, result/copy/statement budgets,
wrong-thread/stale/session ownership, refusal of corrupt/future versions, every
approved failure boundary and fresh reopen. Preserve all historical writer49,
migration24, plan29, bundle35, hold and reference/account assertions; no weakened
or replaced tests without explicit separately approved baseline revision.

Coordinator owns shared writer/catalog/schema/build/state integration; exact scoped
paths and baseline revision must be recorded when this task is selected. This split
is planning only and is not authorized for execution by RATECONTRACT-002's one-task
owner directive. Preserve all seven integrations and all three FX providers.

This baseline explicitly owns the new version-N snapshot-table/retained-name
plan-rebuild step and exact_valuation_repository_manifest() factory oracles,
full expected-schema projection, populated legacy upgrade/rollback/reopen and
old-catalog-refusal acceptance (EV12 schema portion). Pin actual post-rename
sqlite_schema text/FK targets; do not infer them from original CREATE text.
RATESTEST-009 later tests exact new read/write/mixed history on this shipped schema.
