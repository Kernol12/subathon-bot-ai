# CONTRACTPLAN-001 handoff

Coordinator /root, 2026-11-04. Dedicated branch
`task/CONTRACTPLAN-001-wire-prerequisites`, base main `3217c212`.
Scope: [ADR-0004](../decisions/0004-wire-contract-prerequisites.md), parent and four
child briefs, state/HTML/navigation/checkpoint and planning evidence only.

The reviewed split retains the complete CONTRACT-003 integration gate and all ten
epic consumers. Common005, HTTP006, persistent WS007 and scene008 remain planned.
Eight internal-helper gates now wait for this planning integration; 16 other
non-epic consumers need their particular child contracts. Six WS consumers gain
explicit prerequisites, while all 319 previous baseline edges and every unrelated
prerequisite remain. No wire schema/source/test expectation/provider support changed.

[Author evidence](contractplan-001-author.md) records candidate `76c7b6e`.
[Different review](contractplan-001-review.md), commit
`1de03476bfffc6955e30bce12c8176f2218d801b`, note blob
`0224ebf83f4cd232660b1fa0cdc470ef7cebb5bf`, approves the actual plan and corrected
metadata `69ccc21`. No blocker or owner choice. Initial checkpoint links at `1b6fb0c`
failed consistency and were fixed at `69ccc21`; that failed check is not a pass.
Local installed GoogleTest evidence elsewhere was corrected from a guessed1.17 to
actual1.18.0; this plan has no native test or dependency change.

Finalization changes only status/review bookkeeping and accepted/proposed wording;
the approved graph/child scope remains unchanged. Actual consistency/current HTML/
whitespace checks pass (401 tasks/122 decisions). CI coverage is existing scaffold
consistency plus unchanged components; no new executable suite or workflow is needed.
No product/native/provider/renderer/platform tests were run for this plan.

Publication is a separate gate. Initial ordinary API object upload stopped on
truncated JSON before task-ref creation; remote absence was checked. Exact objects
can be resumed without rewriting history. After publication, record focused PR,
inspect ALL final-head check-runs, current rules/threads/reviews and normal merge.
No planning task is assumed integrated until the actual merge. Keep rate workflow
access and parked timer owner decisions explicit; continue milestone test authoring.
On next task branch record the real merge and immediately claim common005 or a
newly ready internal baseline from refreshed main. Full release scope remains.
