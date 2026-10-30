# EPIC-004 — Timed modifiers, stacking and arrival-time context

Module: Automation. Kind: epic. Specification: v0.45; §7.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `MODIFIERTEST-001` → [MODIFIER-001](EPIC-004/MODIFIER-001.md) | Persistent modifier definitions and activation instances |
| `MODIFIERTEST-002` → [MODIFIER-002](EPIC-004/MODIFIER-002.md) | Independent modifier clocks and lifecycle behavior |
| `MODIFIERTEST-003` → [MODIFIER-003](EPIC-004/MODIFIER-003.md) | Per-group and per-target stacking operators |
| `MODIFIERTEST-004` → [MODIFIER-004](EPIC-004/MODIFIER-004.md) | Previewed edits to active instances |
| `MODIFIERTEST-005` → [MODIFIER-005](EPIC-004/MODIFIER-005.md) | Captured modifier context in pending contribution plans |
| `MODIFIERTEST-006` → [MODIFIER-006](EPIC-004/MODIFIER-006.md) | Persist and recover modifier clocks with input holds |

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
