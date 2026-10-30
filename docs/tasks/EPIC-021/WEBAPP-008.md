# WEBAPP-008 — Browser provider setup and capability health

Parent: [EPIC-021](../EPIC-021.md). Kind: implementation.
Specification: v0.45; §14, INTEGRATION-01, AUTH-15.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Browser service linking/key/webhook and health/capability screens.

Primary module paths: web/management/ and server/ static-asset boundary
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Guide supported server-side authorization with exact installation registration/callback requirements where needed.
- Show grant/evidence/history limits, first-connect/import/catch-up/source-selection policies and health without provider secret exposure.

## Boundary and failure cases

No registration-free promise or shared confidential secret; unavailable capabilities remain visibly unknown/unsupported.

## Separate baseline and implementation tasks

- `WEBAPPTEST-008` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `WEBAPP-008` (implementation): depends on that completed baseline and its other
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
