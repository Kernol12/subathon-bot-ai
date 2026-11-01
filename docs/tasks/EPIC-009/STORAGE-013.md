# STORAGE-013 — Identified-receipt SQLite repository adapter

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §3.2, §3.4, DB-01.
Status/owner/dependencies: progress/state.json. Implementation remains planned.
STORAGETEST-013 now supplies the separately reviewed contract/API/22-case compiling
intended-red baseline; [handoff](../../evidence/storagetest-013-handoff.md) records
exact artifacts and remaining gates. No production adapter is claimed.

## Scope and ownership

Implement the existing synchronous ReceiptRepository/ReceiptTransaction ports through writer-owned work, with immutable original receipt facts, attempts and scoped uniqueness. Do not move intake policy into storage fakes.

Primary implementation paths: server/ persistence and scoped tests; read root,
server and tests AGENTS.md. The integrator explicitly owns shared CMake/presets,
schemas/migration ordering, progress/HTML and compatibility decisions. Assign exact
files on claim; independent writers use separate branches/worktrees.

## Acceptance criteria

- Return service success only after actual atomic commit; preserve durable originals and every attempt/retry/conflict provenance.
- Enforce existing receipt identity uniqueness in SQLite and resolve retained arrival configuration/context references without inventing missing facts.
- Round-trip full uint64 IDs and original money/rate spellings without signed narrowing, REAL conversion or lost precision.

## Failure examples and separate gates

Run focused SQL-backed identified-receipt integration with independent raw SQL observations: duplicate/conflict, failure after attempted insert, missing references, max identifier and lossless original values. Preserve original15 simulation evidence as such; it is not retroactively SQL proof.

`STORAGETEST-013` first refines versioned examples/interfaces, authors tests,
obtains a separate actual review and records intended missing-behavior red after
successful compilation. Minimal interfaces/harness/fakes and explicit stubs are
allowed. `STORAGE-013` depends on that completed baseline and its foundations,
preserves reviewed assertions, passes real acceptance and separate code review.
Neither an inherited passing suite nor missing dependencies is a new red baseline.

Primary testing home: Qt Test/CTest with disposable files and synthetic/redacted
fixtures; use existing public service tests where composition is relevant. Extend
ordinary CI for executable stable behavior, observe actual workflows, record exact
commands/revisions/environment and remaining gates. No live provider/chat/payment
side effects. Follow [workflow](../../AGENT_WORKFLOW.md), update evidence/progress
and publish one focused PR per executable task; no full-release claim follows.

## Retained-context dependency and numeric codec gate

[Retention revision1](../../contracts/retained-context-v1.md) preserves existing
configuration/context opaque references and document bytes losslessly; it creates
no redundant numeric IDs and does not certify a typed numeric SQL conversion.
This slice owns the actual ReceiptId uint64 field: before persistence, review its
lossless SQL codec, complete range and domain-zero policy, independently test raw
representations (including values above signed64 and UINT64_MAX), then establish
the genuine reviewed red. No signed INTEGER/REAL coercion or opaque-byte test
substitutes for that typed gate. Existing identifiers/dependencies/acceptance and
all original lossless/provenance scope remain unchanged.

## Initial contract-stage ownership and established zero policy

STORAGETEST-013 is claimed from maina4bb565 after PR34. Author initially owns only
new sqlite-receipts-v1 contract and own evidence, then a different actual review
precedes any new API/stubs/tests. Root owns actual migration2/schema/order and all
shared build/CI/state; immutable migration1/old ports remain unchanged.

The full uint64 SQL codec must round-trip every0..UINT64_MAX literal without
signed/REAL narrowing. Persisted/allocated domain ReceiptId remains1..MAX: the
already reviewed [application-plan contract](../../contracts/application-plans-v1.md)
rejects zero receipt ID, enforced in core/src/application_plan.cpp and its reviewed
missing-local-identity test. Do not alter that completed contract/assertion. Use
explicit allocator absence (proposed nullable last_allocated BLOB) before first1,
then allocateMAX once and fail exhaustion without wrap/reuse. Root's initial
first0 proposal was withdrawn after discovering this actual cross-contract rule;
no first0 behavior was implemented or approved. Exact schema/options still need
new actual contract review before test authoring.


## Actual contract candidate and rate-field scope reconciliation

The [adapter contract](../../contracts/sqlite-receipts-v1.md) and root-owned
[additive migration2 proposal](../../contracts/sqlite-receipts-schema-v1.md) are
actual documents awaiting different review, not finalized behavior/tests. Existing
ReceiptFacts has no exchange-rate field: exact original_decimal/currency belongs
here; actual HistoricalValuation fetched/effective rate spellings/source/time and
provenance belong to the planned contribution/plan adapters014/015 and their new
reviewed test/red gates. Do not invent an unused receipt column or drop that
mandatory downstream lossless proof. The reviewed core/ports and original15
simulation assertions stay unchanged.
