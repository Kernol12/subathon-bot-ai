# Coordinator checkpoint

Updated: 7 November 2026. Verify actual branches, worktrees, PRs and checks before acting;
this file is a resumption aid, not evidence that any agent or process is still active.

## Current state

- Last observed `main` before this update: `436965b` (PR #85 merged). The earlier owner
  instruction "finish the open PRs, then stop" is fulfilled and superseded by the owner's
  EPIC-028 direction.
- This checkpoint is committed inside the EPIC-028 planning PR (branch
  `task/EPIC-028-agent-process-test-cleanup`); read its merge commit from that PR.
- Current priority: [EPIC-028](../docs/tasks/EPIC-028.md) under
  [ADR-0012](../docs/decisions/0012-test-suite-simplification.md). TESTCLEAN-001 and
  TESTCLEAN-002 are dependency-ready and independent; use separate branches/worktrees.
  TESTCLEAN-001 keeps the ADR-0008/0009 review gates.
- Every cleanup task needs a separate approved review, a PR URL in `evidence` and one
  concise note. No active agent assignment remains.

## Preserved unpublished work

- ACCESSCONTRACT-001: approved local document work on local-only branch
  `task/ACCESSCONTRACT-001-local-login` (`53524fc`, behind `main`). Not an open PR. On
  resumption, integrate it with current `main` and publish one focused PR without repeating
  valid author/review stages. It does not block EPIC-028.
- STORAGE-020 production is not started; its reviewed 44-test baseline stays unchanged.
- Old `/tmp/subathon-*` worktrees exist, several prunable. Do not prune or reset blindly.

## Constraints

No live credential, provider, chat, payment or deployment operation is authorized. The full
v0.45 scope, all seven integrations and three FX providers remain required. Detailed prior
coordination history: [coordinator-handoff.md](../docs/evidence/coordinator-handoff.md).
