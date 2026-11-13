# MIGRATE-005 — Verify and close the task-tracking migration

Source: [issue #630](https://github.com/Kernol12/subathon-bot-ai/issues/630), snapshot 2026-11-13.
Contract: `docs/contracts/github-task-migration-v1.md` (authored; independent review pending).

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
This brief registers scope; only 001 is claimed in this session. Until 004, state remains
authoritative. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `MIGRATE-005`
**Work type:** Environment verification
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Independently audit the actual merged cutover and close the migration only when the observed result meets the reviewed contract.

### Scope
- Read back all GitHub task identities, issue states, planned statuses, epic containment and prerequisite links. Compare them with the frozen cutover inventory, including migration tasks and approved board-only planning items.
- Confirm that original-name/history/evidence links and explicit legacy policy survive, and that no tasks or source requirements were silently deleted.
- From a fresh checkout, run the relevant existing repository/tooling/proof/schema/scaffold checks without board access. Check bounded task/brief/proof retrieval through the documented workflow.
- Confirm from existing focused gate cases that stale or missing required proof blocks completion. Confirm that owner pause directives still override dependency readiness and that ordinary board edits cannot grant reviewed approval.
- Verify the archive/rollback reference and that no active instruction still makes the retired JSON/HTML tracker authoritative. Inspect the live board configuration and closure behavior.
- Record an independent factual audit report with actual checks and remaining limitations. Fix findings in focused reviewed corrections before closing the migration epic. Do not open a status-only completion PR merely to describe its own future merge.

### Acceptance
- The observed merged repository and board satisfy every cutover criterion with no unexplained drift or missing required gates.
- There is no duplicate manual tracker, permanent sync process or unexpected live-board dependency in CI.
- WORKFLOW-006/007 and product work remain paused until separately authorized.

### Verification approach
This is an operational audit, not new product behavior. Use a one-time reconciliation report and relevant existing checks; do not author a large migration fixture suite or manufacture an intended-red run for read-only verification. Obtain the applicable independent review before closure.


### Required brief-cleanup prerequisite
Complete [MIGRATE-006](https://github.com/Kernol12/subathon-bot-ai/issues/631) after cutover before this audit. Verify compact active briefs, retained shared requirements, the durable archive of completed briefs, obsolete-file removal and all replacement/pinned GitHub/documentation/evidence links. Native prerequisites now place MIGRATE-006 immediately before this task.
