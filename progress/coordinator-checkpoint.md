# Coordinator checkpoint

Owner: Kernol12. Full specification v0.45 stays in release scope.

## Owner decision, 2026-11-14 (revised after the SLICE-001 feasibility stop)

1. Build a running vertical slice next: the server starts, runs one timer and accepts
   one fake contribution.
2. Authorized task path, in order:
   ACCESS-003 (#249) → RULES-006 (#558) → API-011 (#562) → SLICE-001 (#642) →
   API-016 (#616) → APITEST-015 (#613) → API-001 (#278).
   Nothing else is authorized for execution.
3. Process and tooling work is frozen until the slice runs. WORKFLOW-006 (#619) and
   WORKFLOW-007 (#620) stay stopped.
4. Review tiers follow docs/AGENT_WORKFLOW.md. ACCESS-003 (permissions) and API-011
   (accounting and clock) are tier A. The coordinator sets the tier for RULES-006 and
   API-016 from the tier table, using the higher tier when unclear. SLICE-001 is
   tier B (owner decision). API-001 and APITEST-015 are tier A.
5. At most two fix rounds per review; ask the owner before a third.

Why the path changed: SLICE-001 was first scoped as wiring existing components
together. Its feasibility step showed that is not possible (report in #642): the
application service needs clock, actions and reference producers, an initial state
and a clock startup path that exist only as test fixtures. The already planned
reference path builds exactly those pieces under CONTRACT-014, with no HTTP and no
Drogon SDK change, so it now comes first. SLICE-001 is re-scoped to a small
executable around what API-011 delivers.

## State at this decision

- GitHub owns task status and prerequisites. Each feature is one issue with a tests
  stage and an implementation stage. No tests exist yet for ACCESS-003, RULES-006 or
  API-011.
- SLICE-001 (#642): claimed on 2026-11-14, stopped at its feasibility step, claim
  released. No tests, builds or product edits were made. Local branch
  `task/SLICE-001-development-server` has no commits.
- Draft PR #121 (API-001, branch `task/API-001-host-lifecycle`) has been parked since
  2026-11-12. Check it against the current API-001 brief before reusing any of it.
- EPIC-029 is closed (PR #638). Follow-ups from the cutover review are in #636.
  Old tracker tag: `epic-029-old-tracker-2026-11-14`. Removed documents:
  `docs-archive-2026-11-14`.
- Pre-cutover history is historical, as recorded, not re-verified. Known limits: the
  d717 identity conflict, six missing dirty-run sources and the unrecovered seal.
  Ignored historical outputs stay private.

## Live assignments

None. No session or worktree is assigned.

## Next action

Claim ACCESS-003 (#249) on a branch from current `main`. Its only prerequisite,
ACCESS-002, is closed. Start with the tests stage: examples, authored tests,
tier A review, observed intended red.
