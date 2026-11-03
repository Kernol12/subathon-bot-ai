# STORAGE-016 — Nonowning durable application-review holds

Parent: [EPIC-009](../EPIC-009.md). Baseline STORAGETEST-016 → implementation
STORAGE-016. Specificationv0.45 §§3.3–3.5,4,5; NUM-02/03/04, DB-01.
Status/owners/dependencies: progress/state.json.

## Required bounded outcome

Implement the reviewed application-service contract's durable hold repository on
the caller's one actual active SQLite writer session. Whole-input numerical
review holds cannot be a transient return, successful-bundle claim or overwrite of
another condition. Scope includes immutable hold cause/source/plan/state/provenance,
stable tagged-subject active gating independent of request key/replan, retrievable
history and separately authorized resolution/active-head transition. Preserve
current contribution dimensions and the distinction from timer-only deficit policy.
Exact logical dispositions, port/data shape and domain preconditions must be
refined/reviewed before concrete API/schema or tests; this brief finalizes none.

Hold insertion/resolution and any caller's related decision/audit must compose
atomically under that existing session. No nested BEGIN/commit/adoption/destructor
cleanup, separate writer or live delivery. All reads/duplicates/staged results are
provisional until actual owner COMMIT and normal completion. Real known rollback,
unknown/fail-closed/no blind retry, pure input priority, typed exact numeric/ID/NULL
provenance, scoped uniqueness and resource/materialization bounds remain required.
A hold-storage failure cannot be reported as a successfully durable Held result.

## Review/red/implementation gates

STORAGETEST-016 first supplies the concrete reviewed contract and additive physical
migration/API, independent synthetic frozen oracles/real disposable Qt SQL tests,
different test review, successful compilation/compiled discovery and actual intended
missing-feature red. Preserve complete accepted catalog prefix1–5/66 objects and
all prior assertions/fixtures; root controls new migration order/schema/build/pins.
Missing dependency/compiler error/old passing suites are not this red.

STORAGE-016 then implements only that bounded repository, preserving reviewed
assertions and passing its own and relevant cross-record suites with separate
code/config review and actual promoted ordinary selected-Qt CI. Keep baseline and
implementation separate focused task PRs. Service ACCTTEST-004 starts from integrated
hold primitives; it retains its own orchestration baseline/red, not an aggregate.

Primary paths: server persistence, versioned migrations, tests/qt and fixtures.
Read root/server/tests AGENTS. Record exact artifacts/commands/environment/limits
and explicit unrun live/platform/release gates. No policy/permission/provider/
financial entitlement/recovery/UI capability follows from storing a hold.
