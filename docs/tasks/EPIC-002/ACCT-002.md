# ACCT-002 — Independent eligibility and confirmation linkage

Parent: [EPIC-002](../EPIC-002.md). Kind: implementation.
Specification: v0.45; §3, §5, §14, §18; settlement boundary §8.
Status, owners and dependency IDs: progress/state.json (authoritative).
Contract revision: [contribution eligibility revision 1](../../contracts/contribution-eligibility-v1.md),
authored for ACCTTEST-002; separate review and actual intended red remain required.

## Scope and ownership

Independent contribution dimensions and pure blocker assessment, plus a narrow
writer-side association/evidence service over already committed receipts and explicit
caller-verified activity identities. Preserve application/reversal/settlement markers;
this slice never calculates/applies effects, advances application/reversal or builds
the full ledger. ACCT-004 owns atomic application, ACCT-005 review operations and
ACCT-006 settlement/refund-before-apply reconciliation. Qt/SQL/provider behavior
remains under the appropriate API/storage/connector baselines.

Author owns new `core/include/subathon/core/contribution.hpp` and explicit stub
`core/src/contribution.cpp`; server `contribution_evidence.hpp/.cpp`; acceptance
`tests/acceptance/contribution_evidence_tests.cpp` and primitive fake
`tests/fakes/contribution_repository.hpp`; revision-1 contract, this brief and
`docs/evidence/accttest-002-authoring.md`. Coordinator owns all build/presets/pins,
state/HTML/map/checkpoint, CI integration and publication. No shared schema is changed.

Primary module paths: core/ and server/ application services
Read root/scoped AGENTS.md before editing. Shared schemas, migrations, dependency pins,
root build files and progress records require explicit integrator ownership in the claim.
Adjacent slices retain their assigned responsibilities; do not implement the whole epic.

## Acceptance criteria

- Hold multiple unresolved conditions simultaneously; accepting paused input cannot bypass missing evidence or valuation.
- Link later payment confirmation to the original contribution without creating a second award; support labeled provider-report/review/record-only policy.
- Keep explicit evidence-policy and decision/actor references separate from ordinary
  review; never relabel unknown/unconfirmed as confirmed through report admission.
- Missing monetary source amount remains absent and independently blocks valuation;
  a resolution reference cannot invent zero or remove a stated missing-rate hold.
- Preserve first support receipt/context and all other conditions when linking
  confirmations; unmatched confirmations remain retained until verified matching
  support exists. Exact same-receipt observations are idempotent; distinct receipts
  retain evidence. No names/amount/time/raw payment-reference guessing.
- Hold unresolved settlement and never-applied partial/full reversal for ACCT-006;
  already-applied input never becomes another planning candidate after reversal.
- Atomically commit association/evidence through primitive repository operations;
  known injected failures preserve earlier evidence and permit retry. Fake storage
  cannot supply service deduplication or evidence decisions.

## Boundary and failure cases

Cover repeated/out-of-order confirmation, rejected inputs, missing rates/source
amounts, unsupported source fields, captured record-only behavior and unresolved
settlement without inventing certainty. Verified keys are capability inputs, not
claims about real providers; missing/cross-provider correlation stays API-006 work.
The contract explicitly records local out-of-order support anchoring, conservative
confirmation projection and retrospective policy-override boundaries for review.

## Separate baseline and implementation tasks

- `ACCTTEST-002` (test_baseline): refine this slice's contract/examples against the
  specification, author independently derived tests, obtain a different reviewer's
  approval, then record a real intended red run from explicit missing behavior.
  Compilation failure, missing dependencies or skipping tests does not count.
- `ACCT-002` (implementation): depends on that completed baseline and its other
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
