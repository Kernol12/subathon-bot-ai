# Coordinator checkpoint

Owner: Kernol12. Full specification v0.45 stays in release scope.

## Owner decision, 2026-11-14 (revised after RULES-006 producer inspection)

1. Build a running vertical slice: server starts, runs one timer and accepts one
   fake contribution.
2. Authorized immediate task path, in order:
   ACCESS-010 (#646) → ACCESS-011 (#649) → RULES-006 (#558) → API-011 (#562) →
   SLICE-001 (#642) → API-016 (#616) → APITEST-015 (#613) → API-001 (#278).
   ACCESS-009 (#645) is complete; owner explicitly authorized both new prerequisites.
   The owner approved separating the pure delegation component from authoritative
   integration. ACCESS-003 (#249) remains open; completing #645 does not complete it.
3. ACCESS-010 owns authoritative permission-graph storage/CAS; ACCESS-011 owns
   current reference original-author/capability/USE+GRANT/revision production.
   Complete them in order. Full #249 service implementation remains outside the
   immediate path. Identity linking #251, action planning/delivery
   #208/#232 and client preview rendering #424/#473 keep their ownership and full
   release requirements. Do not create a dependency cycle through these consumers.
4. RULES-006 and API-011 still require actual current supported reference authority
   producers under CONTRACT-014. Verify feasibility before authoring their tests.
   Pure calculations establish no authority or currentness. If a missing producer
   requires substantial work outside #646/#649's bounded scope, stop and report;
   these prerequisites now have execution authorization. This grants no fake
   allow-all policy, fabricated clock or unknown-as-known fact.
5. Process/tooling work stays frozen until the slice runs. WORKFLOW-006 (#619) and
   WORKFLOW-007 (#620) remain stopped.
6. ACCESS-009 and ACCESS-003 are tier A (permissions); ACCESS-010 is tier A
   (permissions/migrations/writer). ACCESS-011 and RULES-006 are tier A
   (authority/exact quantities). API-011, API-001 and APITEST-015 are tier A.
   SLICE-001 remains tier B. Set RULES-006/API-016 tiers from the workflow table,
   using the higher tier when unclear.
7. Preserve all review rejections and authored history. Two ACCESS-003 test fix
   rounds were used. Task-path revision alone does not authorize a third code/test
   fix round. Verify the revised component candidate independently, then obtain
   fresh Claude closing review before intended red. Ask before a third fix round.

## Settled delegation policy

Owner decisions are recorded in #249, comment 6879902355:

- Restrictive edits require management USE and GRANT scope.
- Untouched grants outside scope may remain; check every changed capability,
  newly exposed right and all shared-role affected accounts.
- Automation requires the original author's USE and GRANT for every whole action,
  with separate selected-sender checks. Recheck current authority at execution;
  hold unauthorized actions after revocation.

## Current state and assignments

- ACCESS-009 (#645) completed by protected PR #648 merge `1fd11c3b`.
  Tier A reviews/publication proof passed; final required CI 7/7 passed.
  Full core 445/445 and delegation 55/55, zero skips. #249 integration remains open.
- ACCESS-010 (#646) claimed on `task/ACCESS-010-permission-storage` from verified
  current `main` `1fd11c3b9a11c1340e7498aac2f8408cc6c41c83`. Tier A; fresh author
  authored API, NI stubs, 215-line physical schema contract and 28 Qt test methods
  at `f7c49ddbdccce13b831d84b5f1b3c4e2d4f8fdae`. Compile/link/format checks passed;
  no test bodies or intended red. Initial Claude tier A review requested changes
  PG-T01–T07 (independent seeding, exact physical/decoder/failure/limit coverage).
  First of at most two fix rounds authored at `c2bbdaa4`: 44 Qt methods,
  independent literal physical seed, canonical sparse rows and expanded negative/
  exact-limit cases. Compile/link/discovery passed; no bodies executed.
  Codex verification requested one correction, PG-T06: later calls on a failed
  session return session_closed; completion retains the original first error.
  Second/final fix round corrects only these authored expectations and explanatory
  contract/header text. Compile/link and discovery still pass (44 methods), no
  bodies run. Independent verification re-check and fresh Claude closing next.
  Initial receipt retained at
  `/tmp/access010-review-claude-initial.md`. All-intent composition PG-N03 recorded
  on owning #249; no authenticated-save scope added to this store.
  Prerequisites #645/#579/#297/#301 closed; no production implementation yet.
- Coordinator owns shared CMake/CI, additive migration integration, checkpoint,
  canonical issue pointer and proof. Author owns new permission-store API/stub,
  schema contract and focused Qt tests. No other live task writer assigned.
- ACCESS-011 (#649) is authorized after #646 merges. RULES-006's claim is parked;
  its source-inspected producer gap is recorded in #558, comment 6881646486.
- Draft PR #121 stays parked. SLICE-001 waits for API-011's reviewed startup path.
- EPIC-029 is closed (PR #638); follow-ups #636. Pre-cutover evidence remains
  historical; private outputs stay private.

## Next action

Complete ACCESS-010 contract/examples and test authoring in a fresh session, then
independent tier A tests review before intended red and implementation. Merge only
through current protected gates. Continue with a fresh ACCESS-011 session after
its dependencies land, then recheck RULES-006 producer readiness. Preserve the full
release scope and original-author/currentness decisions.
