# EPIC-008 — Qt/Drogon API server, durable intake and live snapshots

Module: Server/API. Kind: epic. Specification: v0.45; §2, §3.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `APITEST-001` → [API-001](EPIC-008/API-001.md) | Production Qt and Drogon lifecycle bridge |
| `APITEST-002` → [API-002](EPIC-008/API-002.md) | HTTP command schemas exact values and revision checks |
| `APITEST-003` → [API-003](EPIC-008/API-003.md) | Authorized WebSocket snapshots and resynchronization |
| `APITEST-004` → [API-004](EPIC-008/API-004.md) | Capability handshake and matching web assets |
| `APITEST-005` → [API-005](EPIC-008/API-005.md) | Common connector lifecycle and capability contracts |
| `APITEST-006` → [API-006](EPIC-008/API-006.md) | Shared catch-up baseline import and overlap policies |
| `APITEST-007` → [API-007](EPIC-008/API-007.md) | Persisted resumable setup and isolated trial runs |
| `APITEST-008` → [API-008](EPIC-008/API-008.md) | Early fake-event to committed timer and API vertical slice |
| `APITEST-009` → [API-009](EPIC-008/API-009.md) | Integrate review modifiers reversals milestones and recovery |

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
