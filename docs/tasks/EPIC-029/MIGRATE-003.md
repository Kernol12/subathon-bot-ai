# MIGRATE-003 — Prepare publication checks and CI for evidence records

Source: [issue #628](https://github.com/Kernol12/subathon-bot-ai/issues/628), snapshot 2026-11-13.
Contract: [prepared proof publication v1](../../contracts/proof-publication-v1.md),
refining `docs/contracts/github-task-migration-v1.md` and `proof-records-v1.md`.

Owner authorization now permits migration sequence 001 → 002 → 003 → 004 → 006 → 005.
This brief registers scope; only 001 is claimed in this session. Until 004, state remains
authoritative. WORKFLOW-006/007 and product work remain stopped; only WF006 storage/
dependency rescope is permitted. Successors require fresh sessions and all current gates.
Coordinator owns shared metadata/integration; executable authors own only assigned paths.

**Task ID:** `MIGRATE-003`
**Work type:** Implementation
**Area:** Quality and tooling
**Specification:** v0.45 §§17–18

Adapt consumers to the smaller evidence records without changing the current planning authority before cutover.

### Scope
- Adapt tools/check_publication.py to obtain selected stage bindings from the reviewed proof format. Preserve checkout/commit identity, actual reviewed-file binding, outgoing-history/private-path checks and honest limitations.
- Separate tracker/render checks from the independent acceptance-map, evidence/link, preset and repository checks in tools/check_project.py.
- Update the necessary CMake/schema/tooling/CI integration for the proof format. Keep ordinary CI reproducible from the checked-out source without GitHub Projects tokens or live board reads.
- Preserve existing review/publication rejection cases. Test the prepared new path while keeping the current production path working until MIGRATE-004.
- Document any temporary compatibility boundary; remove it in MIGRATE-004. Do not retain the old 496-task schema as a generated compatibility facade.

### Acceptance
- Stale reviewed-file bindings fail; unchanged approved bindings pass.
- Missing required proof and unavailable required validation inputs cannot synthesize success.
- Relevant existing checks pass for both the current production path and the prepared new path.
- No coverage, required check, branch protection or baseline assertion is silently weakened.


### Focused testing agreement
Reuse and adapt existing review-gate and publication tests. Add cases only where the new record loader or changed behavior needs coverage: valid/malformed records, missing required proof, reviewer independence, current-revision approvals and source binding. A missing module, dependency, compilation failure or skipped test is not an intended red baseline.

Use a one-time reconciliation report for the actual data conversion. Do not create hundreds of record fixtures or tests asserting that deleted files/buttons are absent. Preserve meaningful baseline assertions; any removal follows the explicitly reviewed ADR-0012 cleanup scope. Run relevant existing tooling/schema/scaffold/CI checks, including ordinary checks without live board access. New behavior follows the existing contract/examples, independent test review, meaningful intended red, fresh implementation and independent code-review sequence.

### Authorized execution boundary

Owner authorization supersedes the issue snapshot’s instruction to remain Planned/
unassigned; it does not waive registration, fresh sessions, review or protected merges.
Dependency readiness never grants owner authorization. Until cutover, preserve GitHub
planning items during mirror synchronization. No product or WORKFLOW-006/007 execution.
