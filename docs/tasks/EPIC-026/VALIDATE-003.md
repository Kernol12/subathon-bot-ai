# VALIDATE-003 — Run supported browser and client compatibility matrix

Parent: [EPIC-026](../EPIC-026.md). Kind: environment.
Specification: v0.45; §2, §19, WEB-02, API-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Current/previous Chrome/Edge/Firefox workflows, best-effort Safari and supported desktop/API combinations.

Primary module paths: tests/ integration/platform/performance evidence
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record concrete browser/OS/client/server versions with permission/invitation/review/live-update/export workflows.
- Verify incompatible-version guidance and matching web assets; report Safari limitations separately.

## Boundary and failure cases

Playwright engine coverage cannot imply every branded browser/version or actual OBS embedded-browser compatibility.

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
