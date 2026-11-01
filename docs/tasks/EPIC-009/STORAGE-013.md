# STORAGE-013 — Identified-receipt SQLite repository adapter

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §3.2, §3.4, DB-01.
Status/owner/dependencies: progress/state.json. Planned work only; no finalized
slice interface, authored/reviewed tests or implementation is claimed by this brief.

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
