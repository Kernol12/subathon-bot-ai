# Coordinator checkpoint

Updated: 7 November 2026. Verify actual branches, worktrees, PRs and checks before acting;
this file is a resumption aid, not evidence that any agent or process is still active.

## Current state

- Last observed `main`: `b676f08` (PR #90 merged). The earlier owner instruction "finish
  the open PRs, then stop" is fulfilled and superseded by the owner's EPIC-028 direction.
- This checkpoint is being updated on branch `task/EPIC-028-evidence-rule`; verify its
  PR/merge state on resumption.
- Current priority: [EPIC-028](../docs/tasks/EPIC-028.md) under
  [ADR-0012](../docs/decisions/0012-test-suite-simplification.md). EVIDENCE-001 is ready;
  claim it after this planning change merges. EVIDENCE-002 depends on it. TESTCLEAN-001
  and TESTCLEAN-002 are also ready and independent, but come after EVIDENCE-001 in
  execution order; TESTCLEAN-001 keeps the ADR-0008/0009 review gates.
- Every cleanup task needs a separate approved review, a PR URL in `evidence` and one
  note of at most 500 characters. The archive tag does not exist until EVIDENCE-001 creates
  and verifies it; this planning change does not remove any receipt or shorten old notes.

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
