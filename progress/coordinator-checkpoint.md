# Coordinator checkpoint

Owner: Kernol12. Full specification v0.45 stays in release scope.

## Owner decision, 2026-11-14 (revised after ACCESS-003 prerequisite inspection)

1. Build a running vertical slice: server starts, runs one timer and accepts one
   fake contribution.
2. Authorized immediate task path, in order:
   ACCESS-009 (#645) → RULES-006 (#558) → API-011 (#562) → SLICE-001 (#642) →
   API-016 (#616) → APITEST-015 (#613) → API-001 (#278).
   The owner approved separating the pure delegation component from authoritative
   integration. ACCESS-003 (#249) remains open; completing #645 does not complete it.
3. ACCESS-010 (#646) records the missing authoritative permission-graph storage/CAS
   prerequisite. It and #249's authoritative service implementation remain planned
   outside the immediate path. Identity linking #251, action planning/delivery
   #208/#232 and client preview rendering #424/#473 keep their ownership and full
   release requirements. Do not create a dependency cycle through these consumers.
4. RULES-006 and API-011 still require actual current supported reference authority
   producers under CONTRACT-014. Verify feasibility before authoring their tests.
   Pure calculations establish no authority or currentness. If a missing producer
   requires #646 or other new work, stop and report; this ordering grants no fake
   allow-all policy, fabricated clock or unknown-as-known fact.
5. Process/tooling work stays frozen until the slice runs. WORKFLOW-006 (#619) and
   WORKFLOW-007 (#620) remain stopped.
6. ACCESS-009 and ACCESS-003 are tier A (permissions); ACCESS-010 is tier A
   (permissions/migrations/writer). API-011, API-001 and APITEST-015 are tier A.
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

- ACCESS-009 implementation candidate `dacddfb5` implements all five APIs and
  shared borrowed permission-engine internals. Header/tests unchanged; local
  integrated acceptance passes 53 delegation +32 existing permission +33 numeric
  cases, zero skips. Ordinary passing acceptance/CI discovery integration is
  coordinator-owned. Independent Claude initial implementation review requested changes;
  code is not independently approved. Prior incomplete session produced no code.
- Initial implementation review at `026690b1` requested changes: R0-B1 empty-domain
  protected GRANT bypass and R0-B2 cubic membership-row expansion. Fix round 1
  adds two independently approved supplemental tests at `501c5078`; both observed
  intended assertion red before the code change. Whole-proposal guard and unique
  per-side membership-role expansion now authored; 55 delegation +32 permission
  +33 numeric cases pass, zero skips. Supplemental receipts retained for PR #648.
  Independent Codex implementation verification approved `3b627e00`, resolving
  R0-B1/R0-B2. Non-blocking scratch audit comment clarified without behavior change.
  Fresh Claude closing implementation review is next; proof/merge gates pending.
- ACCESS-009 (#645) claimed on `task/ACCESS-009-pure-delegation` from current
  `main` `85341ec4` after PR #647's protected merge. Preserved candidate history
  integrated at `bd3cb983d4ae276a7d6afd612aeb2243715d9038`; no test source pushed.
  Independent Codex verification approved F4 and the revised pure scope at that
  exact commit; receipt `/tmp/access-009-review-codex-verification.md` is retained
  for the PR. Fresh Claude closing review approved the full pure tests stage.
  Separate opt-in baseline registered: configure/build passed, 53 discovered and
  53 observed intended NI failures, zero skips. Initial discovery failed because
  other selected targets were unbuilt; it is not red evidence. Fresh implementation
  session found direct public evaluator reuse incompatible with borrowed graph
  inputs/whole outer-result projection and stopped without code edits. Coordinator
  authorizes a routine private shared-engine refactor within ACCESS-009, including
  `core/src/effective_permissions.cpp` and private core helpers, preserving public
  APIs/semantics and all existing tests. No contract relaxation or new task scope.
  Fresh implementation session continues with this explicit source ownership.
  Closing receipt retained at
  `/tmp/access-009-review-claude-closing.md` for the PR.
- ACCESS-009 inherits the local candidate on `task/ACCESS-003-delegation`, head
  `d3d5ccb682d8f59cc980d5c142ca65fed53895dc`: 53 authored tests and five NI APIs.
  Compile/link/format checks passed; no test bodies, intended red or implementation.
  Initial Claude and Codex verification requested changes. New F4 tests and the
  revised component boundary await independent verification/closing approval.
  Preserve receipts in `/tmp/access-003-review-claude-initial.md` and
  `/tmp/access-003-review-codex-verification.md` for eventual PR comments.
- ACCESS-003's authoritative integration blocker remains: missing policy graph
  persistence/current revision, original-author/hold lifecycle and linked identity
  resolution. The staged proposal is recorded in #249, comment 6880208046.
- SLICE-001's first composition-only attempt stopped at feasibility; no product
  edits/tests/builds. It now hosts the reviewed API-011 startup/reference path.
- Draft PR #121 remains parked; inspect its current brief/head before reuse.
- EPIC-029 is closed (PR #638); cutover follow-ups #636. Pre-cutover history stays
  as recorded, not re-verified. Historical private outputs stay private.

## Next action

Complete ACCESS-009 tier A implementation review (Claude initial, independent
Codex verification, fresh Claude closing), relevant checks and current
protected CI/proof gates. Keep #249 open and deferred integration gates visible.
