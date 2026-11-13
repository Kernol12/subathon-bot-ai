# MIGRATE-002 — Separate review evidence from task tracking

Source: [issue #627](https://github.com/Kernol12/subathon-bot-ai/issues/627), snapshot 2026-11-13.
Contract: `docs/contracts/github-task-migration-v1.md`.

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
This brief registers scope; only 001 is claimed in this session. Until 004, state remains
authoritative. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `MIGRATE-002`
**Work type:** Implementation
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Prepare the small versioned proof format and loader while reusing the existing review-order validation. The production JSON tracker remains authoritative until MIGRATE-004.

### Scope
- Implement the format selected by MIGRATE-001, keyed by stable task identity and GitHub issue identity.
- Retain required stages, actual authors/reviewers, source revisions/paths, approvals/blockers, test evidence and proof references. Exclude ordinary backlog status, assignment, priority, epic grouping and duplicated dependency lists.
- Reuse one review grammar in tools/workflow.py; do not create a second enforcement engine.
- Make missing required records, malformed records and stale/self-approved evidence fail clearly, with explicit treatment of historical exemptions.
- Prepare the exact conversion procedure for existing structured records and history; exercise it with bounded examples. Actual canonical production extraction happens atomically at cutover, not through two editable copies.

### Acceptance
- The new loader and validator preserve existing applicable gate behavior.
- Valid records load; malformed, ambiguous, missing required or stale records are rejected.
- Existing valid approvals and rejected review history retain their actual identities and revisions.
- The current production tracking path continues working. No real task has two authoritative proof records.


### Focused testing agreement
Reuse and adapt existing review-gate and publication tests. Add cases only where the new record loader or changed behavior needs coverage: valid/malformed records, missing required proof, reviewer independence, current-revision approvals and source binding. A missing module, dependency, compilation failure or skipped test is not an intended red baseline.

Use a one-time reconciliation report for the actual data conversion. Do not create hundreds of record fixtures or tests asserting that deleted files/buttons are absent. Preserve meaningful baseline assertions; any removal follows the explicitly reviewed ADR-0012 cleanup scope. Run relevant existing tooling/schema/scaffold/CI checks, including ordinary checks without live board access. New behavior follows the existing contract/examples, independent test review, meaningful intended red, fresh implementation and independent code-review sequence.

### Authorized execution boundary

Owner authorization supersedes the issue snapshot’s instruction to remain Planned/
unassigned; it does not waive registration, fresh sessions, review or protected merges.
Dependency readiness never grants owner authorization. Until cutover, preserve GitHub
planning items during mirror synchronization. No product or WORKFLOW-006/007 execution.
