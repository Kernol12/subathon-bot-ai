# EPIC-002 — Contribution ledger, review, partial/full reversals and deduplication

Module: Accounting. Kind: epic. Specification: v0.45; §3, §5, NUM-03.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `ACCTTEST-001` → [ACCT-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-002/ACCT-001.md) | Normalized receipt identity and duplicate-safe ingestion |
| `ACCTTEST-002` → [ACCT-002](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-002/ACCT-002.md) | Independent eligibility and confirmation linkage |
| `ACCTTEST-003` → [ACCT-003](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-002/ACCT-003.md) | Captured application plans and calculation provenance |
| [ACCTCONTRACT-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/ACCTCONTRACT-001.md) | Logical application-service/current-policy/commit and durable-hold semantics |
| `ACCTTEST-004` → [ACCT-004](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-002/ACCT-004.md) | Atomic effect application and operation idempotency |
| `ACCTTEST-005` → [ACCT-005](https://github.com/Kernol12/subathon-bot-ai/issues/197) | Individual batch and silent contribution review |
| `ACCTTEST-006` → [ACCT-006](https://github.com/Kernol12/subathon-bot-ai/issues/199) | Cumulative partial and full reversal entitlements |
| `ACCTTEST-007` → [ACCT-007](https://github.com/Kernol12/subathon-bot-ai/issues/201) | Apply reversals with debt and preserved progression |

## Completion and integration

Completion requires the listed baseline/implementation/verification outcomes, retained
foundation capabilities and closure evidence; GitHub owns prerequisite relationships.
Planned tasks are not authored
tests, reviewed contracts, implementations or verified capabilities.

Claim a dependency-ready child on its own branch, read its brief and scoped instructions,
and record shared-file ownership. Keep separate independent test/code review roles and
the existing [test strategy](../TEST_STRATEGY.md). No assertion weakening or simulated
provider/platform support is permitted. Missing live/platform evidence stays an explicit
gate; it does not remove a release requirement. Integrate focused PRs, update task evidence
and record the handoff on GitHub with canonical proof references.
