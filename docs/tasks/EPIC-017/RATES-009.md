# RATES-009 — Exact rate snapshots in retained historical valuations

Parent: [EPIC-017](../EPIC-017.md). Kind: implementation.
Specification: v0.45; §3.5, §9, NUM-04, FX-06.
Status, owners and dependencies: progress/state.json.

## Scope and concrete gap

Existing HistoricalValuation.effective_rate_decimal and STORAGE-015 retain a
finite decimal spelling. The exact inverse of1.20 is5/6, which has no finite
decimal spelling. A snapshot-to-valuation adapter cannot round it or silently
report this available rate as missing. Define an explicit reviewed versioned bridge
into captured valuations, relevant command/value codecs and durable plan history.
Preserve original observations separately from exact effective numerator/denominator
and snapshot/override provenance. Audit existing contracts, callers, byte budgets
and migration/compatibility needs before changing representations.

## Acceptance criteria

- Preserve original1.20 and exact inverse5/6 through capture, codecs, durable
  write/read and fresh repository/process reopening, with literal independent tests.
- Convert exactly and apply NUM-04 final-effect rounding without binary floating
  point, decimal approximation or intermediate rounding.
- Rate/override/reporting-currency changes preserve historical revisions, credited
  effects and reversal provenance. Unknown metadata stays unknown.
- Invalid rational values, missing valuation, unsupported old representation and
  resource/commit failures have explicit bounded outcomes, without fabricated
  accepted decimals or partially committed awards.
- Preserve existing reviewed assertions; any genuine contract/schema change needs
  written specification reasoning, separate review and a revised recorded baseline.

## Contract prerequisite

[RATECONTRACT-001](RATECONTRACT-001.md) separates the reviewed logical exact-rate,
provenance and compatibility obligations from this slice's executable baseline.
Its [proposal](../../contracts/exact-retained-valuations-v1.md) supplies document
proof only. [RATECONTRACT-002](RATECONTRACT-002.md) must define and obtain
separate approval for the concrete API/codec/schema contract before the baseline.
Revised assertions, intended red and implementation remain downstream. Existing contracts and shipped
migrations remain unchanged by that prerequisite.

## Separate baseline and implementation

RATESTEST-009 authors independent examples/tests against the actual reviewed model,
gets separate review and observes intended behavioral red with valid setup.
RATES-009 then implements that baseline, obtains different code review and runs
relevant exact-domain/codec/real-SQL/application regression and acceptance checks.
Extend and observe applicable CI. Dependency/compile failures and skips are not red.
No new contract/code/schema/fixture/test is supplied by this planning brief.

This is a required currency release-epic child. Pure RATES-001 model completion
cannot establish this bridge. Provider operations, pin solving, scheduling,
reporting UI and all three providers remain separate required work.

The controlled parent-table rebuild dependency is explicit:
[STORAGECONTRACT-003](../EPIC-009/STORAGECONTRACT-003.md) →
[STORAGETEST-021](../EPIC-009/STORAGETEST-021.md) →
[STORAGE-021](../EPIC-009/STORAGE-021.md). RATESTEST-009 requires that actual
writer/catalog capability before its real-SQL baseline; no unsupported PRAGMA or
unproven populated migration is assumed by this bridge. All three remain planned
under the contract task's one-task stop boundary.

STORAGE-021 owns the concrete new schema MigrationStep,
exact_valuation_repository_manifest() factory, expected-schema projection and
populated legacy upgrade/reopen/old-catalog-refusal proof. RATESTEST-009 consumes
that shipped schema and authors new exact value/write/read/mixed-history tests;
RATES-009 owns exact plan/bundle/hold/reference dispatch and consumes the same
factory. It does not ship a competing migration step or waive lifecycle admission.
