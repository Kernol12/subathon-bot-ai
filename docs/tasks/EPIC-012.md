# EPIC-012 — Patreon v2 payment evidence, webhooks and reconciliation

Module: Integrations. Kind: epic. Specification: v0.45; §14.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `PATREONTEST-001` → [PATREON-001](EPIC-012/PATREON-001.md) | Server-side authorization and verified webhook intake |
| `PATREONTEST-002` → [PATREON-002](EPIC-012/PATREON-002.md) | Payment evidence confirmation and refund mapping |
| `PATREONTEST-003` → [PATREON-003](EPIC-012/PATREON-003.md) | History reconciliation and reconnect policy |
| [PATREON-090](EPIC-012/PATREON-090.md) | Verify v2 payment identifiers charging refunds and history live |

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
