# CORETEST-008 first document-stage author handoff

Author: codec_author, 2026-11-04. Worktree coretest008-contract-author, branch
task/CORETEST-008-contract-author, from integrated main0b631dc after the normal
ACCT-004 PR51 merge and coordinator claimc311f0b. Root owns progress/state.json,
PROGRESS.html, maps/build/CI/checkpoint. This author owns only the two new Markdown
files listed below. The prior agents' source/reviews remain preserved.

## Actual authored scope

[Timer editorial commands proposal](../contracts/timer-editorial-commands-v1.md)
defines the first bounded logical document stage for CORETEST-008: signed time
deltas, cap replacement and hard-deadline replacement using the existing CORE-002
and CORE-005 calculations. It separates full requested/actual Main/debt/unapplied/
cap provenance, current access DATA, expected/current revisions and pure proposals
from actual service authority/CAS/commit. No duplicated numeric algorithm or new
dependency is selected. Absolute Main during start/reopen and debt correction are
explicitly distinct operations with their owning gates.

The16 literal rows ED01–16 preserve independently derivable existing numeric and
countdown rules. ED14 is explicitly a partial deadline-observation requirement,
not a chosen after-edit or server partial-commit policy. All three genuine owner
questions remain visibly unanswered under OPEN-CAP, OPEN-READY and OPEN-DEADLINE;
the document gives alternatives and dependent inputs without inventing expected
command results. No elapsed wait, existing above-cap addition pass or Ready helper
precondition is treated as an answer. No executable oracle for those choices exists.

Authority inspected: specification v0.45 sections3.5/4, TIMER-01/NUM-02–04,
CORE-008 brief, time-credit revision1 and its CONTRACT-001 evidence, time-debt
revision1, timer-lifecycle revision1, event/state-boundaries revision1, CORE-007 and
API-007 follow-up scope. Existing independent pause/debt/Grace/review/permission
obligations remain; no scope reduction or provider capability is claimed.

## Actual checks

Only documentation/link/progress checks were run in this author worktree:

```sh
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0. Project check reported396 task records/122 decisions/local links/
presets/current HTML; render check reported current snapshot. The tooling's
historical draft-test notice supplies no compilation, test review or behavior pass.
No native configure/build, GoogleTest/Qt invocation, new header/stub/test/fixture,
production code or missing-feature red was authored or executed for this task.

## Required next action

Hand the actual candidate to the separate document reviewer. This author cannot
approve it. Resolve the three owner questions through recorded answers before
turning dependent examples into executable oracles. Concrete API/finite bounds and
actual test authoring/review/red remain further stages; CORETEST-008 stays
in_progress and CORE-008 production stays planned. Existing stable CI can cover
this documentation-only stage; no new executable suite is introduced or promoted.

Changed paths:

- docs/contracts/timer-editorial-commands-v1.md
- docs/evidence/coretest-008-contract-author.md

No task completion, independent approval, PR publication/merge or full product
verification is claimed. Coordinator integrates this candidate and updates shared
state/checkpoint/PR as appropriate after actual review and owner decisions.
