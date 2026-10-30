# PACKAGE-002 — Windows installation-free portable ZIP

Parent: [EPIC-025](../EPIC-025.md). Kind: implementation.
Specification: v0.45; §16, PACKAGE-03.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Windows 11 x86-64 client deployment folder/ZIP assembly.

Primary module paths: deploy/ packaging and CMake release presets
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Bundle executable/runtime/Qt/QML/media/plugins/fonts/resources so standard users extract and launch without installer or separate runtime setup.
- Keep client settings/cache/logs in chosen portable folder and persist token only with DPAPI.

## Boundary and failure cases

Multiple files are allowed; no system-wide registration required and no literal single-file/static-Qt shortcut.

## Separate baseline and implementation tasks

- `PACKAGETEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `PACKAGE-002` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Package/manifest checks plus distinct clean-machine installation evidence.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
