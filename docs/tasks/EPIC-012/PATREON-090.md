# PATREON-090 — Verify v2 payment identifiers charging refunds and history live

Parent: [EPIC-012](../EPIC-012.md). Kind: environment.
Specification: v0.45; §14, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Authorized Patreon test-account/campaign evidence separate from fake regression fixtures.

Primary module paths: integrations/ Patreon
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Establish actual payment-state transitions/identifiers and webhook-history correlation with redacted source documentation/captures.
- Record coverage/limits of refund/completeness/recovery and revise capability descriptors without inventing support.

## Boundary and failure cases

If provider tooling cannot exercise a required case, retain that gate explicitly; no real pledge processing by default tests.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: CTest adapter tests with fake HTTP/webhooks and reviewed payment fixtures.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
