# STREAMELEMENTS-090 — Verify Astro events evidence and gap behavior live

Parent: [EPIC-014](../EPIC-014.md). Kind: environment.
Specification: v0.45; §14, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Authorized Astro test-channel validation and descriptor audit.

Primary module paths: integrations/ StreamElements
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Capture actual completed-tip/moderation/transaction behavior and reconnect limitations with redacted evidence.
- Reconcile declared support against feasibility results and leave unavailable/unverified refund/recovery cases explicit.

## Boundary and failure cases

No default real tips/refunds or outbound chat; unsupported fixtures cannot substitute for live evidence.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: CTest adapter tests with fake Astro transport and evidenced payment fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
