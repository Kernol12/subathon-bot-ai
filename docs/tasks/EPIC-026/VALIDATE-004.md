# VALIDATE-004 — Benchmark server eligible-input latency and viewer load

Parent: [EPIC-026](../EPIC-026.md). Kind: environment.
Specification: v0.45; §16, PERF-01.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Reproducible load harness/run on recorded 2-vCPU/2-GiB/local-SSD target environment.

Primary module paths: tests/ integration/platform/performance evidence
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Measure 500 viewers/10 management clients/5 overlays, 5 eligible inputs/s sustained and 50/s for 30-second bursts.
- Report eligible receipt-to-internal-commit p95 against one-second target, excluding provider latency/review holds/scheduled announcements.

## Boundary and failure cases

Do not invent measured minimum requirements or use excluded delays to hide failures; record CPU/storage/network/load setup.

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
