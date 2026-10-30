# DEPLOYMENT-004 — Rootless Podman Quadlet default deployment

Parent: [EPIC-024](../EPIC-024.md). Kind: implementation.
Specification: v0.45; §16, DEP-07.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Dedicated-account rootless application Quadlet/systemd units with host Caddy.

Primary module paths: deploy/ docker/podman/native and proxy configuration
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Define runtime/network/port/storage/startup-persistence requirements and least-needed service ownership.
- Document start/stop/backup/upgrade/removal and validate persistent data/certificate/key behavior.

## Boundary and failure cases

No podman-compose dependency assumption; test rootless permissions/user services/boot and invalid mount conditions.

## Separate baseline and implementation tasks

- `DEPLOYMENTTEST-004` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `DEPLOYMENT-004` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: Disposable installation/lifecycle tests per deployment; no production host mutations.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
