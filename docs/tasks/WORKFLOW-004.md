# WORKFLOW-004 — Bounded coordination policy and implementation split

Kind: contract. Specification v0.45 §§17–18. Depends on WORKFLOW-003.
The owner authorized the seven improvements from the workflow investigation on
12 November 2026. This authorization covers WORKFLOW-004–007 and CI-002 through
reviewed implementation and protected publication, then stop. Product tasks,
including API-001/API-016, remain stopped. Status and assignments live in progress.

## Scope and ownership

The coordinator authors the forward policy and focused briefs, owns shared progress,
schema/build/CI integration and publication, and obtains independent review. This
task changes documentation/planning only. It neither executes new tests/native
consumers nor changes existing approvals, package trust or repository protections.

Normative packet: [bounded coordination v2](../contracts/bounded-coordination-v2.md)
and the briefs for [WORKFLOW-005](WORKFLOW-005.md),
[WORKFLOW-006](WORKFLOW-006.md), [WORKFLOW-007](WORKFLOW-007.md) and [CI-002](CI-002.md).
The contract owns the forward policy; each implementation owns its detailed API,
independently derived tests, intended red and fresh implementation session. Substantial
new prerequisites require an explicit task before expansion.

## Acceptance and gates

- Cover unified artifact-aware approval records, a reusable execution runner, narrow
  results-only review, publication checks/privacy, compact context/parked branches,
  capability compatibility and CI diagnostics/enforcement.
- Preserve v0.45, all existing assertions, historical receipts/120-task exemptions,
  actual author/reviewer independence and current source review gates.
- Define exact eligibility and invalidation for the future results-only path; no
  implicit migration or retroactive reopening/waiver of existing task-specific gates.
- Keep ordinary tooling standard-library-only; reuse admitted schema validators.
- Record bounded scopes, dependencies, examples, path owners and actual evidence.

Review this substantive contract under the existing Claude initial → Codex fixes /
separate verification → fresh Claude closing cycle. Check project/progress/schema
consistency and latest-head CI; publish a focused PR and merge normally. The future
policy activates only with WORKFLOW-006's reviewed tooling/guidance implementation.
Claim WORKFLOW-005 from refreshed main after this contract is merged.
