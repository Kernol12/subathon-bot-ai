# EPIC-003 — Rule builder, exclusions, generated events and simulator

Module: Automation. Kind: epic. Specification: v0.45; §6.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `RULESTEST-001` → [RULES-001](EPIC-003/RULES-001.md) | Versioned rule definitions and typed conditions |
| `RULESTEST-002` → [RULES-002](EPIC-003/RULES-002.md) | Exclusive groups and directional exclusions |
| `RULESTEST-003` → [RULES-003](EPIC-003/RULES-003.md) | Ordered action planning and delegated capabilities |
| `RULESTEST-004` → [RULES-004](EPIC-003/RULES-004.md) | Bounded generated-event processing and parentage |
| `RULESTEST-005` → [RULES-005](EPIC-003/RULES-005.md) | Reusable fake adapters and inspectable simulator |

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
