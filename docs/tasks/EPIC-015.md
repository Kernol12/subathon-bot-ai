# EPIC-015 — Ko-fi payment webhooks and deduplication

Module: Integrations. Kind: epic. Specification: v0.45; §14.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `KOFITEST-001` → [KOFI-001](EPIC-015/KOFI-001.md) | Authenticated payment webhook receipt and deduplication |
| `KOFITEST-002` → [KOFI-002](EPIC-015/KOFI-002.md) | Tip membership and other payment eligibility mapping |
| [KOFI-003](EPIC-015/KOFI-003.md) | Establish payment refund and history capability evidence |
| [KOFI-090](EPIC-015/KOFI-090.md) | Verify installation webhook eligibility and duplicate handling live |

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
