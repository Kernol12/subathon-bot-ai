# MIGRATE-001 — Define the task-tracking migration contract

Source: [issue #626](https://github.com/Kernol12/subathon-bot-ai/issues/626), snapshot 2026-11-13.
Contract: `docs/contracts/github-task-migration-v1.md`.

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
This brief registers scope; only 001 is claimed in this session. Until 004, state remains
authoritative. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `MIGRATE-001`
**Work type:** Contract
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Define and independently review the migration contract before implementation. This task specifies the transition; it does not switch authority.

### Scope
- Inventory every consumer of progress/state.json, tools/progress.py, the state schema and PROGRESS.html, including publication checks, CMake/CI, tests, instructions, PR template, coordinator guidance and task briefs.
- Define ownership of planning fields versus versioned evidence, a stable task-ID/issue-identity mapping, the small proof format and deterministic record-discovery/eligibility rules.
- Define closure: all required gates must be satisfied and the applicable final completion PR must be merged. Intermediate contract/test PRs must not close an entire implementation task. Ready is not execution authority.
- Preserve actual authors/reviewers, review order, revision/path binding, rejected history, evidence limitations and the explicit historical exemption boundary. A missing receipt must never silently become a legacy exemption.
- Specify the one-time conversion/reconciliation receipt, archive and rollback procedure, the single cutover in MIGRATE-004 and the separately approved ADR-0012 test-cleanup scope.
- Resolve WF005 local post-merge metadata against actual merged PR126 rather than copying stale main state. Re-scope WORKFLOW-006 storage integration for the new evidence format; retain its intended behavior and WORKFLOW-007's dependency.

### Acceptance
- A reviewed, concise contract identifies one authority for each field and every affected consumer.
- There is no permanent dual writable source or live Projects API dependency in ordinary CI.
- The expected behavior and targeted cases for MIGRATE-002/003 are independently specified.
- The coordinator has a bounded ownership/PR sequence and a clear cutover/rollback checklist.

### Review and verification
Use the existing independent substantive contract-review cycle. Documentation and inventory alone require no manufactured failing test baseline. Preserve the currently active review policy; results-only policy changes remain WORKFLOW-006 work.


### Task-brief cleanup contract
Include MIGRATE-006 in the reviewed migration contract: define active/shared/completed brief classification, archive eligibility, immutable archive/link mapping, acceptance preservation and reviewed consolidation rules. Brief cleanup runs after MIGRATE-004 and before MIGRATE-005; no deletion is authorized merely by adding the planning item.

### Authorized execution boundary

Owner authorization supersedes the issue snapshot’s instruction to remain Planned/
unassigned; it does not waive registration, fresh sessions, review or protected merges.
Dependency readiness never grants owner authorization. Until cutover, preserve GitHub
planning items during mirror synchronization. No product or WORKFLOW-006/007 execution.
