# RELEASE-004 — Review final artifacts notices diagnostics and release checklist

Parent: [EPIC-027](../EPIC-027.md). Kind: environment.
Specification: v0.45; §1, §16, §19, LICENSE-01, REPO-01.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Final release manifest and artifact/documentation audit; no automatic public release or repository-visibility change.

Primary module paths: release manifests, server/ diagnostics and docs/
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Reconcile complete intended scope, corresponding source/notices, redacted diagnostics and known capability limitations against evidence.
- Require all preceding release epics and independent reviews before claiming release readiness; publication remains an explicit owner action.

## Boundary and failure cases

Open gates stay release blockers rather than removed requirements; do not publish or change visibility as a side effect of this task.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: Manifest/redaction checks with synthetic data and final artifact/document audit.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
