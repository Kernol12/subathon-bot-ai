# Coordinator checkpoint

Owner authorization: WORKFLOW-002 guidance update only, then stop. Keep Opus as the
Claude reviewer; do not substitute Sonnet. Product work remains stopped after API-012
merged in [PR110](https://github.com/Kernol12/subathon-bot-ai/pull/110), main 5ba0bdb.
APITEST-001/API-001 and actual session-clock binding remain unauthorized.

## Current task

- Branch: `task/WORKFLOW-002-opus-review-cycle`, based on current main 5ba0bdb.
- [Brief](../docs/tasks/WORKFLOW-002.md); [authoritative state](state.json).
- Coordinator: codex:9bae765e-bb17-4f99-8859-1361d2e80d35. Owns brief, task index,
  state, generated HTML, evidence summary and this checkpoint.
- Fresh Codex author `/root/guidance_author` owns only AGENTS.md, CLAUDE.md,
  docs/AGENT_WORKFLOW.md and docs/COORDINATOR_PROMPT.md. No other writer is assigned.
- Pending: initial Opus review, independent Codex verification/correction cycle,
  closing Opus approval, project/progress checks, PR/current CI and normal protected
  merge. No review/check is claimed passed yet. No runtime behavior or tests change.

## Next action

Inspect the author handoff and send the exact candidate for bounded initial Opus
review. Resolve findings through Codex fixes and separate Codex review before closing
Opus review. Retain all task-specific gates and unchanged valid approvals. Publish
one focused PR, merge only after latest checks/reviews/protection pass, then stop.
Detailed API-012 receipts remain in PR110, ignored local-evidence/API-012 and git history;
this task does not reopen its completed gates or replay native acceptance.
