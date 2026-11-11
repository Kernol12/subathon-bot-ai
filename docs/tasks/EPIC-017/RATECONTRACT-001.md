# RATECONTRACT-001 — Exact retained valuation bridge contract

Parent: [EPIC-017](../EPIC-017.md). Kind: contract, document only.
Specification v0.45 §§3.5, 9, NUM-04, FX-03/06.
Status, owner and dependencies: progress/state.json.

## Problem and scope

RATES-001 supplies exact inverse/cross rates, including 5/6 for the inverse of
1.20. Existing HistoricalValuation and STORAGE-015 retain a finite decimal;
RATESTEST-009 cannot safely define its bridge baseline without a reviewed
representation and compatibility contract. Split this prerequisite from
[RATES-009](RATES-009.md) before authoring that baseline. Preserve all existing
release tasks, edges, reviewed assertions and shipped migration bytes.

## Acceptance and ownership

- Audit the existing rate, plan, command/reference-capture, storage and application
  consumers. Name each affected representation and its downstream owner/gate.
- Define a versioned logical exact effective-rate representation and immutable
  observation/snapshot/override provenance, preserving original decimal spellings
  and unknown metadata. Define direction and same-currency/missing behavior.
- Define exact final-effect conversion, retained history and refund invariants,
  bounded validation/failure outcomes and durable compatibility obligations.
- Explain old finite-decimal records versus new rational captures; do not invent
  recurring decimals or silently change frozen codecs/migrations. Expose any
  additional concrete ABI/schema/baseline prerequisites explicitly before code.
- Supply independently derived examples including 1.20 inverse 5/6, cross rates,
  half-even ties, resource/overflow/commit failure, reopening and historical edits.
- Obtain independent Claude initial review, Codex fixes/separate verification and
  Claude closing approval. Document proof is not executed test/runtime proof.

Fresh Codex author owns docs/contracts/exact-retained-valuations-v1.md only.
Coordinator owns this brief, linked RATES-009/epic/navigation references,
AGENTS.md, state, evidence summary, generated HTML and checkpoint.
No C++/SQL/schema/fixture/assertion/build/dependency/provider changes or live calls.
No production implementation or test red is part of this contract-only task.
Relevant checks: project consistency, progress validation/render and tooling tests;
CI coverage and protected merge remain required. Complete only this task, then stop.
