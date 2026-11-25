# ADR-0014 — Plain test runs, coordinator-owned run mechanics and leaner tasks

Date: 25 November 2026. Status: decided by the owner on 25 November 2026; in force
when this record merges.
Authority: the owner's decisions of 25 November 2026 after the coordinator stop.
Two sources below are not committed and sit in the primary checkout: the stop report
`build/wind-down/STOP-REPORT.md` and the advisor's decision log
`build/advisor/decisions-2026-11-23.md`.

## Context

[ADR-0013](0013-ci-test-program-admission.md) moved test-program admission into CI
but kept the full local procedure for suites that no CI job runs (its point 4). That
remainder still drove most of the process work:

- On 25 November #588 used six Claude-track sessions, three build attempts, two
  stage attempts and six owner decisions. Its launch stopped in the project's own
  custody script and the test program never started (stop report).
- Pinned local runs needed a rebuild each time `main` changed their inputs. The
  merge holds that protected them blocked product PRs #858, #860 and #864.
- Seven stopped runs on 22–25 November came from the project's own run scripts and
  none from the product (stop report).
- 182 of the 397 lines in the advisor's decision log for 23–25 November mention
  #588 or #238. The stop report puts process questions at about 60 % of the
  advisor's decisions on 25 November.
- From 15 to 25 November (CET) 83 issues were closed and 71 were created. In the
  same period `tests/` grew by about 93,000 lines net and `server/` plus `core/` by
  about 65,000 (`git diff --numstat` from `65936bcb`, the last first-parent commit
  before 15 November, to `6cc23fd8`).

ADR-0013 records that its search found no run in which the local procedure caught a
wrong library, SDK or symbol mismatch, and that the incidents recorded around it
were caused by the procedure.

## Decision

1. **One rule for test runs.** A task has three kinds of run and no others.
   - *Pre-review runs* are unchanged: each candidate suite runs once before the
     first review packet of a stage and earns no test evidence.
   - *Intended red* is one plain local run on the reviewed tests commit, recorded in
     the PR with the commit, the command and the failure summary. The tests review
     reads it and still rejects compile failures, setup failures and skips as red.
   - *The credited pass* is the required CI job on the reviewed PR head. The PR names
     the head commit and the tested merge commit and links the run.

   A suite that no CI job runs joins a CI job in its implementation PR. Adding a
   suite to its existing CI job, together with its entry in `tools/binary_closures/`,
   is part of the task and needs no owner decision. Every other change to CI or to
   `tools/` still goes to the owner before it is written.

   Only when a test cannot run on a hosted runner at all is its credited pass one
   plain local run on the reviewed head. The PR records the commit, the command, the
   result and the reason CI cannot run the test, and the stage's code reviewer reads
   that record before approving. Nothing else is needed for it.

   The local procedure around these runs ends. A local test run needs no pin, seal,
   envelope, custody script, approval record, operator thread or owner confirmation.
   No merge hold is set for a test run and no new `refs/pins/*` is created. Seals and
   identity checks that a CI job performs itself, such as the `native` job's SDK
   gate, are unchanged.

   This replaces ADR-0013 point 4 and the standing test-binary admission in the
   checkpoint. Every other ground for escalating to the owner in
   [docs/ADVISOR.md](../ADVISOR.md) is unchanged, including real providers, live
   credentials and real chat or payments.
2. **The coordinator decides routine run mechanics.** Paths, environment, ref
   refreshes, rebuilds and merge-conflict resolutions are the coordinator's own
   choices. A choice is routine when it changes no test's isolation, needs no
   sandbox exception and weakens no check or assertion. Isolation changes still go
   to the advisor and sandbox exceptions to the owner, and a standing owner rule in
   the checkpoint that stops a particular conflict for the owner still applies. The
   coordinator notes a choice in the PR when a reader needs it. A script that stays
   in the task's ignored build folder, is never committed or used by CI and weakens
   no check or assertion needs no approval. The advisor runs when the coordinator
   ends a turn or asks, and once an hour otherwise. The owner receives two digests a
   day. Up to two Claude tracks may run at once when their material is separate.
3. **Fewer and larger tasks.**
   - A prerequisite found during a task becomes its own issue only when it can
     merge on its own and either blocks more than one task or is larger than about
     one day of work. Otherwise it is a separate commit in the same PR.
   - Before the first task of an epic starts, one design note of at most one page
     settles the schema steps and their order, the public interfaces and the shared
     files. It is a comment on the epic's issue and gets one tier B review.
   - A tier B task uses one PR for both stages. The tests are reviewed on the local
     commit. After that review approves, the author observes intended red locally,
     pushes the reviewed tests commit and opens the draft PR; the tests review and
     the red record are its first comments. The implementation commits follow in the
     same PR. The new tests stay behind the task's default-OFF build option until
     the implementation commits enrol them in their CI job, so no required check
     runs them red. Only the final head, with every required check passing, merges.
     Tier A keeps one PR per stage.
4. **A size limit for tests.** The tests stage of a task adds at most 800 lines of
   tests and fixtures, unless the issue names a larger number and the reason. Write
   one test per row of the examples table and per named edge case, reuse shared
   helpers and generate large fixtures in code. The test reviewer also names every
   test that could be removed without losing a specified behaviour.
5. **Unchanged:** test-first order, specification-derived expected values,
   independent reviews and the three tiers, no weakening of tests, the library-list
   gate of ADR-0013 in the `core` and `qt` jobs with its ten-task trial, and the
   `native` job's SDK gate.

## What is given up

- Passive ELF/ABI hashes, loader-log reconciliation and before/after hashes for
  local runs. A wrong local library would appear as an unexplained result. The
  stage's reviewer classifies such a result before approving.
- The record that a particular local run used a particular sealed build. The commit
  and the command take its place.
- The advisor's second look at routine run mechanics. A mistake there now shows up
  as a failed run or in the code review.

## Alternatives

- **Keep ADR-0013 point 4:** rejected by the owner for its cost; see the context.
- **Fix #588's launch package and run it once more:** rejected. It keeps the
  procedure alive for every later suite outside CI.
- **Shorten the tier A review cycle:** not chosen. A check of the review records of
  the PRs merged since 14 November found later tier A reviews catching real defects
  the first review had missed, for example in #728, #763, #833 and #848.

## Consequences

- The #588 merge hold of 25 November ends when this record merges. #588's launch
  package, pins and `build/SESSION-010/` folders remain as history and are not used
  again. #588 needs a committed driver for its two edge-capture tests, registered
  with CTest, so that they can join a CI job.
- #238's suites join the `qt` job in its Stage 2 PR, and that CI run is the credited
  run.
- `AGENTS.md`, `docs/AGENT_WORKFLOW.md`, `docs/ADVISOR.md`,
  `docs/COORDINATOR_PROMPT.md`, `docs/TEST_STRATEGY.md`, the review and PR templates
  and the checkpoint change as in this PR. Earlier evidence keeps its recorded
  meaning.
- Not part of this record: the MVP milestone in `AGENTS.md`, which the owner decided
  on the same day. It sets the order of work and leaves the release scope unchanged.

## Review of this decision

After three working days the coordinator reports to the owner: PRs merged per day,
issues closed and created, owner decisions per day, the share of advisor decisions
about run mechanics, and any defect that the retired local procedure would plausibly
have caught. The owner then keeps, adjusts or reverses each point.
