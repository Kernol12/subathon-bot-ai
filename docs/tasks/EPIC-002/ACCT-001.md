# ACCT-001 — Normalized receipt identity and duplicate-safe ingestion

Parent: [EPIC-002](../EPIC-002.md). Kind: implementation.
Specification: v0.45; §3, §14, §18.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: [receipt intake revision 1](../../contracts/receipt-intake-v1.md),
authored for ACCTTEST-001; independent approval and observed red remain required.

## Scope and ownership

Core receipt identity/facts/arrival values plus the server-owned generic intake
service and atomic repository port. No contribution/payment eligibility or event
correlation behavior is implemented in this slice; ACCT-002 owns those dimensions.
SQL uniqueness/durability/concurrency and the concrete writer adapter belong to
STORAGE-002; Qt command/writer integration remains an API/storage acceptance gate.

The ACCTTEST-001 author owns `core/include/subathon/core/receipt.hpp`,
`server/include/subathon/server/receipt_intake.hpp`, explicit stub
`server/src/receipt_intake.cpp`, `tests/acceptance/receipt_intake_tests.cpp`,
`tests/fakes/receipt_repository.hpp`, the revision-1 contract, this brief and
`docs/evidence/accttest-001-authoring.md`. The coordinator owns build/presets/pins,
progress/HTML/acceptance map, shared checkpoint, CI integration and publication.

Primary module paths: core/ and server/ application services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Key already validated observations by provider, stable source/account, documented
  delivery/event identifier namespace and opaque ID; retain connections as attempt
  provenance so reconnect cannot erase deduplication.
- First arrival commits one owned original snapshot and initial attempt; source time
  remains distinct from server arrival and retained versioned configuration/context.
- Exact normalized-facts retry returns the existing receipt ID and appends an attempt
  with its own arrival/connection/context. Meaningful changed facts return conflict,
  retaining original and conflicting observations. Revision 1 compares every field
  of the bounded projection, including absent/present values and source precision.
- Stage each intake atomically through the repository port; write/known commit
  failures expose no partial data or successful result, preserve earlier evidence and
  permit retry. The primitive fake must not implement deduplication decisions.
- Keep payment/activity references separate from receipt identity. Overlapping
  sources require configured authority or verified correlation before credit.

## Boundary and failure cases

Equal names/amounts/timestamps are insufficient identity; unknown fields remain
unknown and conflicting duplicate payloads retain audit evidence. Missing stable
IDs, cross-provider authority/correlation and provider-specific normalization remain
explicit API-006/connector policy gates; synthetic namespaces cannot establish real
provider capabilities. Service recreation over a fake is not restart/disk evidence.

## Separate baseline and implementation tasks

- `ACCTTEST-001` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCT-001` (implementation): depends on that completed baseline and its other
  recorded prerequisites. Preserve reviewed assertions, implement only this slice,
  run its relevant acceptance/regression checks and obtain separate code review.

Record baseline author/reviewer/revision and red command/output in evidence before
production code; record implementation acceptance and code-review identity separately.
Use separate role sessions/PRs as needed. A single author cannot approve either of
its own independent-review gates. This brief does not declare any tests reviewed.

## Checks and handoff

Primary home: GoogleTest for accounting; Qt Test for writer/service integration.
Select exact suite labels/commands when the reviewed harness exists and record what
actually ran; no unbuilt test target or unrun command is evidence. Update applicable
acceptance-map entries only when tests/evidence are authored or observed. Fixtures must
be synthetic/redacted and default checks must not send chat/process live contributions.
Follow [the child-task workflow](../README.md) for PRs, role handoffs and completion.
