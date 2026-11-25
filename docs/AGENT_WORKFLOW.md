# Agent workflow

GitHub Issues own task status, assignment and prerequisites. Pull requests own review
receipts. `progress/proofs` holds proof records for tier A tasks only. History before
the MIGRATE-004 cutover is kept as recorded and is not re-verified.

This file is the single source for the task sequence and the review rules. Other
documents link here instead of repeating them.

## Task sequence

For new behavior: contract/examples → authored tests → independent test review →
observed intended red → implementation → independent code review → relevant checks.

- Compile failures, missing dependencies and skipped tests are not red evidence.
- Before sending the first review packet for either stage, run each candidate suite
  once. In the tests stage, confirm setup and fixture reachability, including
  reaching the intended stub where applicable. Fix setup failures before review.
  In implementation, run the frozen tests against the candidate and fix any
  failures before review. Record the run in the handoff. These pre-review runs
  earn no test evidence.
- Test programs are admitted in CI ([ADR-0013](decisions/0013-ci-test-program-admission.md)):
  the `core` and `qt` jobs compare every program's library closure with its list in
  `tools/binary_closures/`. The credited passing run is required CI on the reviewed
  PR head; the PR names the head commit and the tested merge commit and links the
  run. Intended red is one plain local run on the reviewed tests commit, recorded in
  the PR with its commit, command and failure summary.
- A suite that no CI job runs joins a CI job in its implementation PR
  ([ADR-0014](decisions/0014-plain-test-runs-and-leaner-tasks.md)). Adding a suite
  to its existing job, with its entry in `tools/binary_closures/`, is part of the
  task and needs no owner decision. Only when a test cannot run on a hosted runner
  at all is its credited pass one plain local run on the reviewed head: the PR
  records the commit, the command, the result and the reason CI cannot run it, and
  the code reviewer reads that record before approving.
- A local test run needs no pin, seal, approval record or owner confirmation, and
  no merge hold is set for it. Seals and identity checks that a CI job performs
  itself, such as the `native` job's SDK gate, are unchanged.
- Expected values come from the specification, never from the implementation.
- Do not delete, skip or weaken reviewed assertions to make code pass. Correct a
  genuine test defect only with specification-based reasoning and a separate reviewer
  (ADR-0012 covers planned cleanup).
- A reviewed red baseline is not an implemented feature. Implementation is done when
  the reviewed tests pass, its review approves and the PR is merged.

One feature is one GitHub issue with two stages, tests and implementation. The
issue body is the brief and defines scope and acceptance. Every new task ID gets
a short tracked pointer file under `docs/tasks` with one line each for `Task ID`,
`Work type` and `Issue`. Use the full GitHub issue URL for `Issue` and include
the pointer in a complete reviewed stage for publication.

An imported issue may name a separate `…TEST-nnn` baseline task. Read that as
the tests stage of the same issue. A tier A task uses one PR per stage. A tier B
task uses one PR for both stages:

1. The tests are reviewed on the local commit; unreviewed test source stays local.
2. After that review approves, observe intended red locally, push the reviewed
   tests commit and open the draft PR. Post the tests review and the red record as
   its first comments.
3. Add the implementation commits to the same PR and post the code review there.
4. Keep the new tests behind the task's default-OFF build option until the
   implementation commits enrol them in their CI job, so that no required check
   runs them red. Only the final head, with every required check passing, merges.

The tests stage adds at most 800 lines of tests and fixtures unless the issue names
a larger number and the reason. Write one test per row of the examples table and
per named edge case. The test reviewer names every test that could be removed
without losing a specified behaviour.

## Roles

The coordinator selects dependency-ready tasks inside the owner scope, assigns paths,
sets the review tier, integrates shared files and merges. Authors produce bounded work
and factual evidence. Reviewers stay read-only and assess requirements, oracles,
implementation and gaps.

- The coordinator decides routine run mechanics itself: paths, environment, ref
  refreshes, rebuilds and merge-conflict resolutions. A choice is routine when it
  changes no test's isolation, needs no sandbox exception and weakens no check or
  assertion. Isolation changes go to the advisor and sandbox exceptions to the
  owner, and a standing owner rule in the checkpoint that stops a particular
  conflict for the owner still applies.
