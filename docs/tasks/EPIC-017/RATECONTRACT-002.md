# RATECONTRACT-002 — Exact valuation API, codec and additive storage contract

Parent: [EPIC-017](../EPIC-017.md). Kind: contract, document only.
Specification v0.45 §§3.5, 9, NUM-04, FX-03/06.
Status, owner and dependencies: progress/state.json. Planned and unassigned.

Refine the reviewed [logical bridge](../../contracts/exact-retained-valuations-v1.md)
into concrete bounded DTO/API, error priority, ownership/resource, command/codec and
additive SQL compatibility contracts before RATESTEST-009. Audit actual consumers:
application_plan and application-service valuation sources; STORAGE-015 plan
write/read; bundle readers/size accounting; STORAGE-018 reference capture and
STORAGE-019 structural codecs. References alone do not embed a valuation: prove
which envelopes remain byte-identical and version only genuinely changed meanings.
The no-money reference path does not automatically gain monetary valuation support.

Define exact field grammar, version/tag admission, canonical rational budgets,
complete provenance closure and immutable durable keys; explicitly bound every
borrowed preflight, copy, read and retained history result. Name concrete ports,
error order, input/output and cumulative budgets. Define append-only schema/catalog
migration and legacy read/new write dispatch, upgrade/reopen/rollback and unknown
commit outcomes. Preserve all shipped migration bytes and old readers' honest
refusal of unsupported versions. No fabricated decimals, REAL conversion,
mutable-current lookup, unsafe partial publication or implicit legacy reinterpretation.

Supply independent examples and the exact separately reviewed baseline revision
plan. Existing old assertions stay in their owning compatibility suites; any
replacement needs specification-based reasoning and a separate approved review.
Obtain the full independent Claude initial → Codex fix/separate verification →
Claude closing cycle before RATESTEST-009. This planned task supplies no approved
ABI/schema/fixture/test/implementation and authorizes no runtime change by itself.
Coordinator owns shared state/build/schema integration. RATES-009 retains actual
capture/durable closure and baseline/implementation gates; RATES-005/007 retain
actual fetched/effective/override stores. All providers and release scope remain.
