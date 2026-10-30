# TIPEEESTREAM-090 — Verify socket history identity and confirmation behavior live

Parent: [EPIC-016](../EPIC-016.md). Kind: environment.
Specification: v0.45; §14, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Authorized current-service protocol/history validation beyond the shared adapter spike.

Primary module paths: integrations/ TipeeeStream
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record concrete protocol/client versions, event/history identifiers, payment labels and observed gaps.
- Verify duplicate-safe installed ingestion and document unproven refund/recovery behavior with redacted evidence.

## Boundary and failure cases

Unavailable credentials or test events leave the gate open; no real supporter data belongs in source or HTML.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: CTest adapter tests with fake compatible Socket.IO/history and reviewed payloads.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
