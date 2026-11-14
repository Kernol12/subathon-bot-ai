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
- ACCESS-010 (#646) completed by protected PR #650 merge `65936bcb`.
  Corrected closure139/139 and graph94/94 passed, zero skips/warnings; tier A,
  proof/publication and final-head7/7 CI passed. Owner-approved PG-B01 correction
  and historical rejections remain recorded in the PR and proof.
- ACCESS-011 (#649), branch `task/ACCESS-011-reference-authority`, claimed after
  #646 merge. Contract/schema/API/NI and42 test methods authored at22c7753b.
  Initial Claude tests review requested changes (RA-T01–T05). Fix round1 at4ed77298
  completed independent Codex verification and fresh Claude closing approval of
  the full contract/tests stage. All verdicts remain recorded; no reviewed header,
  contract or test changed during integration. Opt-in registration discovers47
  methods/145 invocations with ordinary intentional-red semantics and no skips or
  inverted results. Integrated build132/132 passes without warnings. Private
  callback state now carries existing enclosing ceilings; all new reads/units and
  the step10 factory remain NI. Draft PR #651 at62297124 owns all three receipts.
  Observed red:145/145 invocations,141 direct NI and4 outcome guards separately
  traced to the NI constructor as their first throw. Hooks2pass, zero skips or
  warnings; no old setup failure before the first NI. Later assertions/setup after
  NI remain unexecuted. Existing service/graph regression nodes2/2pass.
  Fresh implementation WIP bc1f4095 is local/unpushed:145/145 authority and140/140
  closure pass, but the source audit reproduced a historical-reader blocker.
  Legacy bundle/hold facades roll back/translate failures and scan hold history;
  green output does not meet the selected-reader contract. No code review yet.
  Supplemental tests/header authored032f91b1:5 families/24 static rows,52 methods
  compile/list; main47 bodies/data/hooks byte-exact. Initial Claude supplemental
  review requested SR-T01 (unstated aggregate-limit mapping). Coordinator fix1
  explicitly accepts captures for whole selected historical source bytes, with
  owning component/item minima; authority contract revision3 remains290 lines.
  Private header revision2 clarifies selected parent/full closure validation;
  caller marker is now outside selected records. Bodies remain unrun.
- Coordinator owns shared CMake/CI, migration allocation, checkpoint, receipts and
  proof. Step10 is reserved for this registry after compiled permission step9;
  preserve all shipped prefix bytes. Full #249 integration remains open. RULES-006
  resumes only after #649 supplies an actual current producer. No other writer is
  assigned; completed review worktrees are historical, not live assignments.
- Draft PR #121 stays parked. Process/tooling work remains frozen until the slice.
  EPIC-029 closed, follow-ups #636; pre-cutover evidence remains historical.

## Next action

Obtain supplemental ACCESS-011 Codex verification and fresh Claude closing after
fix round1 (of2), then behavioral red against unchanged bc1f4095 source.
Preserve existing assertions/public headers/contracts. This remains #649's existing
historical-disclosure scope: private borrowed-session bundle/selected hold readers
may be factored from bundle access/bundle.cpp/holds.cpp while retaining the old
public facades' behavior. No new public capability, writer or transaction.
Only after supplemental approval/red complete that extraction and the authority
composition in a fresh session, then full tier A implementation review. Coordinator
owns shared build/CI/checkpoint/receipts/proof and eventual merge.
Actual current authentication/graph/capture production, finite indexed reads,
service-ceiling enforcement and nested-facade failure truth need implementation
and the full tier A source/resource audit. Provisional facts are not permits.
Preserve current original-author, scope, revision and full release requirements;
stop if an additional substantial prerequisite falls outside #649's bounded scope.