- An author never approves their own work. A reviewer who writes a fix cannot approve
  that fix.
- The default split is Codex authors and Claude reviewers. The coordinator chooses the
  model for each review and records the actual provider/model in the review comment.
- Use a fresh implementation session per task and separate worktrees for concurrent
  writers. Never assume an old assignment is a live session.
- If no authorized model is available for a required review, leave the gate pending
  and continue other authorized work. Do not waive the review, switch providers or
  buy usage without the owner.

## Review tiers

The coordinator states the tier in the PR description when the task is claimed.

| Tier | Applies to | Review |
| --- | --- | --- |
| A — full cycle | Exact accounting, rounding and reversal; authentication, sessions, credentials and permissions; SQL migrations and the single writer; thread ownership, host lifecycle and shutdown; contracts that define any of these | Claude initial review → Codex fixes with a separate Codex verification → fresh Claude closing review |
| B — single review | All other product code, tests, contracts, build and CI changes | One independent review; the same reviewer re-checks the fixes |
| C — checks only | Documentation, briefs, checkpoint and comments that change no requirement, test, build or evidence claim | Required CI |

- When the tier is unclear, use the higher one. Any reviewer may escalate a change.
- Each stage (tests, implementation) gets its own review at the task's tier. Complete
  the tests review before observing intended red. The pre-review run above does
  not replace that review.
