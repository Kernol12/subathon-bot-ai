# ACCESS-007 — Chat command categories arguments and cooldowns

Parent: [EPIC-006](../EPIC-006.md). Kind: implementation.
Specification: v0.45; §10.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Core command definitions and authorization independent of incoming chat transport.

Primary module paths: core/ permissions and server/ identity services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Support aliases/arguments/actions/responses with per-user and global cooldowns and all/any combinations of Twitch categories.
- Permit public commands without app accounts; management commands use configured operation permissions rather than blanket Twitch moderator access.

## Boundary and failure cases

Test VIP AND subscriber, moderator OR VIP, explicit users, unknown roles and duplicate delivery using a fake clock.

## Separate baseline and implementation tasks

- `ACCESSTEST-007` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCESS-007` (implementation): depends on that completed baseline and its other
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
