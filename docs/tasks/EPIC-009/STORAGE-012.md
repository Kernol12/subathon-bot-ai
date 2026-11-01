# STORAGE-012 — Immutable configuration and arrival-context version retention

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §3, §6, §7, DB-01.
Status/owner/dependencies: progress/state.json. Initial contract/examples authoring
is claimed from integrated STORAGE-011; no finalized interface, authored/reviewed
tests or implementation is claimed by this brief.

## Scope and ownership

Retain caller-validated versioned configuration/context document bytes and immutable references before durable intake can rely on them. This is not a rule/modifier evaluator or finalized HTTP/config document schema.

Primary implementation paths: server/ persistence and scoped tests; read root,
server and tests AGENTS.md. The integrator explicitly owns shared CMake/presets,
schemas/migration ordering, progress/HTML and compatibility decisions. Assign exact
files on claim; independent writers use separate branches/worktrees.

## Acceptance criteria

- Round-trip immutable version identifiers, original document bytes and links; keep optional/unknown facts absent rather than synthesizing values.
- Reject conflicting reuse of a retained identity and expose explicit missing-reference results.
- Preserve the full domain identifier/revision range with a reviewed lossless codec; retain historical meaning for downstream receipt/plan references.

## Failure examples and separate gates

Test empty/nonempty and binary/Unicode content according to the reviewed contract, identity conflict, missing versions, full identifier range and reopen. No current rule/modifier format is fabricated.

`STORAGETEST-012` first refines versioned examples/interfaces, authors tests,
obtains a separate actual review and records intended missing-behavior red after
successful compilation. Minimal interfaces/harness/fakes and explicit stubs are
allowed. `STORAGE-012` depends on that completed baseline and its foundations,
preserves reviewed assertions, passes real acceptance and separate code review.
Neither an inherited passing suite nor missing dependencies is a new red baseline.

Primary testing home: Qt Test/CTest with disposable files and synthetic/redacted
fixtures; use existing public service tests where composition is relevant. Extend
ordinary CI for executable stable behavior, observe actual workflows, record exact
commands/revisions/environment and remaining gates. No live provider/chat/payment
side effects. Follow [workflow](../../AGENT_WORKFLOW.md), update evidence/progress
and publish one focused PR per executable task; no full-release claim follows.

## Initial contract-stage ownership

Root claim036fad8 starts STORAGETEST-012 from refreshed main29f3818 after PR32.
The author initially owns only new docs/contracts/retained-context-v1.md and own
evidence; a different agent reviews the actual contract/examples before any new
API/stubs/tests are authorized. Root owns actual schema/migration order, shared
build/CI/state/map/HTML/checkpoint. Existing opaque core configuration/context
references and writer/migration interfaces remain unchanged. Concrete reference
identity, numeric representation, optional known links, bounds and zero policy
need explicit reviewed decisions; no finalized wire/config evaluator is inferred.
