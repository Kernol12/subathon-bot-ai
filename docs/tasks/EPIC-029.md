# EPIC-029 — Move task management to GitHub

Source: [issue #625](https://github.com/Kernol12/subathon-bot-ai/issues/625), snapshot 2026-11-13.
Contract: `docs/contracts/github-task-migration-v1.md` (authored; independent review pending).

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
This brief registers scope; only 001 is claimed in this session. Until 004, state remains
authoritative. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `EPIC-029`
**Work type:** Epic
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Move task management from the Python/JSON tracker to GitHub Issues and Projects while keeping requirements and the necessary review evidence versioned in Git.

### Intended outcome
- GitHub owns tasks, readable titles, ownership, status, epic containment, prerequisites, activity and actual PR lifecycle.
- Git retains specification, architecture/contracts, acceptance briefs, permanent workflow rules, the coordinator authority checkpoint and small source-bound review evidence records.
- Retire the large task ledger and generated HTML dashboard; retain only the necessary validator and publication/repository checks.
- No permanent dual-entry tracker, generated facade of the old ledger, synchronization service or new project-hosted service.

### Sequence and closure
The six tasks are linked as native sub-issues. Execute MIGRATE-001 → MIGRATE-002 → MIGRATE-003 → MIGRATE-004 → MIGRATE-006 → MIGRATE-005; the actual authority switch occurs only in MIGRATE-004. Completion requires the final independent audit, not only a percentage shown by GitHub.

WORKFLOW-006 remains stopped and is sequenced after MIGRATE-005; its storage integration must be re-scoped without changing its intended artifact/results policy. WORKFLOW-007 retains its dependency on WORKFLOW-006. Product work stays paused.


### Mandatory task-brief cleanup
MIGRATE-006 compacts active briefs, archives full completed briefs and removes obsolete active copies after cutover. Pin and repair links before removal; keep shared briefs needed by unfinished tasks. MIGRATE-005 must verify this cleanup before the epic closes.
