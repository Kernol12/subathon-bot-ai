# ACCESS-004 — Account login and optional Twitch identity linking

Parent: [EPIC-006](../EPIC-006.md). Kind: implementation.
Specification: v0.45; §10, AUTH-01, AUTH-15.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Local account sign-in plus provider identity-link callback binding to stable Twitch user IDs.

Primary module paths: core/ permissions and server/ identity services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Both login methods resolve to the same application account/permissions; optional Twitch linking uses supported server-side flows.
- Retain owner local fallback and require fresh authentication for sensitive login-method changes.

## Boundary and failure cases

Prevent linking to another account through stale/replayed callbacks; app sign-in is distinct from broadcaster/bot sender authorization.

## Separate baseline and implementation tasks

- `ACCESSTEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCESS-004` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for pure permission/command rules; Qt Test for account transactions.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
