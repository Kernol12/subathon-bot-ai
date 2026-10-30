# KOFI-003 — Establish payment refund and history capability evidence

Parent: [EPIC-015](../EPIC-015.md). Kind: feasibility.
Specification: v0.45; §14, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Bounded provider documentation/test-webhook experiment and recovery-limit declaration.

Primary module paths: integrations/ Ko-fi and server/ webhook routing
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record actual message IDs/retry behavior, payment fields and verification mechanism.
- Investigate refund/history coverage and expose unsupported/unverified gaps rather than inventing a history client.

## Boundary and failure cases

Use provider test tools/authorized data; no default live contribution processing.

## Feasibility gate

Use current official documentation and a bounded reproducible experiment. Record
versions, sanitized inputs/output, limits and supported versus unresolved capabilities.
Do not promote spike code or undocumented behavior into production. Missing access or
inconclusive results remain blocked/unverified; revise dependent contracts only with
evidence and review. No production implementation is assigned by this task.

## Checks and handoff

Primary home: CTest adapter tests with fake authenticated webhook/retry fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
