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

- No live author/reviewer is assigned. GitHub owns claims, stages and prerequisites.
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

Publish the owner-approved checkpoint revision through protected PR gates. Then
claim #645, preserve/reuse its existing local candidate history, and obtain fresh
independent Codex verification of the F4 additions and explicit component scope,
followed by fresh Claude closing test review. No unreviewed test source is pushed.
Keep #249 open and all deferred integration gates visible. Implementation still
requires completed test review and observed intended behavioral red.
