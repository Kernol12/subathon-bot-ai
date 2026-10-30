# CONTRACT-002 — Define event, effect, clock and persistence boundaries

Module: Core. Kind: contract.
Authoritative status/owner/dependencies: progress/state.json.
Specification references: v0.45 §§2–5,8,18; SERVER-01, DB-01, NUM-01–04,
TIMER-01–10 and linked milestone/intake decisions.

## Scope and ownership

Create the documentation-only [internal foundation contract revision 1](../contracts/event-state-boundaries-v1.md).
Separate receipt/delivery attempts, normalized events, contributions, candidate plans,
actual applied effects/reversals and persisted jobs. Define controlled clock domains,
authoritative Qt queue/SQLite writer ownership and atomic application/deduplication
invariants with concrete examples before dependent implementation.

Author owns that contract, this brief and docs/evidence/contract-002-authoring.md.
Coordinator owns progress/state.json, generated PROGRESS.html, shared architecture,
coordinator handoff, PR and integration. Independent review consumes the actual
authored commit. Concurrent writers use isolated branches/worktrees.

This task supplies no finalized C++/SQL/wire schema, production behavior, provider
capability promise or executable/red-baseline claim. Time-credit revision 1 remains
unchanged. Concrete domain/API/storage interfaces, test baselines and acceptance
red runs belong to their dependency-ready child tasks; full release scope remains.

## Acceptance and handoff

- Version the internal contract and map every record's ownership/links to §3.
  Preserve independent eligibility, captured arrival context and current-state actual
  application; include unknown/missing evidence, duplicate and stale-preview cases.
- Define atomic effect/state/application/job commit and durable receipt separation,
  rollback/retry invariants, writer/read-connection ownership and post-commit
  publication/delivery. Distinguish local deduplication from uncertain remote sending.
- Preserve exact numeric/provenance/reversal requirements, cap-limited actual effects,
  separate debt/grace, completed milestones and performed actions.
- Define monotonic/UTC clock roles, independent pauses, checkpoints, Freeze/Count
  elapsed/Recovery review, hard-end precedence and schedule/DST boundaries without
  silently selecting unresolved clock-discontinuity/provider algorithms.
- Provide specification-derived expected examples/error cases and identify the
  downstream baseline that owns executable coverage. Independent reviewer checks
  consistency and scope before completion; document any unresolved requirement conflict.
- Assess CI coverage, run project/link/progress/tooling checks, record exact commands
  and reviewed revision in evidence. No feature-red run is required for this
  documentation-only contract; it cannot satisfy a production test-first gate.
- Coordinator updates progress/HTML and handoff, publishes one focused PR, observes
  latest-revision CI/review/merge gates, then starts dependent work from integrated main.
