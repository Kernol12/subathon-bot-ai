# FIXTURE-002 — Storage fixture recipes and named selector roles

Module: Fixture tooling. Kind: implementation. Specification: v0.45; §17.
Contract revision: reviewed fixture-generation R1 plus current storage/original-claim contracts.
Parent: EPIC-028 / TESTCLEAN-006. Depends on: FIXTURE-001.

## Scope and ownership

Own `tools/fixture_sources/{bundle,holds,original_claims}.py`, their explicit CLI-family
registration, focused generation goldens, ONLY those three JSON outputs, and selector
row names/role metadata in `tests/qt/sqlite_original_claim_tests.cpp`. Coordinator owns
shared metadata/docs/CI/CMake/PR. Fresh implementation session; read scoped instructions.

Preserve all independently supplied literal schema/prefix/framing/hash, query/snapshot,
SQL setup, arithmetic, version, detection, view, refusal and byte/item oracles. Relevant
families include bundle's14table declarations/56setup statements/six scenarios/28constraint
refusals/10runtime admissions; holds'71detections including pause1–65/seven causes/views/
35byte-item oracles/arrangements/history; claims'45ordered setup statements/nine queries/
six selectors/A-B logical bytes. Detailed inventory/provenance and exact coverage map must
be reviewed before source changes; these counts are planned preservation constraints.

Use shared literal source functions, never generated outputs, production code/binaries,
SQLite observations, a generic SQL interpreter/patch framework or a new policy simulator.
Finite named data construction is allowed; expected outcomes remain independent literals.
No behavior deletion to meet a line-count target. Public errors/order/budgets/ownership,
exactness/rollback/idempotency and every migration freeze/full-prefix comparison remain.

## Exact named-selector delta

Add only `name` and `check_lower_priority_corruption` to each existing selector, preserving
all old typed fields and order. Use explicit row/boolean metadata, not map order or index0.

| Old row | New name | Extra corruption role |
| --- | --- | --- |
| selector-0 | contribution-max-receipt-prefers-subject-A | true |
| selector-1 | receipt-max-resolves-source-B | false |
| selector-2 | contribution-receipt8-prefers-subject-A | false |
| selector-3 | missing-contribution42-receipt9 | false |
| selector-4 | receipt8-selects-original-subject-C | false |
| selector-5 | contribution42-receipt8-resolves-source-C | false |

The old first-row lower-priority-corrupt-B assertion MUST still execute after selectingA.
Keep ordered setup[9] and its public PA-revision/caller bind positions30–33 unchanged.
Existing named query/result assertions remain. Do not infer field roles from a reordered
array or delete a unique branch because an index looks private.

## Gates and acceptance

- Extend R1 family contracts/examples and independently reviewed golden tests BEFORE
  implementing new family entrypoints. Reachable named inert stubs permit intended red;
  no missing-dependency red or test of deleted machinery. Source/oracle execution review
  precedes generation/native use. No expected value comes from a candidate generator.
- Bundle/holds typed contents preserve parent baseline canonical hashes. Claims preserves
  its exact old canonical content after removing ONLY the two declared selector metadata
  fields; separately verify all six exact names/role values against the table.
- Generation works without six legacy outputs present, using only approved seeds/source
  functions. Check supported-family/drift semantics stay explicit and truthful.
- All original method/assertion/fixture families remain. Review a one-to-one native PASS-ID
  crosswalk: only six selector labels change from current869IDs/361methods. Unique corruption
  priority and physical/rollback/budget evidence remain; account44 stays passive red.
- Run generation and relevant Qt/tooling once for actual changed baseline; record actual
  counts, separate reviews, current CI/rules/discussions/normal protected focused PR merge.
  No new dependencies/registrations/provider effects or unrelated native receipt replay.
