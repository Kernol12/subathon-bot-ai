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
- Expected values come from the specification, never from the implementation.
- Do not delete, skip or weaken reviewed assertions to make code pass. Correct a
  genuine test defect only with specification-based reasoning and a separate reviewer
  (ADR-0012 covers planned cleanup).
- A reviewed red baseline is not an implemented feature. Implementation is done when
  the reviewed tests pass, its review approves and the PR is merged.

One feature is one GitHub issue with two stages: tests, then implementation. The
issue body is the brief: it defines scope and acceptance. A file under `docs/tasks`
exists only for a task whose brief is too long for an issue. Issue bodies imported
from the old tracker name a separate `…TEST-nnn` baseline task; read that as the
tests stage of the same issue. Use one PR per stage when the stages are reviewed
apart, otherwise one PR.

## Roles

The coordinator selects dependency-ready tasks inside the owner scope, assigns paths,
sets the review tier, integrates shared files and merges. Authors produce bounded work
and factual evidence. Reviewers stay read-only and assess requirements, oracles,
implementation and gaps.

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
  the tests review before observing intended red.
- At most two fix rounds per review. Ask the owner before a third.
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
- If a substantial prerequisite appears, split it into its own task before expanding
  scope. Do not split small fixes and do not silently shrink scope.

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

## Continuous integration

Add a suite to [the CI workflow](CI.md) when its module lands. Keep actions and
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
