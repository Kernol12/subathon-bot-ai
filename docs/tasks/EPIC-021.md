# EPIC-021 — React/TypeScript/Vite management foundation and workflows

Module: Web. Kind: epic. Specification: v0.45; §2, §10.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `WEBAPPTEST-001` → [WEBAPP-001](EPIC-021/WEBAPP-001.md) | React TypeScript Vite shell and static server delivery |
| `WEBAPPTEST-002` → [WEBAPP-002](EPIC-021/WEBAPP-002.md) | Browser login sessions and permission-aware live state |
| `WEBAPPTEST-003` → [WEBAPP-003](EPIC-021/WEBAPP-003.md) | Browser timer lifecycle and recovery operations |
| `WEBAPPTEST-004` → [WEBAPP-004](EPIC-021/WEBAPP-004.md) | Browser contribution review reversals and silent processing |
| `WEBAPPTEST-005` → [WEBAPP-005](EPIC-021/WEBAPP-005.md) | Browser rules and simulator workflows |
| `WEBAPPTEST-006` → [WEBAPP-006](EPIC-021/WEBAPP-006.md) | Browser modifier definition and active-instance workflows |
| `WEBAPPTEST-007` → [WEBAPP-007](EPIC-021/WEBAPP-007.md) | Browser accounts delegation and session management |
| `WEBAPPTEST-008` → [WEBAPP-008](EPIC-021/WEBAPP-008.md) | Browser provider setup and capability health |
| `WEBAPPTEST-009` → [WEBAPP-009](EPIC-021/WEBAPP-009.md) | Browser milestone editing completion and action queue |
| `WEBAPPTEST-010` → [WEBAPP-010](EPIC-021/WEBAPP-010.md) | Browser rate configuration and historical conversion review |
| `WEBAPPTEST-011` → [WEBAPP-011](EPIC-021/WEBAPP-011.md) | Browser resumable setup simulation and start |

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
