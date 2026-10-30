# EPIC-005 — Sequential milestones and persisted action delivery

Module: Automation. Kind: epic. Specification: v0.45; §8.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `MILESTONETEST-001` → [MILESTONE-001](EPIC-005/MILESTONE-001.md) | Sequential achievement and deficit calculation |
| `MILESTONETEST-002` → [MILESTONE-002](EPIC-005/MILESTONE-002.md) | Previewed target edits and completed-target corrections |
| `MILESTONETEST-003` → [MILESTONE-003](EPIC-005/MILESTONE-003.md) | Durable delivery-job states and uncertain outcomes |
| `MILESTONETEST-004` → [MILESTONE-004](EPIC-005/MILESTONE-004.md) | Sequential action groups and server presentation timeline |
| `MILESTONETEST-005` → [MILESTONE-005](EPIC-005/MILESTONE-005.md) | Deferred skipped and refund-held action groups |
| `MILESTONETEST-006` → [MILESTONE-006](EPIC-005/MILESTONE-006.md) | Replace unstarted actions and interrupt groups |
| `MILESTONETEST-007` → [MILESTONE-007](EPIC-005/MILESTONE-007.md) | Manual sequential completion without score changes |
| `MILESTONETEST-008` → [MILESTONE-008](EPIC-005/MILESTONE-008.md) | Audited completion reset and replay safeguards |

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
