# VALIDATE-006 — Consolidate platform package capture and restore release gates

Parent: [EPIC-026](../EPIC-026.md). Kind: environment.
Specification: v0.45; §19, PLATFORM-01, PLATFORM-04, PLATFORM-05.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Release evidence reconciliation across Windows/Arch/Kubuntu desktop, Ubuntu server, browsers, OBS and all deployment formats.

Primary module paths: tests/ integration/platform/performance evidence
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Link actual versioned results and independent reviews, including package move/keyring/media, proxy/backup/restore/migration and capture behavior.
- Keep incomplete/failed provider/platform/performance cases visible; establish support only for exercised agreed paths.

## Boundary and failure cases

This task cannot waive any of the seven integration or three rate-provider release requirements or turn unknown behavior into support.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: Existing relevant acceptance suites plus explicitly recorded platform/load runs.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
