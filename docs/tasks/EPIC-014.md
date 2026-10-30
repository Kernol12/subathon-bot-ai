# EPIC-014 — StreamElements Astro tips and recovery limits

Module: Integrations. Kind: epic. Specification: v0.45; §14.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `STREAMELEMENTSTEST-001` → [STREAMELEMENTS-001](EPIC-014/STREAMELEMENTS-001.md) | Astro connection and completed-tip normalization |
| `STREAMELEMENTSTEST-002` → [STREAMELEMENTS-002](EPIC-014/STREAMELEMENTS-002.md) | Payment identity source authority and eligibility |
| [STREAMELEMENTS-003](EPIC-014/STREAMELEMENTS-003.md) | Establish refund and gap-recovery capability boundaries |
| `STREAMELEMENTSTEST-004` → [STREAMELEMENTS-004](EPIC-014/STREAMELEMENTS-004.md) | Capability-aware reconnect and visible gap handling |
| [STREAMELEMENTS-090](EPIC-014/STREAMELEMENTS-090.md) | Verify Astro events evidence and gap behavior live |

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
