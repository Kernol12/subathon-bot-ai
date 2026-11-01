# STORAGE-011 — Version catalog and transactional migration harness

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §16, DB-01, OPS-02.
Status/owner/dependencies: progress/state.json. Planned work only; no finalized
slice interface, authored/reviewed tests or implementation is claimed by this brief.

## Scope and ownership

Add the internal version catalog and controlled migration runner, without inventing application effect/job schemas. Later repository tasks own their versioned domain migrations.

Primary implementation paths: server/ persistence and scoped tests; read root,
server and tests AGENTS.md. The integrator explicitly owns shared CMake/presets,
schemas/migration ordering, progress/HTML and compatibility decisions. Assign exact
files on claim; independent writers use separate branches/worktrees.

## Acceptance criteria

- Apply migration changes and catalog advancement atomically on the controlled writer; verify schema identity/integrity before normal store use.
- Refuse unsupported/newer or inconsistent versions without silently repairing, narrowing or destroying stored state.
- Preserve supported prior-version fixtures and define failure/interruption outcomes; no network storage or platform durability claim.

## Failure examples and separate gates

Test fresh and supported-old catalogs, failed/interrupted migration, wrong identity/integrity, duplicate version and future-version refusal. Use synthetic DDL and disposable files; crash/power-loss recovery guarantees remain explicit later gates.

`STORAGETEST-011` first refines versioned examples/interfaces, authors tests,
obtains a separate actual review and records intended missing-behavior red after
successful compilation. Minimal interfaces/harness/fakes and explicit stubs are
allowed. `STORAGE-011` depends on that completed baseline and its foundations,
preserves reviewed assertions, passes real acceptance and separate code review.
Neither an inherited passing suite nor missing dependencies is a new red baseline.

Primary testing home: Qt Test/CTest with disposable files and synthetic/redacted
fixtures; use existing public service tests where composition is relevant. Extend
ordinary CI for executable stable behavior, observe actual workflows, record exact
commands/revisions/environment and remaining gates. No live provider/chat/payment
side effects. Follow [workflow](../../AGENT_WORKFLOW.md), update evidence/progress
and publish one focused PR per executable task; no full-release claim follows.
