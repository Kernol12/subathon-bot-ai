# WEBAPP-003 — Browser timer lifecycle and recovery operations

Parent: [EPIC-021](../EPIC-021.md). Kind: implementation.
Specification: v0.45; §4, TEST-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Live timer, reasons/grace/debt/deadline and authorized recovery/reopen UI.

Primary module paths: web/management/ and server/ static-asset boundary
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Render server state with local animation and submit operation-ID/revision-checked timer decisions.
- Expose recovery previews and explicit reopening/grace/debt/action choices under current permissions.

## Boundary and failure cases

Do not clear independent reasons or automatically resend stale commands after reconnect; test with fake clocks and browser workflows.

## Separate baseline and implementation tasks

- `WEBAPPTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `WEBAPP-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Vitest for units/components; Playwright for complete browser workflows.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
