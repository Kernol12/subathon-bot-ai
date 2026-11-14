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
  Full core445/445, delegation55/55, zero skips; tier A/proof/required CI passed.
- ACCESS-010 (#646), branch `task/ACCESS-010-permission-storage`, draft PR #650.
  Original contract/tests approved at67084944; original94 NI/outcome-guard red
  preserved. Implementation f1a5c104 plus source robustness fixes f9a34be0 authored.
  Initial Claude source review found no source blockers; PG-B01 fixture defect was
  owner-approved for an extra correction round (646 comment6883152604).
  Revised baseline711e059d completed separate Claude/Codex/Claude approval.
  Corrected runtime:139/139 closure nodes and94/94 graph invocations pass, zero
  skips/warnings. Separate ordinary Qt CI execution/discovery now integrated;
  local discovery mirror passes44 methods/94 invocations. Independent Codex
  implementation verification and fresh Claude closing are next; no merge approval.
- Coordinator owns shared CMake/CI, migration allocation, checkpoint, receipts and
  proof. Source and revised test history retained; no further test change authorized.
  Full #249 integration remains open; ACCESS-011 (#649) follows #646 merge, then
  RULES-006 resumes after real producer readiness. No other live writer assigned.
- Draft PR #121 stays parked. Process/tooling work remains frozen until the slice.
  EPIC-029 closed, follow-ups #636; pre-cutover evidence remains historical.

## Next action

Finish ACCESS-010 independent implementation verification and fresh Claude closing
at the same exact final candidate, then one proof/policy commit and protected merge
on current CI. Do not treat corrected-test approval or runtime checks as code review.
Continue ACCESS-011 in a fresh session only after #646 lands; preserve all current
original-author, scope, revision and full release requirements.
