# EPIC-006 — Invites, permissions, local/Twitch identities and chat roles

Module: Identity. Kind: epic. Specification: v0.45; §10.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `ACCESSTEST-001` → [ACCESS-001](https://github.com/Kernol12/subathon-bot-ai/issues/245) | Initial owner setup and invitation registration |
| `ACCESSTEST-002` → [ACCESS-002](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-006/ACCESS-002.md) | Effective permissions and complete-grant constraints |
| `ACCESSTEST-003` → [ACCESS-003](https://github.com/Kernol12/subathon-bot-ai/issues/249) | Delegation and authorized automation capabilities |
| [ACCESS-009](https://github.com/Kernol12/subathon-bot-ai/issues/645) | Pure delegation, complete edit impact and dual-channel automation calculations; immediate reference-path prerequisite |
| [ACCESS-010](https://github.com/Kernol12/subathon-bot-ai/issues/646) | Authoritative permission-graph persistence and writer-owned revisions; planned integration prerequisite |
| `ACCESSTEST-004` → [ACCESS-004](https://github.com/Kernol12/subathon-bot-ai/issues/251) | Account login and optional Twitch identity linking |
| `ACCESSTEST-005` → [ACCESS-005](https://github.com/Kernol12/subathon-bot-ai/issues/253) | Recovery codes and administrative recovery |
| `ACCESSTEST-006` → [ACCESS-006](https://github.com/Kernol12/subathon-bot-ai/issues/255) | Two-party ownership transfer and fresh authentication |
| `ACCESSTEST-007` → [ACCESS-007](https://github.com/Kernol12/subathon-bot-ai/issues/257) | Chat command categories arguments and cooldowns |

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
