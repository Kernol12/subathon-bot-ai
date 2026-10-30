# EPIC-018 — Qt Quick desktop management and permissions

Module: Desktop. Kind: epic. Specification: v0.45; §2, §10.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `DESKTOPTEST-001` → [DESKTOP-001](EPIC-018/DESKTOP-001.md) | Qt Quick shell login and capability-aware connection |
| `DESKTOPTEST-002` → [DESKTOP-002](EPIC-018/DESKTOP-002.md) | Live timer grace pause and reopening controls |
| `DESKTOPTEST-003` → [DESKTOP-003](EPIC-018/DESKTOP-003.md) | Contribution review and reversal management panels |
| `DESKTOPTEST-004` → [DESKTOP-004](EPIC-018/DESKTOP-004.md) | Structured rule editor and simulation inspection |
| `DESKTOPTEST-005` → [DESKTOP-005](EPIC-018/DESKTOP-005.md) | Modifier configuration and active-instance controls |
| `DESKTOPTEST-006` → [DESKTOP-006](EPIC-018/DESKTOP-006.md) | Milestone editor and action queue controls |
| `DESKTOPTEST-007` → [DESKTOP-007](EPIC-018/DESKTOP-007.md) | Accounts roles invitations sessions and audit management |
| `DESKTOPTEST-008` → [DESKTOP-008](EPIC-018/DESKTOP-008.md) | Provider connection setup health and capability panels |
| `DESKTOPTEST-009` → [DESKTOP-009](EPIC-018/DESKTOP-009.md) | Rate providers reporting currency and override panels |
| `DESKTOPTEST-010` → [DESKTOP-010](EPIC-018/DESKTOP-010.md) | Resumable guided desktop setup and trial events |

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
