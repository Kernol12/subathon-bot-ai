# LOCALE-090 — Verify English German and theme layouts on native and browser UIs

Parent: [EPIC-023](../EPIC-023.md). Kind: environment.
Specification: v0.45; §12, I18N-01, I18N-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Actual UI language/theme/layout checks plus automated missing-entry/placeholder/plural validation.

Primary module paths: shared localization catalog/generator, desktop/ and web/ preferences
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Exercise long German text, errors, menus/forms and current preference changes across native/web/viewer labels.
- Record Light/Dark/System behavior and separation from authored scenes/content on the tested platforms.

## Boundary and failure cases

DOM controller simulation alone cannot establish product translation/layout or accessibility behavior.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: Catalog/generator checks, Qt Quick Test and Vitest/Playwright for runtime preferences.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
