# EPIC-011 — betterplace public integration and aggregate visibility

Module: Integrations. Kind: epic. Specification: v0.45; §14.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `BETTERPLACETEST-001` → [BETTERPLACE-001](EPIC-011/BETTERPLACE-001.md) | Public polling and new-support baseline |
| `BETTERPLACETEST-002` → [BETTERPLACE-002](EPIC-011/BETTERPLACE-002.md) | Separate aggregate observations and private donation fields |
| `BETTERPLACETEST-003` → [BETTERPLACE-003](EPIC-011/BETTERPLACE-003.md) | Polling gap reconciliation and capability descriptors |
| [BETTERPLACE-090](EPIC-011/BETTERPLACE-090.md) | Verify public fields polling identities and visibility live |

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
