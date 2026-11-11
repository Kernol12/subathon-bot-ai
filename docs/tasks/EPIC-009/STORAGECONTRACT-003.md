# STORAGECONTRACT-003 — Controlled parent-table rebuild migration contract

Parent: [EPIC-009](../EPIC-009.md). Status, owner and dependencies: progress/state.json.
Specification v0.45 §16, OPS-02, NUM-04, FX-06. Planned and unassigned.

The [concrete exact valuation proposal](../../contracts/exact-valuation-storage-v1.md)
requires an honest exact alternative in the plan revision table while existing
operation/effect/hold/reference foreign keys retain their target names. Migration4
requires decimal fields for resolved legacy rows. The writer enables foreign keys
and permits only narrow read-only PRAGMAs; an arbitrary parent rebuild or deferred
constraint mechanism is not a supported capability. Do not fabricate decimal fields,
weaken foreign keys, change shipped migration bytes or silently alter old formats.

Define the smallest controlled migration mechanism that can safely stage the
specified parent-table replacement on populated databases through the actual Qt
writer/catalog. Freeze concrete ports, SQL allowlist/admission, exact ordered DDL,
catalog identity/version and full-prefix/schema result before the executable baseline.
Do not assume PRAGMA changes are safe inside an active transaction. Specify bounded
copy/SQL/result/work budgets, ownership, FK validation, rollback/cleanup, unknown
commit, cancellation and fresh reopen. Preserve every legacy row/optional/blob byte,
all referencing rows and constraints, and original prefix1–8 SQL/IDs/digests.
Prove ordering for actual FK graph; refuse unsupported/corrupt/future catalogs before
writes. Operational explicit-update/pre-upgrade backup/restore gates remain OPS-02;
this narrow mechanism supplies no automatic production upgrade or backup authority.

Include independent populated upgrade, orphan/corrupt/duplicate, DDL/copy/constraint/
commit fault and interrupted/reopened cases. Full independent Claude initial → Codex
fixes/separate verification → Claude closing approval is required. Document-only;
no writer/catalog/schema production changes or authored red from this planned task.

Coordinator owns shared writer/catalog/schema/build/state integration; exact scoped
paths and baseline revision must be recorded when this task is selected. This split
is planning only and is not authorized for execution by RATECONTRACT-002's one-task
owner directive. Preserve all seven integrations and all three FX providers.
