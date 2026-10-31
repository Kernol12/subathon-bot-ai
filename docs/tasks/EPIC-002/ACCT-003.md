# ACCT-003 — Captured application plans and calculation provenance

Parent: [EPIC-002](../EPIC-002.md). Kind: implementation.
Specification: v0.45; §3, §6, §7.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Versioned plan/effect contracts with replaceable rule/modifier interfaces; production evaluator wiring is API-008.

Primary module paths: core/ and server/ application services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Capture arrival configuration, modifier context and requested effect details with original event/contribution/rate references.
- Audited authorized overrides create revisions; current timer caps are applied at commit, not frozen into the preview.

## Boundary and failure cases

Pending inputs retain captured behavior after configuration edits; stale previews and missing historical valuation must be explicit.

## Separate baseline and implementation tasks

- `ACCTTEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCT-003` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for accounting; Qt Test for writer/service integration.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.

## ACCTTEST-003 proposed bounded revision

[Application plans revision 1](../../contracts/application-plans-v1.md) adds pure
owned capture/revision/preview functions with replaceable resolved rule/modifier
sources. It does not introduce a repository or application transaction, implement
API-008 evaluation, calculate numeric effects or freeze current cap-limited actual
amounts. [Authoring evidence](../../evidence/accttest-003-authoring.md) tracks the
unreviewed baseline and concrete checks. Independent review and intended red remain
required; no production behavior is implemented by the baseline author.

Current baseline:22 cases/interfaces/contract/harness independently approved36ac654,
then actual stub-red ata524447 (22/22 failures after successful build). See
[baseline handoff](../../evidence/accttest-003-baseline-handoff.md). Implementation
still requires preserved assertions, separate code review, passing cases and observed
automatic application-plan CI with the selected pinned Boost setup.
