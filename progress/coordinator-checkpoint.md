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
- RULES-006 (#558) completed by protected PR #652 merge `03e52e70`.
  Full152/152 and focused12/12 pass; Qt34cases+2hooks, zero skips/warnings.
  Both Tier A stages, single proof/publication and final-head7/7 CI passed.
  Historical rejections/reds remain in PR/proof; #562 retains configuration/ports.
- API-011 (#562), branch task/API-011-reference-intake, base/main03e52e70.
  Claimed6890787150 after #558 protected merge/readback; both stages tier A.
  Source feasibility67d321bd accepted6890950491: bounded #562 is feasible; existing
  components are not a running path. Complete configuration, owner-only fresh SAS1
  startup, genuine UTC/elapsed journal and current policy/action/reference producers
  remain explicit in-scope work, not outside blockers. No runtime/build proof from
  feasibility. Actual stored-session authority is required; no owner boolean.
  Header/examples1 and tests authored locally at e85848d1:11 new files,22 unconditional
  NI behavioral methods; compile/link/static and66 name-only listings pass. No test
  body/hook/provider/red ran. Examples posted6891350560; initial whole Claude tests
  review requested changes (15blockers API11-B01..B15,10notes N01..N10). Receipt and
  proposed examples1 stay preserved. Fix round1 of2 authored d3dbfb9b:header/examples2,
  12 owned paths,83 name-only Qt methods; strict compile/link/static pass. Author's
  proposed25 dispositions were independently checked; Codex requested changes for
  API11-B16/B17 (writer truth and unreachable completion boundary), plus residual
  N02. Both rejection receipts stay preserved. Root fix round2 of2 separates bounded
  local diagnosis from ORIGINAL rolled_back/not_committed, and independently seeds
  a3072-byte token:completion12607 fit/12606 refusal exceeds auth intermediates.
  Exact CID-conflict outcome/reason asserted. Strict compile/link/static and83 name
  listings pass. Fresh Codex re-verification approved8ab29748:B16/B17/N02 resolved,
  prior whole-stage assessment carried forward. N11 duplicate-copy assertion precision
  stays nonblocking. Fresh WHOLE Claude closing at8ab29748 requested changes for
  NEW API11-B18:entropy/token oracles omit mandatory advanced_state issuance.
  B01 RP05 evidence remained blocked; B02-B17 otherwise resolved. Closing rejection
  preserved. Owner explicitly approved ONE bounded third round (record6892536806).
  Root applied exactly the prepared B18 correction:runtime header3 pins ONE RCO1
  even for reuse, full domain/purpose reference identity,3draws per attempt and
  after03/retry advanced05+after06. No N11 optional change or production behavior.
  Fresh strict compile/link66steps/static and83 name-only listings pass. Examples4
  posted6892590521. NEW independent Codex verification and fresh WHOLE Claude
  closing APPROVED2154964a:B18/B01 RP05 authored-oracle objections resolved, no
  blockers. N11 duplicate consistency and N12 bounded direct-issue spent set remain
  mandatory implementation-audit notes, not test edits. At that stage no fourth
  round had been authorized.
  Root opt-in CMake integration built154steps/no warnings; approved12hashes unchanged.
  Tests review closed2154964a. Integrated sourcefe14d2bb actually ran137 new rows:
  135 exact first-NI failures,2 pre-NI SQL SETUP failures,4 passing hooks,0 skips/
  warnings. Both trigger installs hit the old writer's semicolon rejection; neither
  is valid red. Five preparation rows are explicitly mapped by tag in classification.
  Existing dependency CTest152/152 passes. Complete behavioral red/implementation
  remain unestablished. Owner explicitly approved ONE fourth fixture-only round,
  record6893032140. Root applied the prepared two-function patch:identical trigger
  DDL now installed through idle Observer with schema checks, as old service tests.
  All following assertions/fault predicates unchanged. Incremental6step compile/link,
  strict syntax/static and83 name-only listings pass. NEW independent Codex
  verification and fresh WHOLE Claude closing APPROVED exactd7155b42, including
  opt-in CMake registration. All8 review receipts and historical rejections remain
  preserved. A fifth test-correction round is NOT authorized.
  Root then actually reran both corrected suites atd7155b42:137/137 exact expected
  first-NI failures,4 passing hooks,0 setup failures/skips/Qt warnings; all83 methods
  and69 parameter rows discovered, all22 distinct APIs witnessed. Both repaired
  setups reached their intended NI. Original135NI+2setup evidence is unchanged.
  Dependency CTest rerun152/152 passes,0 skips. Complete intended-red gate is met;
  later assertions after each first NI remain unobserved and all22 APIs remain NI.
  No outside prerequisite or owner product decision was identified. Next is ONE
  DIFFERENT fresh implementation session, then its full Tier A review cycle.
  Temporary implementation ownership: three new reference_configuration/intake/
  runtime.cpp sources; necessary private representation/includes only in their
  three headers; new private server/src/reference_*_internal or
  application_state_codec_internal helpers; existing application_service.cpp ONLY
  for bounded shared SAS1 extraction with old wrappers/bytes/error behavior intact;
  reference_application.cpp ONLY for sharing unchanged authority validation through
  private helpers. No existing public API/contract or test/fixture/oracle changes.
  Small server/CMakeLists.txt source-list additions for those existing libraries
  are temporarily assigned if required; root retains options/registration/CI/pins,
  migrations/checkpoint/publication/proof. No competing writer is assigned.
  Mandatory audits include B04 dual SAS1 echoes, N11 full duplicate-completion
  consistency, N12 bounded spent sets, preallocation/nested limits/selected SQL,
  original writer ownership/errors, current whole authority and journal lifetimes.
  No implementation approval, proof or merge exists. Full #249/API010/recovery/
  host/live/process boundaries stay.
- Coordinator owns shared CMake/CI, migration allocation, checkpoint, receipts and
  proof. Step10 shipped under #649; all compiled prefix bytes remain frozen.
  Full #249 integration remains open. No new migration or other product writer is assigned;
  completed review worktrees are historical, not live assignments.
- Draft PR #121 stays parked. Process/tooling work remains frozen until the slice.
  EPIC-029 closed, follow-ups #636; pre-cutover evidence remains historical.

## Next action

Publish the closed tests stage and classified137NI/152-regression evidence in a
focused draft PR, preserving all8 review receipts and original135NI+2setup history.
Assign ONE DIFFERENT fresh API-011 implementation session with the temporary file
ownership above. Keep tests and public contracts fixed, obtain full Tier A source
reviews/checks, then final-head CI/proof/protected merge. No fifth test round, scope
expansion, downstream task or premature proof. Root owns shared publication.
