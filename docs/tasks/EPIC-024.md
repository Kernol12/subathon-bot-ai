# EPIC-024 — Docker Compose, Podman Quadlet, native Linux and Caddy

Module: Distribution. Kind: epic. Specification: v0.45; §16.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `DEPLOYMENTTEST-001` → [DEPLOYMENT-001](EPIC-024/DEPLOYMENT-001.md) | Common application image storage and secret layout |
| `DEPLOYMENTTEST-002` → [DEPLOYMENT-002](EPIC-024/DEPLOYMENT-002.md) | Caddy defaults and existing reverse proxy contract |
| `DEPLOYMENTTEST-003` → [DEPLOYMENT-003](EPIC-024/DEPLOYMENT-003.md) | Docker Compose lifecycle and persistent services |
| `DEPLOYMENTTEST-004` → [DEPLOYMENT-004](EPIC-024/DEPLOYMENT-004.md) | Rootless Podman Quadlet default deployment |
| `DEPLOYMENTTEST-005` → [DEPLOYMENT-005](EPIC-024/DEPLOYMENT-005.md) | Rootful Podman with containerized Caddy alternative |
| `DEPLOYMENTTEST-006` → [DEPLOYMENT-006](EPIC-024/DEPLOYMENT-006.md) | Native Linux server archive and systemd installation |
| [DEPLOYMENT-090](EPIC-024/DEPLOYMENT-090.md) | Verify Docker persistence HTTPS upgrade and recovery |
| [DEPLOYMENT-091](EPIC-024/DEPLOYMENT-091.md) | Verify both Podman Quadlet deployment variants |
| [DEPLOYMENT-092](EPIC-024/DEPLOYMENT-092.md) | Verify native Ubuntu service install update and restore |

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
