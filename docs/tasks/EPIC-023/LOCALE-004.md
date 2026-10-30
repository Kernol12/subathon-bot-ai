# LOCALE-004 — Independent Light Dark and System appearance

Parent: [EPIC-023](../EPIC-023.md). Kind: implementation.
Specification: v0.45; §12, I18N-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Native/web management theme preference and platform appearance detection/fallback.

Primary module paths: shared localization catalog/generator, desktop/ and web/ preferences
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Default to System and apply Light/Dark/System changes immediately per individual user.
- Follow system appearance changes while System is selected without affecting broadcast scenes or configured viewer design.

## Boundary and failure cases

Test unavailable OS detection, saved preference and simultaneous clients with distinct theme choices.

## Separate baseline and implementation tasks

- `LOCALETEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `LOCALE-004` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Catalog/generator checks, Qt Quick Test and Vitest/Playwright for runtime preferences.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
