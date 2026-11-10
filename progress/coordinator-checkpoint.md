# Coordinator checkpoint

Owner authorization: WORKFLOW-002 guidance update only, then stop. The latest owner
preference gives the coordinator reviewer model selection; no model is mandated.
Product work remains stopped after API-012 merged in
[PR110](https://github.com/Kernol12/subathon-bot-ai/pull/110), main 5ba0bdb.
APITEST-001/API-001 and actual session-clock binding remain unauthorized.

## Current task and proof

- Branch: `task/WORKFLOW-002-opus-review-cycle`, based on main 5ba0bdb. The branch
  name is historical; current guidance does not pin any model.
- [Brief](../docs/tasks/WORKFLOW-002.md); [authoritative state](state.json).
- [PR111](https://github.com/Kernol12/subathon-bot-ai/pull/111) is draft and watched.
  Do not relink or rewatch solely on wakes; inspect the actual latest head.
- Coordinator: codex:9bae765e-bb17-4f99-8859-1361d2e80d35. Author and reviewer
  handoffs are complete; no old assignment is assumed active.
- Earlier initial/closing Claude and Codex reviews through4452648 are retained.
  The owner then superseded the fixed-model preference with coordinator selection.
- Revised cleance55cbf32045576d4d7ee7bb8416ba50faabf7c9 independently APPROVED by
  Codex `/root/model_choice_codex_review` and closing Claude Sonnet5.5 (9464edef).
  Subsequent edits only record actual reviews/checks and final publication steps.
- Project/link/progress checks passed (469 tasks), tooling50 and diff checks passed.
  Guidance component is complete; no runtime behavior, tests or provider settings
  changed. Hosted CI/protected merge are publication gates, not claimed passed.

## Next action and stop

Push completion metadata, update the PR body/review records and mark ready. Require
all applicable latest-head CI checks, no unresolved discussions and live repository
protection before a normal merge with the exact head guard. Refresh main, unwatch,
then stop. Record merge/check receipts in PR111 rather than a follow-up metadata
commit. If resumed after merge, verify and stop. Detailed API-012 receipts remain in
PR110, ignored local-evidence/API-012 and git history; do not reopen product gates or
replay native acceptance for this guidance task.
