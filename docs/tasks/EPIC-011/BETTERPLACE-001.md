# BETTERPLACE-001 — Public polling and new-support baseline

Parent: [EPIC-011](../EPIC-011.md). Kind: implementation.
Specification: v0.45; §14, INTAKE-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Betterplace public HTTP adapter, checkpoints and polling schedule with fake network.

Primary module paths: integrations/ betterplace
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Fetch public totals/donation-related data and establish a first-connect baseline without historical automatic credit.
- Persist supported cursors/seen identities and normalize source/arrival times with provider limits/backoff and health.

## Boundary and failure cases

No authenticated API/webhook assumption in public scope; malformed/partial pages and retries cannot duplicate normalized input.

## Separate baseline and implementation tasks

- `BETTERPLACETEST-001` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `BETTERPLACE-001` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: CTest adapter tests with fake HTTP/polling and privacy fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
