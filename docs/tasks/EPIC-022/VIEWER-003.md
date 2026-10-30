# VIEWER-003 — Exact metrics and historical refund aggregation

Parent: [EPIC-022](../EPIC-022.md). Kind: implementation.
Specification: v0.45; §13, STATS-01, STATS-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Query/aggregation service over immutable contributions/effects/rates with bounded exact totals.

Primary module paths: server/ projections/statistics and web/viewer/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Provide gross/refund/net(default) money, credited score/time and known subscriptions/tier/Prime/gifts/Bits classifications with pending/rejected separated.
- Use reliable source time else arrival, historical catch-up buckets and refund-time negatives in selected timezone; default event-window support plus later corrections.

## Boundary and failure cases

Do not count subscription rule valuations as cash or multiply donations by score factors; post-finish support is explicit opt-in and missing rates yield partial totals.

## Separate baseline and implementation tasks

- `VIEWERTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `VIEWER-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest/Qt Test for exact projections; Vitest/Playwright for viewer workflows.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
