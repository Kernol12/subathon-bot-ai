# MIGRATE-006 — Compact active task briefs and archive completed briefs

Source: [issue #631](https://github.com/Kernol12/subathon-bot-ai/issues/631), snapshot 2026-11-13.
Contract: `docs/contracts/github-task-migration-v1.md` (authored; independent review pending).

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
This brief registers scope; only 001 is claimed in this session. Until 004, state remains
authoritative. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `MIGRATE-006`
**Work type:** Cleanup
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Make task-brief compaction and removal an explicit migration deliverable after MIGRATE-004's authority cutover and before MIGRATE-005's final audit. This task does not remove any files merely by being added to the board.

### Scope
- Inventory task Markdown files and all their consumers. Distinguish active briefs, completed historical briefs and shared briefs that still serve an unfinished test, implementation or verification task.
- Compact active briefs to stable scope, acceptance criteria, specification/contracts, implementation constraints, relevant ownership boundaries and required review/test gates. Remove duplicated task status, current assignment, task-dependency lists and progress notes once GitHub owns those fields. Preserve domain dependency constraints and all technical requirements.
- Consolidate genuinely overlapping briefs where they describe the same work. Preserve each task's identity, acceptance coverage and source-bound review context. Do not impose a word-count target or remove meaningful detail merely to shorten files.
- Archive full completed/historical briefs at an agreed durable Git revision/tag before removing obsolete copies from the active tree. Prefer immutable commit references for historical links; ensure the archive is remotely available before relying on it.
- Keep a shared or completed brief active if any unfinished task, acceptance requirement or current gate still needs it. Completion of one task sharing a brief is insufficient reason to delete the file.
- Update GitHub issue, documentation, navigation, acceptance-map and evidence references to the correct compact active brief or pinned archived version before removing a path. Preserve reviewed revisions and approval currency; obtain applicable review for material acceptance edits.
- Do not delete or compact specification, contracts or scoped agent instructions under the guise of task-brief cleanup. Do not move required versioned acceptance criteria solely into mutable issue text.

### Acceptance
- Every active task retains accessible, versioned scope and acceptance criteria with no lost requirements, review gates or unsupported capability claims.
- Tracking-only duplication is removed from active briefs. Completed briefs eligible for retirement are archived and obsolete active copies are removed; any retained exceptions have an explicit active consumer or gate justification.
- Shared briefs remain available until all required consumers are accounted for.
- Every removed path has a documented retained/replacement/archive destination, and all affected GitHub/documentation/evidence links resolve to it.
- Historical original names, review reports, limitations and full prior brief contents remain traceable.
- The cleanup is complete before MIGRATE-005 audits and closes the migration.

### Verification and review
Use one-time inventory, coverage and link-reconciliation reports and the relevant existing Markdown/repository checks. Do not add tests whose only assertion is that a file was deleted, or create a giant brief fixture suite. This documentation cleanup needs the applicable independent substantive review; no manufactured intended-red baseline is required for deleting or condensing prose. Preserve meaningful tests; any assertion removal requires the approved ADR-0012 scope and separate review.

### Authorized execution boundary

Owner authorization supersedes the issue snapshot’s instruction to remain Planned/
unassigned; it does not waive registration, fresh sessions, review or protected merges.
Dependency readiness never grants owner authorization. Until cutover, preserve GitHub
planning items during mirror synchronization. No product or WORKFLOW-006/007 execution.
Native blocker: MIGRATE-004. Native successor: MIGRATE-005.
