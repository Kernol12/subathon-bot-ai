# EPIC-008 — Qt/Drogon API server, durable intake and live snapshots

Module: Server/API. Kind: epic. Specification: v0.45; §2, §3.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| [APICONTRACT-001](EPIC-008/APICONTRACT-001.md) | Actual host lifecycle/current scope and native persistent WS profile contract |
| `APITEST-012` → [API-012](EPIC-008/API-012.md) | Admitted native control/fragment/Close lifecycle prerequisite |
| [APICONTRACT-005](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-008/APICONTRACT-005.md) → `APITEST-016` → [API-016](EPIC-008/API-016.md) | Generic bounded HTTP SDK foundation, separate from account capture and host authority |
| [APICONTRACT-004](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-008/APICONTRACT-004.md) → [APITEST-015](EPIC-008/APITEST-015.md) | Resolve isolated native listener versus HTTP host baseline conflict before API-001 topology changes |
| `APITEST-001` → [API-001](EPIC-008/API-001.md) | Production Qt and Drogon lifecycle bridge |
| [APICONTRACT-002](EPIC-008/APICONTRACT-002.md) | Separate bounded authentication-continuation host extension contract |
| `APITEST-013` → [API-013](https://github.com/Kernol12/subathon-bot-ai/issues/600) | Host continuation/lease, guarded auth storage, elapsed and password permit prerequisites |
| [APICONTRACT-003](EPIC-008/APICONTRACT-003.md) | Guarded enrollment/recovery host-write and workflow contract prerequisite |
| `APITEST-014` → [API-014](https://github.com/Kernol12/subathon-bot-ai/issues/603) | Guarded setup/reset writes and finite shared-permit profiles |
| `APITEST-002` → [API-002](https://github.com/Kernol12/subathon-bot-ai/issues/280) | HTTP command schemas exact values and revision checks |
| `APITEST-003` → [API-003](https://github.com/Kernol12/subathon-bot-ai/issues/282) | Authorized WebSocket snapshots and resynchronization |
| `APITEST-004` → [API-004](https://github.com/Kernol12/subathon-bot-ai/issues/284) | Capability handshake and matching web assets |
| `APITEST-005` → [API-005](https://github.com/Kernol12/subathon-bot-ai/issues/286) | Common connector lifecycle and capability contracts |
| `APITEST-006` → [API-006](https://github.com/Kernol12/subathon-bot-ai/issues/288) | Shared catch-up baseline import and overlap policies |
| `APITEST-007` → [API-007](https://github.com/Kernol12/subathon-bot-ai/issues/290) | Persisted resumable setup and isolated trial runs |
| `APITEST-008` → [API-008](https://github.com/Kernol12/subathon-bot-ai/issues/292) | Early fake-event to committed timer and API vertical slice |
| `APITEST-009` → [API-009](https://github.com/Kernol12/subathon-bot-ai/issues/294) | Integrate review modifiers reversals milestones and recovery |

| [CONTRACT-013](CONTRACT-013.md) | Bounded reference HTTP package; complete HTTP006/scene gates retained |
| [CONTRACT-014](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/CONTRACT-014.md) | Single-rule/current-adapter contract before reference behavior |
| `APITEST-010` → [API-010](https://github.com/Kernol12/subathon-bot-ai/issues/560) | Real reference HTTP negotiation and authorized committed snapshot |
| `APITEST-011` → [API-011](https://github.com/Kernol12/subathon-bot-ai/issues/562) | Synthetic intake, actual current adapters and operation-time atomic timer state |

[ADR-0006](../decisions/0006-reference-path-prerequisites.md) separates the early
§17 path from complete scene/milestone/modifier presentation, full transports and
recovery. All replacement guarantees must actually finish; planning creates no
runtime capability or ready implementation. See also
[RULES-006](https://github.com/Kernol12/subathon-bot-ai/issues/558).

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
