# EPIC-012 — Patreon v2 payment evidence, webhooks and reconciliation

Module: Integrations. Kind: epic. Specification: v0.45; §14.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `PATREONTEST-001` → [PATREON-001](https://github.com/Kernol12/subathon-bot-ai/issues/353) | Server-side authorization and verified webhook intake |
| `PATREONTEST-002` → [PATREON-002](https://github.com/Kernol12/subathon-bot-ai/issues/355) | Payment evidence confirmation and refund mapping |
| `PATREONTEST-003` → [PATREON-003](https://github.com/Kernol12/subathon-bot-ai/issues/357) | History reconciliation and reconnect policy |
| [PATREON-090](https://github.com/Kernol12/subathon-bot-ai/issues/358) | Verify v2 payment identifiers charging refunds and history live |

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
