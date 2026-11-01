# STORAGE-010 — Writer-owned dynamic repository transactions

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §2, §3.4, DB-01, SERVER-01.
Status/owner/dependencies: progress/state.json. Planned work only; no finalized
slice interface, authored/reviewed tests or implementation is claimed by this brief.

## Scope and ownership

Extend the controlled writer with the separately reviewed work/session boundary, preserving prebuilt batch and read-only reader behavior. No real receipt/contribution tables belong to this mechanism slice.

Primary implementation paths: server/ persistence and scoped tests; read root,
server and tests AGENTS.md. The integrator explicitly owns shared CMake/presets,
schemas/migration ordering, progress/HTML and compatibility decisions. Assign exact
files on claim; independent writers use separate branches/worktrees.

## Acceptance criteria

- Execute dynamic read/decide/write on the same private owner-thread connection and preserve actual synchronous service commit semantics.
- Share admission, active-work accounting, FIFO, bounded SQL/payload/result resources and drain/rejection with existing batches; reject stale/foreign session capabilities before SQL access.
- Distinguish known rollback, successful commit followed by work failure and unresolved durability; no blind replay, second writer, nested waits or raw SQL handle escape.

## Failure examples and separate gates

Use real disposable SQLite and deterministic callbacks: actual committed visibility, rollback after a prior write, busy, work exception before/after commit, retained capability misuse, resource exhaustion and shutdown. Preserve the existing25 writer cases; add focused service-port/session integration cases.

`STORAGETEST-010` first refines versioned examples/interfaces, authors tests,
obtains a separate actual review and records intended missing-behavior red after
successful compilation. Minimal interfaces/harness/fakes and explicit stubs are
allowed. `STORAGE-010` depends on that completed baseline and its foundations,
preserves reviewed assertions, passes real acceptance and separate code review.
Neither an inherited passing suite nor missing dependencies is a new red baseline.

Primary testing home: Qt Test/CTest with disposable files and synthetic/redacted
fixtures; use existing public service tests where composition is relevant. Extend
ordinary CI for executable stable behavior, observe actual workflows, record exact
commands/revisions/environment and remaining gates. No live provider/chat/payment
side effects. Follow [workflow](../../AGENT_WORKFLOW.md), update evidence/progress
and publish one focused PR per executable task; no full-release claim follows.
