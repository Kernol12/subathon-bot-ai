# LOCALE-003 — Web management and viewer runtime translation

Parent: [EPIC-023](../EPIC-023.md). Kind: implementation.
Specification: v0.45; §12, I18N-01, I18N-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Browser catalog consumption/detection/preferences for management and built-in viewer labels.

Primary module paths: shared localization catalog/generator, desktop/ and web/ preferences
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Switch English/German immediately with browser detection, explicit saved choice and English fallback.
- Keep streamer-authored viewer design/content separate and use structured localized server errors.

## Boundary and failure cases

Verify plural/parameter substitution and long German content; translation changes must not require business-logic edits.

## Separate baseline and implementation tasks

- `LOCALETEST-003` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `LOCALE-003` (implementation): depends on that completed baseline and its other
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
