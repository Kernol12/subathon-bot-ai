# EPIC-026 — Target platform/browser/OBS matrix and performance evidence

Module: Release. Kind: epic. Specification: v0.45; §19.
Authoritative status, owners and dependencies: progress/state.json.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| [VALIDATE-001](EPIC-026/VALIDATE-001.md) | Maintain complete requirement and integration evidence matrix |
| [VALIDATE-002](EPIC-026/VALIDATE-002.md) | Run combined core accounting recovery and security scenarios |
| [VALIDATE-003](EPIC-026/VALIDATE-003.md) | Run supported browser and client compatibility matrix |
| [VALIDATE-004](EPIC-026/VALIDATE-004.md) | Benchmark server eligible-input latency and viewer load |
| [VALIDATE-005](EPIC-026/VALIDATE-005.md) | Benchmark both scene renderers and missed-frame timing |
| [VALIDATE-006](EPIC-026/VALIDATE-006.md) | Consolidate platform package capture and restore release gates |

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
