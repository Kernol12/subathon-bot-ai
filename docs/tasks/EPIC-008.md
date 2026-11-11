# EPIC-008 — Qt/Drogon API server, durable intake and live snapshots

Module: Server/API. Kind: epic. Specification: v0.45; §2, §3.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| [APICONTRACT-001](EPIC-008/APICONTRACT-001.md) | Actual host lifecycle/current scope and native persistent WS profile contract |
| `APITEST-012` → [API-012](EPIC-008/API-012.md) | Admitted native control/fragment/Close lifecycle prerequisite |
| `APITEST-001` → [API-001](EPIC-008/API-001.md) | Production Qt and Drogon lifecycle bridge |
| [APICONTRACT-002](EPIC-008/APICONTRACT-002.md) | Separate bounded authentication-continuation host extension contract |
| `APITEST-013` → [API-013](EPIC-008/API-013.md) | Host continuation/lease, guarded auth storage, elapsed and password permit prerequisites |
| `APITEST-002` → [API-002](EPIC-008/API-002.md) | HTTP command schemas exact values and revision checks |
| `APITEST-003` → [API-003](EPIC-008/API-003.md) | Authorized WebSocket snapshots and resynchronization |
| `APITEST-004` → [API-004](EPIC-008/API-004.md) | Capability handshake and matching web assets |
| `APITEST-005` → [API-005](EPIC-008/API-005.md) | Common connector lifecycle and capability contracts |
| `APITEST-006` → [API-006](EPIC-008/API-006.md) | Shared catch-up baseline import and overlap policies |
| `APITEST-007` → [API-007](EPIC-008/API-007.md) | Persisted resumable setup and isolated trial runs |
| `APITEST-008` → [API-008](EPIC-008/API-008.md) | Early fake-event to committed timer and API vertical slice |
| `APITEST-009` → [API-009](EPIC-008/API-009.md) | Integrate review modifiers reversals milestones and recovery |

| [CONTRACT-013](CONTRACT-013.md) | Bounded reference HTTP package; complete HTTP006/scene gates retained |
| [CONTRACT-014](CONTRACT-014.md) | Single-rule/current-adapter contract before reference behavior |
| `APITEST-010` → [API-010](EPIC-008/API-010.md) | Real reference HTTP negotiation and authorized committed snapshot |
| `APITEST-011` → [API-011](EPIC-008/API-011.md) | Synthetic intake, actual current adapters and operation-time atomic timer state |

[ADR-0006](../decisions/0006-reference-path-prerequisites.md) separates the early
§17 path from complete scene/milestone/modifier presentation, full transports and
recovery. All replacement guarantees must actually finish; planning creates no
runtime capability or ready implementation. See also
[RULES-006](EPIC-003/RULES-006.md).

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
