# RATES-003 — ExchangeRate-API plan and quota-aware adapter

Parent: [EPIC-017](../EPIC-017.md). Kind: implementation.
Specification: v0.45; §15, §9.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Distinct supported keyed/open plan profiles and quota-response normalization.

Primary module paths: core/ exact currency values and server/ rate/provider services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Validate supported endpoints/update cadence/plan capabilities and keep credentials server-side.
- Count quota-check requests where required and expose stale authoritative quota plus subsequent local usage separately.

## Boundary and failure cases

Do not let reference allowance edits lift provider restrictions; test exhausted quota, auth errors and unavailable reset metadata.

## Separate baseline and implementation tasks

- `RATESTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `RATES-003` (implementation): depends on that completed baseline and its other
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
