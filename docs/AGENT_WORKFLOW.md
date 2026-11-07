# Agent workflow

## Task sequence and roles

For new behavior use: contract/examples → authored tests → independent test review →
observed intended red → implementation → independent code review → relevant checks.
Missing dependencies, compilation failures and skipped tests are not valid red evidence.
An independent reviewer is a different agent/session/person from the author; authors
never approve their own work.

The coordinator selects and decomposes tasks, assigns shared paths and integrates state.
Authors produce bounded work and factual evidence; reviewers assess requirements, oracles,
implementation and gaps. Status lives only in `progress/state.json`; task briefs define
acceptance, and `docs/tasks/README.md` is navigation. Read the brief and scoped
`AGENTS.md` before editing. Production tasks require dependencies, specification refs,
contract revision, acceptance criteria and shared-file ownership.

## Scope and sessions

Epics are planning containers. If a substantial architecture prerequisite emerges, split
it into dependency-linked tasks and focused PRs before implementing the expansion. Preserve
requirements and valid work; do not split every small fix or silently shrink scope.
Use a fresh implementation session for each executable task. Supply its ID, brief,
reviewed context, branch/worktree, owned paths and next action; keep the coordinator as
needed. Use durable links rather than relying on chat history, and never invent a reviewer.

## Branches and pull requests

Before edits, start from current `main`, create one branch per executable task and claim
the task in state. Use separate branches/worktrees for concurrent writers; the integrator
coordinates `state.json`, shared schemas, migrations, dependencies and root CMake.
Open a focused PR to `main` using `.github/pull_request_template.md`; use a draft while
required work or review remains. Include task IDs, specification scope, exact checks,
reviews and remaining gates.

Merge only when latest-head required CI and relevant local checks pass, applicable
independent reviews approve, discussions are resolved and GitHub permits the protected
merge. Recheck after new commits. Never bypass branch protection, hide failures, or
weaken baseline assertions. If access or a gate is unavailable, leave the PR open and
record the concrete blocker. Instructions alone do not configure branch protection.

## CI and concurrent work

Assess CI coverage for each change and add workflows as executable modules, suites,
builds or packaging land. Keep actions/dependencies pinned and permissions limited;
run stable reviewed suites on relevant PRs. Keep intentional red baselines and live
credential checks out of passing product gates. Record actual workflow results and gaps;
new workflows do not automatically become required checks.

Keep commits focused and do not overwrite another writer's work. Batch related reviews
within a test-first stage, not across test and implementation gates. Re-review changed
behavior, tests, contracts, build/CI or material evidence claims; unchanged approvals
remain valid. Correct genuine test defects only with specification-based reasoning and
a separate reviewer.

## Done and evidence

Test-authoring done means reviewed tests and intended red, not implemented behavior.
Implementation done requires relevant passing acceptance/regression checks and separate
code review. Distinguish simulation from live service, hardware, browser and package
evidence; a green scaffold is not product proof.

For a completed task, put its PR URL in `evidence`, one concise outcome/next-step line
in `notes`, and record separate review in the state `review` field and PR. Keep detailed
historical receipts intact and use `docs/evidence/README.md` as their index. Record exact
commands/results and remaining gates in the PR; update state and regenerate `PROGRESS.html`.
The coordinator checkpoint is `progress/coordinator-checkpoint.md`. Do not rerun completed
checks just to improve receipts, and do not put credentials or personal data in evidence.

Default to fake connectors and disposable databases; automated tests must not trigger
real chat/payment side effects. Keep live checks separate. Resolve routine issues within
scope; record unresolved product choices and provider capabilities explicitly.
