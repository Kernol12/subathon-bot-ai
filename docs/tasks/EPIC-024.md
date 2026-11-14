# EPIC-024 — Docker Compose, Podman Quadlet, native Linux and Caddy

Module: Distribution. Kind: epic. Specification: v0.45; §16.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `DEPLOYMENTTEST-001` → [DEPLOYMENT-001](https://github.com/Kernol12/subathon-bot-ai/issues/509) | Common application image storage and secret layout |
| `DEPLOYMENTTEST-002` → [DEPLOYMENT-002](https://github.com/Kernol12/subathon-bot-ai/issues/511) | Caddy defaults and existing reverse proxy contract |
| `DEPLOYMENTTEST-003` → [DEPLOYMENT-003](https://github.com/Kernol12/subathon-bot-ai/issues/513) | Docker Compose lifecycle and persistent services |
| `DEPLOYMENTTEST-004` → [DEPLOYMENT-004](https://github.com/Kernol12/subathon-bot-ai/issues/515) | Rootless Podman Quadlet default deployment |
| `DEPLOYMENTTEST-005` → [DEPLOYMENT-005](https://github.com/Kernol12/subathon-bot-ai/issues/517) | Rootful Podman with containerized Caddy alternative |
| `DEPLOYMENTTEST-006` → [DEPLOYMENT-006](https://github.com/Kernol12/subathon-bot-ai/issues/519) | Native Linux server archive and systemd installation |
| [DEPLOYMENT-090](https://github.com/Kernol12/subathon-bot-ai/issues/520) | Verify Docker persistence HTTPS upgrade and recovery |
| [DEPLOYMENT-091](https://github.com/Kernol12/subathon-bot-ai/issues/521) | Verify both Podman Quadlet deployment variants |
| [DEPLOYMENT-092](https://github.com/Kernol12/subathon-bot-ai/issues/522) | Verify native Ubuntu service install update and restore |

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
