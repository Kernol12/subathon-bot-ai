# KOFI-090 — Verify installation webhook eligibility and duplicate handling live

Parent: [EPIC-015](../EPIC-015.md). Kind: environment.
Specification: v0.45; §14, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Authorized installed webhook end-to-end checks with redacted records.

Primary module paths: integrations/ Ko-fi and server/ webhook routing
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Verify retry produces one receipt/contribution and invalid verification produces none.
- Show evidenced category/privacy/gap limitations in setup and confirm no unsupported history/refund claim.

## Boundary and failure cases

Missing live test facilities remain an open gate, not simulation-tested support.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: CTest adapter tests with fake authenticated webhook/retry fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
