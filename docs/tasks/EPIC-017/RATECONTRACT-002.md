# RATECONTRACT-002 — Exact valuation API, codec and additive storage contract

Parent: [EPIC-017](../EPIC-017.md). Kind: contract, document only.
Specification v0.45 §§3.5, 9, NUM-04, FX-03/06.

Owner resumed for this ONE document-only task after PR117; finish normal merge, then STOP.

Refine the reviewed [logical bridge](../../contracts/exact-retained-valuations-v1.md)
into concrete bounded DTO/API, error priority, ownership/resource, command/codec and
additive SQL compatibility contracts before RATESTEST-009. Audit actual consumers:
application_plan and application-service valuation sources; STORAGE-015 plan
write/read; bundle readers/size accounting; STORAGE-018 reference capture and
STORAGE-019 structural codecs. References alone do not embed a valuation: prove
which envelopes remain byte-identical and version only genuinely changed meanings.
The no-money reference path does not automatically gain monetary valuation support.

Freeze the logical contract's per-field closure classification: full owned selected
snapshot/manual values and typed shared-snapshot keys versus opaque provider/config/
request/evidence/decision/parent identities. Provider configuration refs do not by
themselves target the application configuration store; future RATES-005/007 typed
external closure needs its own explicit dependencies before capability admission.
Define owned-snapshot revalidation using the actual query API, contribution/plan
resolution consistency, and identity/snapshot/manual basis tags. Assess whether
the 8MiB model value can fit each current statement/plan/bundle budget; lower
admission or immutable shared storage must be explicit, without widening old limits.

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

## Acceptance, ownership and gates

The fresh author owns [the proposed concrete contract](../../contracts/exact-valuation-storage-v1.md)
only. Coordinator owns this brief, navigation, AGENTS.md, shared state, evidence,
generated HTML, checkpoint and PR/CI/merge. Acceptance is a complete concrete
API/value/error contract, literal codec framing, field closure and bounded resource
profiles, proposed additive schema/catalog/legacy dispatch, independently derived
examples and a separately reviewed baseline plan for RATESTEST-009/RATES-009.
Audit the actual current consumers; expose substantial extra prerequisites as
explicit dependency-linked planned tasks before implementation.

No C++/SQL/schema/fixtures/assertions/build/CI/dependencies or provider behavior is
changed by this task. Preserve RATECONTRACT-001 and RATES-001 approvals, all existing
baseline assertions and every shipped migration byte. Full independent Claude
initial → Codex fixes/separate verification → Claude closing review is mandatory.
Document proof is not runtime, authored-test or observed-red proof. Project/progress
validation, render and tooling checks plus latest-head CI, resolved discussions and
normal protected merge remain required. API-001 and actual clock binding stay outside
authorization. Do not start RATESTEST-009, RATES-009 or another task after this merge.
