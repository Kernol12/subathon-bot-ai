# STORAGE-014 — Contribution evidence and receipt-association SQLite adapter

Parent: [EPIC-009](../EPIC-009.md). Specification v0.45: §3.2, §3.4, §3.5, DB-01.
Status/owner/dependencies: progress/state.json. Planned work only; no finalized
slice interface, authored/reviewed tests or implementation is claimed by this brief.

## Scope and ownership

Implement existing contribution/evidence transaction ports on the controlled connection. Persist independent evidence/review/valuation dimensions and immutable original-receipt linkage, not actual contribution effects.

Primary implementation paths: server/ persistence and scoped tests; read root,
server and tests AGENTS.md. The integrator explicitly owns shared CMake/presets,
schemas/migration ordering, progress/HTML and compatibility decisions. Assign exact
files on claim; independent writers use separate branches/worktrees.

## Acceptance criteria

- Retain contribution identity, original source/arrival context, observation links and independent state dimensions atomically.
- Promote committed evidence without overwriting the support original or fabricating payment status/known source fields.
- Enforce link/identity consistency and full-range lossless IDs with actual SQL constraints.

## Failure examples and separate gates

Test rollback during linking/state update, duplicate and ambiguous association, missing committed receipt, full identifier range, and durable independent state round-trip. Keep original21 service simulations distinct from real adapter verification.

`STORAGETEST-014` first refines versioned examples/interfaces, authors tests,
obtains a separate actual review and records intended missing-behavior red after
successful compilation. Minimal interfaces/harness/fakes and explicit stubs are
allowed. `STORAGE-014` depends on that completed baseline and its foundations,
preserves reviewed assertions, passes real acceptance and separate code review.
Neither an inherited passing suite nor missing dependencies is a new red baseline.

Primary testing home: Qt Test/CTest with disposable files and synthetic/redacted
fixtures; use existing public service tests where composition is relevant. Extend
ordinary CI for executable stable behavior, observe actual workflows, record exact
commands/revisions/environment and remaining gates. No live provider/chat/payment
side effects. Follow [workflow](../../AGENT_WORKFLOW.md), update evidence/progress
and publish one focused PR per executable task; no full-release claim follows.

## Retained-context dependency and numeric codec gate

[Retention revision1](../../contracts/retained-context-v1.md) preserves existing
configuration/context opaque references and document bytes losslessly; it creates
no redundant numeric IDs and does not certify a typed numeric SQL conversion.
This slice owns the actual ContributionId uint64 field: before persistence, review its
lossless SQL codec, complete range and domain-zero policy, independently test raw
representations (including values above signed64 and UINT64_MAX), then establish
the genuine reviewed red. No signed INTEGER/REAL coercion or opaque-byte test
substitutes for that typed gate. Existing identifiers/dependencies/acceptance and
all original lossless/provenance scope remain unchanged.


## Initial baseline ownership and cross-contract checks

STORAGETEST-014 is separately claimed from actual integrated maine331714 after
PR36. Initial author owns only new sqlite-contributions-v1 contract and own
stage evidence; a different actual contract review precedes API/stubs/tests.
Root owns actual additive migration3/order/schema and shared build/CI/state/map/
checkpoint; immutable retention1/receipt2 and all previous interfaces/tests remain.

Review domain-zero policy from actual existing contracts, not a fake's allocator
start. Current inspection found no ContributionId0 rejection: PlanEvent has an
optional ContributionId and Capture rejects ReceiptId0 only. Thus proposed full
ContributionId0..MAX with explicit nullable absence/first0 needs actual review;
ReceiptId remains1..MAX. If an agreed nonzero rule is found, reconcile it explicitly.
Never use zero as missing/unlinked where std::optional already represents absence.
ContributionValuation stores original money and a resolution reference/missing-rate
flag, not fetched/effective rates. Preserve all actual fields; keep downstream
HistoricalValuation rates/source/time in015 rather than invent an unused column.
A plain VerifiedActivityLink/proof reference is not provider authorization. Preserve
trusted upstream semantic/pre-committed-receipt preconditions and actual resolvable
arrival references; do not invent stronger proof from merely nonempty bytes or
same-session visibility. Independent review must pin any validation/error boundary.
