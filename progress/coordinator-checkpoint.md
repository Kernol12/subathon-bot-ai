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
- ACCESS-011 (#649), branch `task/ACCESS-011-reference-authority`, draft PR #651.
  Main tests approved4ed77298, selected-reader supplement747e1d5f and admission
  supplementeaf1cb51. All earlier rejections and observed reds remain in PR receipts.
  Initial whole-source review at707fd680 rejected A11-I01; first fixbce94db7 is
  verified for A11-I01/N01. Codex whole-producer review instead found A11-I02:
  generic mutation authentication swallows original writer exceptions.
  Authentication supplement approved through all3 tests reviews at2881fb1b after
  A11-T01 correction. Observed red:187 originals PASS,9 controls PASS/18 FAIL,
  hooks2 PASS; all12 aggregated failure paths traced past setup into authentication.
  Second implementation fix round uses the existing strict lifecycle reader and a
  per-call adapter to rethrow original source/clock writer errors after generic
  resolution. Public session facades and all214 reviewed assertions remain frozen.
  Actual214cases+2hooks, strict compile, format/project/diff and updated59/214/216
  CI gate PASS, zero skips/warnings. Full current140/140 closure PASS. Independent
  Codex re-verification/fresh Claude closing, current-head CI/proof/merge pending.
  Any third implementation fix round requires owner approval; #558 remains stopped.
- Coordinator owns shared CMake/CI, migration allocation, checkpoint, receipts and
  proof. Step10 is reserved for this registry after compiled permission step9;
  preserve all shipped prefix bytes. Full #249 integration remains open. RULES-006
  resumes only after #649 supplies an actual current producer. No other writer is
  assigned; completed review worktrees are historical, not live assignments.
- Draft PR #121 stays parked. Process/tooling work remains frozen until the slice.
  EPIC-029 closed, follow-ups #636; pre-cutover evidence remains historical.

## Next action

Obtain independent Codex re-verification, then fresh Claude whole-producer closing
at the same final candidate. This is the second
implementation fix round: preserve original writer errors through authentication,
public session defaults and genuine source/clock refusals. No liveness SELECT or
caller-owned transaction cleanup is introduced. Third implementation fix round
requires owner approval.
Preserve all source/test history, rejected receipts and failures.
The private selected path must preserve old public facades, original writer truth,
metadata-before-ownership bounds, finite indexed selection and current authority.
Only after exact-head reviews, current required CI and one proof/policy commit with
both-stage publication validation may the coordinator perform a protected merge.
Then continue the authorized path with actual producer feasibility for RULES-006.
Full #249 and identity/UI/public auth/delivery remain with their owning issues;
process/tooling stays frozen until the vertical slice runs. No live effects.
