# STORAGE-015 — Captured application-plan and revision-history persistence

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §3, §6, §7, NUM-01, DB-01.
Status/owner/dependencies: progress/state.json. Planned work only; no finalized
slice interface, authored/reviewed tests or implementation is claimed by this brief.

## Scope and ownership

Persist owned captured plans and immutable revision history with retained source/config/context references. No evaluator, actual permission engine, current-cap effect application or provider valuation is supplied.

Primary implementation paths: server/ persistence and scoped tests; read root,
server and tests AGENTS.md. The integrator explicitly owns shared CMake/presets,
schemas/migration ordering, progress/HTML and compatibility decisions. Assign exact
files on claim; independent writers use separate branches/worktrees.

## Acceptance criteria

- Round-trip original plan, historical selections/valuation references and authorized-override proposal provenance without mutating original history.
- Store arbitrary-precision rational numerator/denominator and original decimal/rate spellings losslessly, with signed64 resolved units and full uint64 revisions preserved.
- Require resolvable retained references and atomic append/version conflict handling; preview freshness stays distinct from eligibility/authority to commit.

## Failure examples and separate gates

Test exact large rational and decimal representations, max revision, stale/duplicate append, missing references and rollback after preparing a new history row. SQL storage must not reconstruct expected values by calling the implementation under test.

`STORAGETEST-015` first refines versioned examples/interfaces, authors tests,
obtains a separate actual review and records intended missing-behavior red after
successful compilation. Minimal interfaces/harness/fakes and explicit stubs are
allowed. `STORAGE-015` depends on that completed baseline and its foundations,
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
This slice owns the actual CapturedPlanRevision uint64 field: before persistence, review its
lossless SQL codec, complete range and domain-zero policy, independently test raw
representations (including values above signed64 and UINT64_MAX), then establish
the genuine reviewed red. No signed INTEGER/REAL coercion or opaque-byte test
substitutes for that typed gate. Existing identifiers/dependencies/acceptance and
all original lossless/provenance scope remain unchanged.
