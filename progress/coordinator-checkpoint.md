# Coordinator checkpoint

Owner: Kernol12. Full specification v0.45 stays in release scope.

## Owner decision, 2026-11-14

1. Build a running vertical slice next: the server starts, runs one timer and accepts
   one fake contribution.
2. Authorized task path, in order: API-016 (#616) → APITEST-015 (#613) → API-001
   (#278). Nothing else is authorized for execution.
3. Process and tooling work is frozen until the slice runs. WORKFLOW-006 (#619) and
   WORKFLOW-007 (#620) stay stopped.
4. Review depth follows the three tiers in docs/AGENT_WORKFLOW.md. API-001 and
   APITEST-015 (host lifecycle, thread ownership) are tier A; the coordinator sets
   the tier for API-016.
5. At most two fix rounds per review; ask the owner before a third.

## State at this decision

- EPIC-029 is closed: MIGRATE-005 merged as PR #638 (merge 831f7a76); issues #630 and
  #625 are closed. Follow-ups from the cutover review are in #636.
- GitHub owns task status and prerequisites. Old tracker tag:
  `epic-029-old-tracker-2026-11-14`. Full brief tag: `epic-029-task-briefs-2026-11-14`.
- Draft PR #121 (API-001, branch `task/API-001-host-lifecycle`) has been parked since
  2026-11-12. Check it against the current API-001 brief before reusing any of it.
- Pre-cutover history is historical, as recorded, not re-verified. Known limits: the
  d717 identity conflict, six missing dirty-run sources and the unrecovered seal.
  Ignored historical outputs stay private.

## Live assignments

None. No session or worktree is assigned.

## Next action

Claim API-016 on a branch from current `main`, confirm its reviewed APITEST-016
baseline and intended red, then implement.
