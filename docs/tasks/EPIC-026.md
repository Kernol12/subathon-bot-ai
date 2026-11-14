# EPIC-026 — Target platform/browser/OBS matrix and performance evidence

Module: Release. Kind: epic. Specification: v0.45; §19.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| [VALIDATE-001](https://github.com/Kernol12/subathon-bot-ai/issues/535) | Maintain complete requirement and integration evidence matrix |
| [VALIDATE-002](https://github.com/Kernol12/subathon-bot-ai/issues/536) | Run combined core accounting recovery and security scenarios |
| [VALIDATE-003](https://github.com/Kernol12/subathon-bot-ai/issues/537) | Run supported browser and client compatibility matrix |
| [VALIDATE-004](https://github.com/Kernol12/subathon-bot-ai/issues/538) | Benchmark server eligible-input latency and viewer load |
| [VALIDATE-005](https://github.com/Kernol12/subathon-bot-ai/issues/539) | Benchmark both scene renderers and missed-frame timing |
| [VALIDATE-006](https://github.com/Kernol12/subathon-bot-ai/issues/540) | Consolidate platform package capture and restore release gates |

## Completion and integration

Completion requires the listed baseline/implementation/verification outcomes, retained
foundation capabilities and closure evidence; GitHub owns prerequisite relationships.
Planned tasks are not authored
tests, reviewed contracts, implementations or verified capabilities.

Claim a dependency-ready child on its own branch, read its brief and scoped instructions,
and record shared-file ownership. Keep separate independent test/code review roles and
the existing [test strategy](../TEST_STRATEGY.md). No assertion weakening or simulated
provider/platform support is permitted. Missing live/platform evidence stays an explicit
gate; it does not remove a release requirement. Integrate focused PRs, update task evidence
and record the handoff on GitHub with canonical proof references.
