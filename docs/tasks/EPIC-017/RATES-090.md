# RATES-090 — Verify all three rate provider plan profiles

Parent: [EPIC-017](../EPIC-017.md). Kind: environment.
Specification: v0.45; §15, §19.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: use the current reviewed slice contract; none is finalized by this plan.

## Scope and ownership

Separate bounded official-documentation and authorized endpoint checks for Frankfurter, ExchangeRate-API and Open Exchange Rates.

Primary module paths: core/ exact currency values and server/ rate/provider services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Record date/plan/endpoints/frequency/quota accounting for each provider; identify unavailable metadata without invented values.
- Validate schedule/cache/fallback decisions against actual limits and redact keys; keep any unexercised plan/history path explicit.

## Boundary and failure cases

Default tests stay fake and offline; paid credentials or unsupported plans are not assumed available.

## Verification gate

Run only the explicitly scoped platform/provider/integration checks in a disposable or
separately authorized environment. Record exact commands, versions, expected versus
observed results, evidence and unresolved gates. Simulation cannot certify a live or
platform claim. Missing hardware/accounts/access remains blocked or not run, not passed.
Do not add/fix product behavior here; return defects to their owning implementation
slice and follow its test/review gates before rerunning verification.

## Checks and handoff

Primary home: GoogleTest for exact rates; Qt Test/fake HTTP for scheduling/cache/adapters.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
