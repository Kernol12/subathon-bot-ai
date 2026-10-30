# EPIC-002 — Contribution ledger, review, partial/full reversals and deduplication

Module: Accounting. Kind: epic. Specification: v0.45; §3, §5, NUM-03.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `ACCTTEST-001` → [ACCT-001](EPIC-002/ACCT-001.md) | Normalized receipt identity and duplicate-safe ingestion |
| `ACCTTEST-002` → [ACCT-002](EPIC-002/ACCT-002.md) | Independent eligibility and confirmation linkage |
| `ACCTTEST-003` → [ACCT-003](EPIC-002/ACCT-003.md) | Captured application plans and calculation provenance |
| `ACCTTEST-004` → [ACCT-004](EPIC-002/ACCT-004.md) | Atomic effect application and operation idempotency |
| `ACCTTEST-005` → [ACCT-005](EPIC-002/ACCT-005.md) | Individual batch and silent contribution review |
| `ACCTTEST-006` → [ACCT-006](EPIC-002/ACCT-006.md) | Cumulative partial and full reversal entitlements |
| `ACCTTEST-007` → [ACCT-007](EPIC-002/ACCT-007.md) | Apply reversals with debt and preserved progression |

## Completion and integration

The epic depends on every listed baseline/implementation/verification child plus its
retained foundation prerequisites in progress/state.json. Marking it done requires all
of those dependencies done and a closure evidence note. Planned tasks are not authored
tests, reviewed contracts, implementations or verified capabilities.

Claim a dependency-ready child on its own branch, read its brief and scoped instructions,
and record shared-file ownership. Keep separate independent test/code review roles and
the existing [test strategy](../TEST_STRATEGY.md). No assertion weakening or simulated
provider/platform support is permitted. Missing live/platform evidence stays an explicit
gate; it does not remove a release requirement. Integrate focused PRs, update task evidence
and regenerate PROGRESS.html before handoff.
