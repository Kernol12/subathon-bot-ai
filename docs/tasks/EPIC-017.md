# EPIC-017 — Frankfurter, ExchangeRate-API and Open Exchange Rates

Module: Currency. Kind: epic. Specification: v0.45; §9, §15.

This epic is a release-scope container, not a whole-epic coding assignment. Its required
children below are bounded assignments; a test-baseline arrow means separate review
and observed intended red must precede the corresponding production implementation.
See [backlog workflow and complete epic index](README.md).

| Tasks (baseline → implementation where applicable) | Bounded outcome |
| --- | --- |
| `RATESTEST-001` → [RATES-001](https://github.com/Kernol12/subathon-bot-ai/blob/f674131464b5907be0ec390b40bd1d1436e36c7e/docs/tasks/EPIC-017/RATES-001.md) | Lossless fetched and effective rate snapshot model |
| `RATESTEST-002` → [RATES-002](https://github.com/Kernol12/subathon-bot-ai/issues/396) | Frankfurter provider adapter and freshness |
| `RATESTEST-003` → [RATES-003](https://github.com/Kernol12/subathon-bot-ai/issues/398) | ExchangeRate-API plan and quota-aware adapter |
| `RATESTEST-004` → [RATES-004](https://github.com/Kernol12/subathon-bot-ai/issues/400) | Open Exchange Rates plan and usage adapter |
| `RATESTEST-005` → [RATES-005](https://github.com/Kernol12/subathon-bot-ai/issues/402) | Persistent sync schedule retries and request accounting |
| `RATESTEST-006` → [RATES-006](https://github.com/Kernol12/subathon-bot-ai/issues/404) | Ordered fallback and source provenance |
| `RATESTEST-007` → [RATES-007](https://github.com/Kernol12/subathon-bot-ai/issues/406) | Consistent manual inverse cross-rate and pin overrides |
| `RATESTEST-008` → [RATES-008](https://github.com/Kernol12/subathon-bot-ai/issues/408) | Historical display conversion and missing valuation review |
| [RATECONTRACT-001](EPIC-017/RATECONTRACT-001.md) | Exact retained valuation bridge contract prerequisite |
| [RATECONTRACT-002](EPIC-017/RATECONTRACT-002.md) | Concrete exact valuation API/codec/additive-storage contract prerequisite |
| `RATESTEST-009` → [RATES-009](https://github.com/Kernol12/subathon-bot-ai/issues/553) | Exact rate snapshots in retained historical valuations |
| [RATES-090](https://github.com/Kernol12/subathon-bot-ai/issues/409) | Verify all three rate provider plan profiles |

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
