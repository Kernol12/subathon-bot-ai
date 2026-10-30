# EPIC-001 — Core test plan: exact arithmetic and full timer lifecycle

Module: Core. Kind: epic. Specification: v0.45; §3, §4, TEST-04.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `CORETEST-001` → [CORE-001](EPIC-001/CORE-001.md) | Bounded exact decimal parsing and final-effect rounding |
| `CORETEST-002` → [CORE-002](EPIC-001/CORE-002.md) | Main countdown and ending-mode transitions |
| `CORETEST-003` → [CORE-003](EPIC-001/CORE-003.md) | Independent pause reasons and delayed status signals |
| `CORETEST-004` → [CORE-004](EPIC-001/CORE-004.md) | Recurring schedules and timezone occurrence previews |
| `CORETEST-005` → [CORE-005](EPIC-001/CORE-005.md) | Refund deficit policies and separate time debt |
| `CORETEST-006` → [CORE-006](EPIC-001/CORE-006.md) | Downtime reconstruction and recovery-review decisions |
| `CORETEST-007` → [CORE-007](EPIC-001/CORE-007.md) | Finished-input and audited reopening policies |
| `CORETEST-008` → [CORE-008](EPIC-001/CORE-008.md) | Manual adjustments cap edits and hard-deadline commands |

The original CONTRACT-001 → TEST-001 → REVIEW-001 → TEST-002 → IMPL-001
cap-credit sequence remains unchanged. IMPL-001 is also a required child completion
of this epic; the new tasks extend its scope without duplicating or approving its draft tests.

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