- At most two fix rounds per review. Rounds three to five need the
  [advisor's](ADVISOR.md) approval; a sixth needs the owner.
- A material change after approval needs a re-review of the changed part. In tier A
  that means Codex verification plus a new closing review, not a new initial review.
- In tier A, if the initial review finds no blockers, still obtain the Codex
  verification before closing. Do not invent changes to exercise the cycle.

Tiers B and C are a trial that began on 2026-11-14. After ten tier B tasks the
coordinator reports to the owner any defect found later that a full cycle would
plausibly have caught; the owner then keeps, widens or narrows tier A.

## Review handoffs and receipts

Give the reviewer one packet per stage: task and brief, exact commit, owned paths,
the contract sections and acceptance criteria that apply, checks actually run and open
findings. Link the relevant sections instead of sending the whole contract closure.
For a re-check, send only the changed commit, the findings and their resolutions.

For each SQLite catalog step, list every existing reader and its version and
migration marker checks in the first catalog review packet. Include one upgrade
control per reader that exercises its normal entry point after advancing from the
protected catalog to the proposed version.

Each review is one PR comment using [the review template](templates/REVIEW.md): tier,
stage, reviewed commit, provider/model/session, verdict and findings with stable IDs,
blockers marked. Lead with the verdict. Never delete a rejected review.

The reviewer verifies the commit and worktree before reviewing and may read
surrounding code beyond the diff.

## Proof records (tier A only)

The PR comments are the review record. For a tier A task the coordinator also writes
`progress/proofs/TASK-ID.json`, in one commit after the final closing approval and
immediately before merge. That commit adds the record and its entry in
`progress/proof-policy.json`.

- Do not enroll a pending record first and do not commit after each review.
- Record only what happened: actual identities, commits, verdicts and comment URLs.
  Omit unknown times rather than estimating them.
- Check the record before merge:

```sh
python3 tools/proofs.py validate
python3 tools/check_publication.py --base origin/main --head HEAD \
  --task TASK-ID --stage implementation --schema-python .venv/bin/python --complete
```

Repeat `--stage` for every required stage. The schema interpreter comes from the
[isolated schema setup](schema-validation.md). The checker verifies recorded bindings
and outgoing paths; it does not authenticate reviews or grant merge authority.

Tier B and C tasks need no proof record. The 120 tasks finished before the cutover
keep their frozen historical records and exemptions.

## Contracts

By default the contract for a task is its public header or schema plus a table of
examples in the issue or PR. Put edge-case detail in tests, where it is executed,
rather than in prose. The specification stays the single statement of what to build.

- Write a file under `docs/contracts` only for a boundary that no single header
  covers: the wire protocol, the database schema, the scene format. Keep it under
  300 lines; split it or move detail into tests if it grows.
- Existing contracts stay as they are. Trim one when a task next revises it, and
  prefer the header and tests over the prose once the code exists.
- Record each dependency capability a task relies on and whether it is contracted,
  observed or unknown. Unknown provider behavior is never a supported capability.
- A prerequisite found during a task becomes its own issue only when it can merge on
  its own and either blocks more than one task or is larger than about one day of
  work. Otherwise it is a separate commit in the same PR. Do not silently shrink scope.
- Before the first task of an epic starts, a design note of at most one page settles
  the schema steps and their order, the public interfaces and the shared files. Post
  it as a comment on the epic's issue and obtain one tier B review.

## Branches, pull requests and merging

1. Start a branch from current `main`, one per task, and claim the issue on GitHub.
2. Open a focused PR with [the template](../.github/pull_request_template.md); keep it
   a draft while work or review remains. A draft still runs CI, so keep unreviewed
   test source local until its review passes and observe intended red locally.
3. Merge only when latest-head required CI passes, the tier's reviews approve,
   discussions are resolved and GitHub permits the protected merge. Recheck after new
   commits. Never bypass branch protection or hide a failure.
4. After merging, read back the merge commit and close the issue with the PR link.
   Do not open a follow-up PR only to record the merge.

Keep commits focused and do not overwrite another writer's work. Park a blocked
branch until its prerequisites merge, then integrate `main` once before resuming.
Before resuming any work, check the actual checkout, PR head and state, required
checks and the latest owner instruction.

## Temporary files

Build directories and scratch files of a task are disposable. They go under one
directory named after the task, for example `${TMPDIR:-/tmp}/TASK-ID/`, with one
subdirectory per session.

- A delegated author or reviewer deletes its own subdirectory before it reports. This
  does not break a reviewer's read-only role, which covers repository content.
- Before deleting, the session writes the commands and results it relies on into its
  handoff or review comment. Raw evidence that must be kept moves to ignored private
  storage outside the task directory, and the comment says where it is.
- After the merge read-back the coordinator deletes the task directory and removes the
  worktrees it assigned to the task's sessions. It removes a worktree only when no
  session or pending stage uses it and it holds no uncommitted or unpushed work.
  Otherwise it keeps the worktree and reports it; it never forces a removal.
- After the merge read-back the coordinator also deletes the task's local and remote
  branches. It deletes a branch only when all its commits are in `main`, no worktree
  or session uses it and no open or parked PR or pending stage needs it. Otherwise
  it keeps the branch and reports it; it never forces a deletion.
- When a task stops without a merge, the coordinator applies the same checks and
  deletes only what no pending stage needs.
- Delete only the task's own directory and worktrees. Never clear a shared temporary
  location or the files of another live session.
- Apply this cleanup after every merge and include the available space on `/home`
  in each morning coordination report.

## Continuous integration

Add a suite to [the CI workflow](CI.md) when its module lands. Adding a suite to its
existing job, with its library-list entry, is routine task work; any other CI or
`tools/` change needs the owner first. Keep actions and
dependencies pinned and permissions limited. Intentional-red baselines and live
credential checks stay out of passing gates. Never use `WILL_FAIL`,
continue-on-error or skips to hide a failure. Renaming or removing a required job
needs the ruleset's required checks updated in the same change.

## Evidence and privacy

- Distinguish planned, authored, reviewed, observed red, implemented, simulated and
  live/platform-verified. Never claim an unrun check.
- Put commands, results, proof level and open gates in the PR. Keep
  [the evidence summary](evidence/README.md) short and add no per-task receipt files.
- The [checkpoint](../progress/coordinator-checkpoint.md) holds the owner scope, live
  assignments, blockers and the next action. It is not a ledger.
- Use fake connectors, synthetic fixtures and disposable databases. Automated tests
  must not trigger real chat or payment effects; live checks are separate.
- Keep raw evidence in ignored private storage. Inspect every outgoing commit, not
  only the final tree: deleting a file does not remove it from earlier commits.
