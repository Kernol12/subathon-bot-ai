# VALIDATE-005 — Benchmark both scene renderers and missed-frame timing

Parent: [EPIC-026](../EPIC-026.md). Kind: environment.
Specification: v0.45; §11, §19, MEDIA-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Native/browser renderer 1080p 60-FPS target and optional 30-FPS performance runs on recorded graphics hardware.

Primary module paths: tests/ integration/platform/performance evidence
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Measure representative static/media/animated scenes under asset budgets with separate native/browser frame/memory results.
- Verify skipped frames retain elapsed-time durations and resize modes; record demanding-scene warnings and unsupported profiles.

## Boundary and failure cases

Targets are not guarantees; actual OBS/compositor results must be preserved separately from normal browser/native tests.

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
