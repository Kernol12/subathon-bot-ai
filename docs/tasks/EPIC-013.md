# EPIC-013 — Streamlabs socket, verified payments and history

Module: Integrations. Kind: epic. Specification: v0.45; §14.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `STREAMLABSTEST-001` → [STREAMLABS-001](EPIC-013/STREAMLABS-001.md) | Socket.IO authentication and live receipt normalization |
| `STREAMLABSTEST-002` → [STREAMLABS-002](EPIC-013/STREAMLABS-002.md) | Paginated verified payment history and reconciliation |
| `STREAMLABSTEST-003` → [STREAMLABS-003](EPIC-013/STREAMLABS-003.md) | Gap catch-up first import and capability reporting |
| [STREAMLABS-090](EPIC-013/STREAMLABS-090.md) | Verify Socket.IO live-to-REST payment evidence live |

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
