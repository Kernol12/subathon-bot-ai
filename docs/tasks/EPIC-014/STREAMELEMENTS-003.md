# STREAMELEMENTS-003 — Establish refund and gap-recovery capability boundaries

Parent: [EPIC-014](../EPIC-014.md). Kind: feasibility.
Specification: v0.45; §14, INTAKE-01, INTAKE-02.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Bounded protocol/documentation experiment for history/refund/recovery limits; no invented production API.

Primary module paths: integrations/ StreamElements
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record current official documentation and reproducible redacted experiments for identifiers, refund notifications and available history.
- Define a reviewed descriptor/policy for each established supported or unavailable path before dependent implementation.

## Boundary and failure cases

Unknown remains blocked/unverified; a failed experiment is not proof of permanent absence.

## Feasibility gate

Use current official documentation and a bounded reproducible experiment. Record
versions, sanitized inputs/output, limits and supported versus unresolved capabilities.
Do not promote spike code or undocumented behavior into production. Missing access or
inconclusive results remain blocked/unverified; revise dependent contracts only with
evidence and review. No production implementation is assigned by this task.

## Checks and handoff

Primary home: CTest adapter tests with fake Astro transport and evidenced payment fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
