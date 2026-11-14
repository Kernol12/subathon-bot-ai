# EPIC-007 — Session/credential encryption, keys and remembered login

Module: Identity. Kind: epic. Specification: v0.45; §10, §16.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| [SESSIONCONTRACT-001](EPIC-007/SESSIONCONTRACT-001.md) | Reviewed current-session supplier and token lifecycle document |
| [SESSIONCONTRACT-002](EPIC-007/SESSIONCONTRACT-002.md) | Reviewed browser/desktop credential transport document |
| [SESSIONCONTRACT-007](EPIC-007/SESSIONCONTRACT-007.md) | Additive AUTH transport-budget producer and edge-anchor contract |
| `SESSIONTEST-012` → [SESSION-012](https://github.com/Kernol12/subathon-bot-ai/issues/606) | Actual AUTH remaining-budget producer and both proxy-edge admissions |
| `SESSIONTEST-001` → [SESSION-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-007/SESSION-001.md) | Password hashing and parameter upgrades |
| `SESSIONTEST-002` → [SESSION-002](https://github.com/Kernol12/subathon-bot-ai/issues/262) | Opaque sessions expiry revocation and freshness |
| `SESSIONTEST-003` → [SESSION-003](https://github.com/Kernol12/subathon-bot-ai/issues/264) | Browser cookie CSRF and desktop header transport |
| `SESSIONTEST-004` → [SESSION-004](https://github.com/Kernol12/subathon-bot-ai/issues/266) | Windows DPAPI remembered-login storage |
| `SESSIONTEST-005` → [SESSION-005](https://github.com/Kernol12/subathon-bot-ai/issues/268) | Linux QtKeychain remembered-login fallback |
| `SESSIONTEST-006` → [SESSION-006](https://github.com/Kernol12/subathon-bot-ai/issues/270) | Encrypted provider credentials and protected key provisioning |
| `SESSIONTEST-007` → [SESSION-007](https://github.com/Kernol12/subathon-bot-ai/issues/272) | Crash-safe credential encryption-key rotation |

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

## ADR-0010 additive session prerequisites

[Session executable decomposition](../decisions/0010-session-executable-prerequisites.md)
preserves all original scope and test-first/release gates. The authoritative task
graph adds the relevant supplier/lifecycle/store/local-login/clock/ingress prerequisites.
These components do not complete this task's own full acceptance or real integration.
