# RATES-004 — Open Exchange Rates plan and usage adapter

Parent: [EPIC-017](../EPIC-017.md). Kind: implementation.
Specification: v0.45; §15, §9.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Provider rate/usage endpoint mapping and verified plan restrictions.

Primary module paths: core/ exact currency values and server/ rate/provider services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Normalize rates and provider plan/update/usage metadata with exact source/fetch timestamps.
- Apply documented usage-check accounting and distinguish authoritative quota from server-local estimates.

## Boundary and failure cases

Test missing metadata, forbidden plan features and authentication failure; historical retrieval is enabled only for supported profiles.

## Separate baseline and implementation tasks

- `RATESTEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RATES-004` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for exact rates; Qt Test/fake HTTP for scheduling/cache/adapters.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
