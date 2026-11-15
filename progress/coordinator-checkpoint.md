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
- ACCESS-011 (#649) completed by protected PR #651 merge `d7d64f25`.
  Authority214 cases+2hooks and140/140 closure pass, zero skips/Qt warnings.
  Both tier A stages, single proof/publication and final-head7/7 CI passed;
  historical rejections/reds stay in PR receipts and proof. #249 remains separate.
- RULES-006 (#558), draft PR #652, branch task/RULES-006-reference-evaluator.
  Base/main d7d64f25; claim6888000850, source feasibility6888185517. Owner path unchanged.
  Original tests approved4202917a/reda4775776; all receipts and two baseline fix
  rounds preserved. Fresh implementation7030bfeb/root8ee had152/152 and CI7/7, but
  independent Codex verification rejected R6-I01/I02 (6889616240); initial Claude
  approval is preserved, never substituted for verification.
  Additive header3/12row supplement144bb0f3 now has all3 Tier A tests approvals.
  Actual old-source red:7failing rows+5passing controls,2setup/cleanup,0skips/warnings;
  24methods/34invocations observed. Original22methods and fixtures remain byte-exact.
  Root code fix round1 preserves caller authority input errors outside translation
  and projects complete SAC1 envelope/value/document/reference limits before encoder
  allocation. Independent authority DTO budgets remain unchanged. Full152/152 and
  focused12/12 pass; Qt36passes/0skips/warnings. Fresh ASan/UBSan11pure+34Qt invocations
  pass, no diagnostics; LSan disabled (ptrace limitation). Strict/static/actual CI
  guard pass. Existing Qt CI inventory updated24methods/34invocations/36results.
  Whole code re-verification and fresh Claude closing/current-headCI/proof pending.
  No new baseline correction, producer/migration/dependency change or merge approval.
  R6-I03/I04,C01/C02 remain nonblocking; #562 retains full configuration/ports/submission.
- Coordinator owns shared CMake/CI, migration allocation, checkpoint, receipts and
  proof. Step10 shipped under #649; all compiled prefix bytes remain frozen.
  Full #249 integration remains open. No new migration or other product writer is assigned;
  completed review worktrees are historical, not live assignments.
- Draft PR #121 stays parked. Process/tooling work remains frozen until the slice.
  EPIC-029 closed, follow-ups #636; pre-cutover evidence remains historical.

## Next action

Publish the reviewed supplement, actual red and passing code fix round1 to draft
PR #652; obtain fresh independent whole-implementation Codex re-verification and
fresh Claude whole closing, especially R6-S03 before-allocation/caller-error audit.
Keep all approved assertions/header3 frozen. Then current required CI, one proof/
policy commit, both-stage publication and protected merge. Root owns CI/checkpoint/
proof; no advance past #558 before these gates. Owner #562/fullports and frozen
process/tooling/live boundaries remain unchanged.
