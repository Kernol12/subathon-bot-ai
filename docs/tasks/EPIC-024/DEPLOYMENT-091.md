# DEPLOYMENT-091 — Verify both Podman Quadlet deployment variants

Parent: [EPIC-024](../EPIC-024.md). Kind: environment.
Specification: v0.45; §16, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Actual rootless dedicated-account/host-Caddy and rootful/container-Caddy installation matrix.

Primary module paths: deploy/ docker/podman/native and proxy configuration
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Exercise boot/startup persistence, volume permissions, HTTPS/WebSockets, upgrade and backup/restore for each variant.
- Record Podman/systemd/network/storage versions and separate results for both supported paths.

## Boundary and failure cases

Docker success cannot replace either Podman path; no unsolicited changes to real host services.

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
