# Coordinator checkpoint

Owner authorization: complete WORKFLOW-001 (T3 workflow/context command), open its PR,
normal protected merge, then STOP. Product work is not resumed; API-012 remains
planned/unclaimed. The earlier stop-after-APITEST-012 instruction still applies to
product work. Full release scope remains incomplete.

## Current handoff

- Task: [WORKFLOW-001](../docs/tasks/WORKFLOW-001.md); status/evidence authority:
  [state.json](state.json). CLI contract revision 1 is in the brief.
- Branch: `task/WORKFLOW-001-t3-context`; base main
  `6594758adebfdb1f87cd8bdb261ba39178fc189b`. Verify actual HEAD/checkout on resume.
- Sol coordinator owns shared documentation/state/HTML and integration. Separate
  Codex test author and fresh implementation worker own their assigned test/tool
  paths; separate Claude test/final reviewers. Actual session links and exact reviewed
  revisions belong in the PR; do not infer active processes from this checkpoint.
- Pending: test review, intended red, fresh implementation, independent final review,
  tooling/project/scaffold checks, current-head CI and normal protected merge.
- Next action: finish only the pending WORKFLOW-001 gates and stop after merge.

## Preserved product handoff

APITEST-012 merged normally as main `6594758adebfdb1f87cd8bdb261ba39178fc189b`.
Detailed baseline/source/build/runtime/CI receipts and reviews remain in
[PR108](https://github.com/Kernol12/subathon-bot-ai/pull/108) and the
[pre-migration checkpoint](https://github.com/Kernol12/subathon-bot-ai/blob/6594758adebfdb1f87cd8bdb261ba39178fc189b/progress/coordinator-checkpoint.md).
Ignored local receipts, including `local-evidence/APITEST-012/final-stop.json`, old
branches/worktrees and SDKs are preserved. No cleanup/pruning or native rerun is
part of WORKFLOW-001. On future explicit product resumption, verify live PR/main
state and consume the accepted baseline for API-012 without reauthoring it.
