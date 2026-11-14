# MIGRATE-004 — Switch task authority to GitHub and retire the tracker

Issue: https://github.com/Kernol12/subathon-bot-ai/issues/629
Specification: v0.45 §§17–18.
Contract: [cutover and latest owner decision](https://github.com/Kernol12/subathon-bot-ai/blob/docs-archive-2026-11-14/docs/contracts/github-authority-cutover-v1.md).
Test retirement: [separately approved ADR-0012 scope](https://github.com/Kernol12/subathon-bot-ai/blob/docs-archive-2026-11-14/docs/contracts/github-tracker-retirement-scope-v1.md).

Tag the old tracker as recorded, integrate the reviewed cutover code and update
AGENTS/workflow/templates. Produce one reconciliation report for task IDs, titles,
parents and prerequisites against current GitHub. Run the relevant existing checks.
Use one cutover PR and one review cycle, with at most two fix rounds. Preserve the
full v0.45 scope, existing task/issue identities, historical names and review records.

Acceptance:
- GitHub owns planning, task lifecycle and native relationships; Git retains stable
  specifications, acceptance briefs, current proof and the authority checkpoint.
- Old state/HTML tracking and temporary compatibility paths are retired; ordinary
  CI works without Projects access. There is one proof authority and no sync tracker.
- New work still requires independent review, current source-bound proof, passing
  applicable checks and a normal protected merge. Missing required proof fails.
- Pre-cutover history is historical, as recorded, not re-verified. Private audit
  artifacts stay private. The withdrawn per-row audit is not a migration gate.
- MIGRATE-006 performs brief inventory, compaction, shared retention, archival,
  link repair and removals before MIGRATE-005 verifies the migration and closes it.

WORKFLOW-006 storage/dependency rescope is included; implementing WORKFLOW-006/007,
product or API behavior remains stopped. No new contract/audit packet/test suite is
required. Keep actual CI, independent review and protected-merge evidence in the PR.
