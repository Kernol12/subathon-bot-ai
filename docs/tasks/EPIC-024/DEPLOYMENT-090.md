# DEPLOYMENT-090 — Verify Docker persistence HTTPS upgrade and recovery

Parent: [EPIC-024](../EPIC-024.md). Kind: environment.
Specification: v0.45; §16, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Actual disposable Docker host installation across supported lifecycle operations.

Primary module paths: deploy/ docker/podman/native and proxy configuration
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Exercise proxy/webhooks/WebSockets, stop/restart, backup/restore, failed upgrades and persistent certificate/data state.
- Record host/Docker/Compose/image versions and commands, not just syntax checks.

## Boundary and failure cases

No production installation/data; failed recovery or missing environment stays an explicit release blocker.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: Disposable installation/lifecycle tests per deployment; no production host mutations.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
