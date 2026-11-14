# MIGRATE-004 — Switch task authority to GitHub and retire the tracker

Source: [issue #629](https://github.com/Kernol12/subathon-bot-ai/issues/629), snapshot 2026-11-13.
Contracts: [approved migration](../../contracts/github-task-migration-v1.md),
[proof records](../../contracts/proof-records-v1.md),
[publication](../../contracts/proof-publication-v1.md),
[authored cutover proposal](../../contracts/github-authority-cutover-v1.md) and
[separate exact ADR-0012 scope proposal](../../contracts/github-tracker-retirement-scope-v1.md).
The two004 proposals require independent review; links do not claim approval.

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
MIGRATE-001/002/003 have merged; only004 is currently claimed, for contract authorship.
State remains the sole planning/proof authority until the final protected004 cutover
merge, including across the required intermediate protected archive-preparation PR. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `MIGRATE-004`
**Work type:** Implementation
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Perform the single reviewed authority cutover using the previously prepared contract and proof support.

### Scope
- Reconcile the complete current task inventory, including these migration items, against GitHub before switching. Preserve readable titles, stable IDs, issue identities, parent/prerequisite distinctions, original-name history, evidence and completion limitations.
- Archive the final old tracker at an agreed Git revision/tag and preserve explicit historical exemptions and existing review records. Convert canonical proof records once and verify them against the original data.
- Make GitHub the only editable planning/status authority; actual PR/check records own merge facts. Activate the new proof input and remove temporary compatibility paths.
- Update AGENTS/CLAUDE/workflow guidance, coordinator prompt/checkpoint instructions, PR templates and active task-brief references together. Task Markdown retains scope, acceptance and constraints; status/assignment/dependencies/progress live on GitHub. Keep briefs accessible through cutover. Mandatory bulk compaction, archival and removal are MIGRATE-006 work after this cutover and before the final audit, with pinned historical links and no loss of acceptance criteria.
- Retire the old task ledger/state schema, HTML dashboard, renderer/controller/template, UI-only checks/targets and obsolete status/reconciliation commands. Keep the necessary proof validator, publication checks and independent repository checks.
- Apply the reviewed ADR-0012 cleanup for obsolete assertions. Configure issue closure and board status consistently; manual dragging to Done must not become evidence or authorization. Keep the coordinator as the dispatcher; no new claim/lease service.
- Update WORKFLOW-006's storage assumptions and dependency ordering without implementing its new review/results policy. Keep WORKFLOW-007 queued behind it.

### Acceptance
- After merge there is one planning authority and one current proof authority, with no permanent sync tool.
- All required checks pass before merge; a fresh checkout passes the relevant ordinary checks without the old ledger or live Projects API.
- Required proof/source-binding failures remain enforceable, and no missing record gains an implicit exemption.
- Source scope, all task relationships, history and rollback evidence are accounted for.


### Focused testing agreement
Reuse and adapt existing review-gate and publication tests. Add cases only where the new record loader or changed behavior needs coverage: valid/malformed records, missing required proof, reviewer independence, current-revision approvals and source binding. A missing module, dependency, compilation failure or skipped test is not an intended red baseline.

Use a one-time reconciliation report for the actual data conversion. Do not create hundreds of record fixtures or tests asserting that deleted files/buttons are absent. Preserve meaningful baseline assertions; any removal follows the explicitly reviewed ADR-0012 cleanup scope. Run relevant existing tooling/schema/scaffold/CI checks, including ordinary checks without live board access. New behavior follows the existing contract/examples, independent test review, meaningful intended red, fresh implementation and independent code-review sequence.

### Authorized execution boundary

Owner authorization supersedes the issue snapshot’s instruction to remain Planned/
unassigned; it does not waive registration, fresh sessions, review or protected merges.
Dependency readiness never grants owner authorization. Until cutover, preserve GitHub
planning items during mirror synchronization. No product or WORKFLOW-006/007 execution.

### Cutover packet and separate cleanup gate

The authored cutover proposal defines one-time full identity/title/root/relationship/
consumer reconciliation, protected archive-preparation merge/readback before the actual
immutable pin, final source-bound publication and coordinated consumer retirement.
The intermediate PR leaves004 open/in progress. No canonical extraction, remote writes,
assertion removal or authority switch is authorized by this contract authoring packet.
The separate ADR-0012 proposal pins exact methods/assertion groups and replacements;
obtain its own independent review cycle and reviewed changed tests before any removal.
The full code review cycle still applies to cleanup source. Preserve all current and
historical gates, including rejected reviews and actual newer completion observations.

All brief paths remain accessible through004.006 must then perform actual lossless
compaction/consolidation, full remote archival, link repair, justified removal and
explicit shared/unfinished retention before005 audits. WF006 receives storage/dependency
rescope only; neither it nor007 nor product/API execution resumes. Scope and Acceptance
above retain the issue snapshot wording; latest owner authorization governs execution.


### Owner-authorized historical evidence amendment

The owner accepted preparation and independent review of the narrow
[historical evidence dispositions](../../contracts/historical-evidence-dispositions-v1.md).
Apply no disposition until approved. Preserve all original claims, rejected history,
available safe evidence and explicit missing-content limitations; inventory unfinished
reliance and record actual affected-task holds. This does not waive current proof,
outgoing guards, archive/readback, finalization, CI or protected merge requirements.
