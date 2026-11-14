# EPIC-029 current coordinator checkpoint

Owner: Kernol12. Coordinator: codex:coordinator-59095155.
Scope: MIGRATE-004 → MIGRATE-006 → MIGRATE-005 only.
Product/API and WORKFLOW-006/007 execution remain stopped; full v0.45 is preserved.

## Latest owner decision (2026-11-14)

Stop A/B historical audits, reliance roles, seal recovery and old-artifact privacy review.
All pre-cutover records are “historical, as recorded, not re-verified”.
This replaces the earlier narrow amendment and its per-row acceptance requirements.
Private artifacts stay private, referenced by hash only; none is admitted as new proof.
Historical d717 identity, six dirty-run sources and missing seal do not block cutover.
New execution on unfinished tasks still requires its own current proof.

## Current state

001 merged PR632; 002 merged PR633; 003 merged PR634.
004 is in progress on task/MIGRATE-004-github-authority; state remains authority until merge.
Old tracker tag: epic-029-old-tracker-2026-11-14, target 355ffdfe (unchanged source).
Archive snapshot da8db0dc preserves tracker values; formatting is compact for bounded reads.
The old-tracker tag is pushed. A/B audit jobs cancelled; late privacy result not acted on.
Integrated reviewed cutover source: 0041d000317d7806b304b711f95ab9fcbe64310b.
Authored/reviewed oracle revision: 7da9561f44224ae725d1921256542a745fda7bc5.
Historical audit workers were cancelled; late results are retained without action.
Private audit proposals remain ignored/local; do not publish or continue them.
Prior checkpoint and detailed receipts remain in Git history at the old-tracker tag.

## Next actions

Cutover code/instructions/templates integrated; unrelated work/worktrees preserved.
Reconciliation: 503 task IDs/issues; zero parent/prerequisite mismatches; 496 readable title changes.
Existing checks pass: tooling 155/155; scaffold 3/3; SDK preset 6/6 nodes.
Repository/canonical validation and bootstrap publication preflight pass.
Earlier pre-staging runs failed fixture setup; preserved as setup failures, not red.
Cutover PR: https://github.com/Kernol12/subathon-bot-ai/pull/635 (draft).
Current review: Claude initial pending → Codex fix/verify → Claude closing.
Only wrong/unsafe cutover findings block; others go into one follow-up issue.
At most two fix rounds; ask owner with a short list before a third.
Current-head CI and normal protected merge remain mandatory; never bypass protection.
After merge: 006 inventory table, archive tag, cleanup PR and one review cycle.
Then 005 one fresh-checkout audit report, verification and migration closure.

## Guard boundary

Current proof/source/checkout/outgoing/missing-source guards apply to new records.
Historical records are retained as recorded and cannot implicitly authorize future execution.
One planning authority after merge: GitHub issues/native relationships/PR lifecycle.
One proof authority after merge: progress/proofs plus proof-policy; no duplicate tracker.

## Privacy

Private artifacts remain private. An earlier attempt to post a private privacy proposal
was automatically rejected; no payload was posted and no retry is authorized or needed.
Only hash references may describe those private audit artifacts.

Current implementation packet: a911d8a5; new review pending, older receipts preserved.
Fix rounds used in the new owner-directed cycle: 0 of 2.
