# Agent workflow

## Task sequence and roles

For new behavior use: contract/examples → authored tests → independent test review →
observed intended red → implementation → independent code review → relevant checks.
Missing dependencies, compilation failures and skipped tests are not valid red evidence.
An independent reviewer is a different agent/session/person from the author; authors
never approve their own work.

The coordinator selects and decomposes tasks, assigns shared paths and integrates state.
Authors produce bounded work and factual evidence; reviewers assess requirements, oracles,
implementation and gaps. Status lives only in `progress/state.json`; use
`python3 tools/progress.py context TASK-ID` for a read-only task view. Task briefs define
acceptance, and `docs/tasks/README.md` is navigation. Read the brief and scoped
`AGENTS.md` before editing. Production tasks require dependencies, specification refs,
contract revision, acceptance criteria and shared-file ownership.

## T3 roles and handoffs

Sol is the Codex coordinator: it selects tasks, assigns paths, integrates shared
files and owns state, evidence, CI inspection and protected merges. A Codex worker
authors contract examples/tests and captures reviewed intended red. A fresh Codex
implementation session receives that approved baseline. Separate Claude sessions
review the test stage against the specification and the finished implementation.
These are two default review stages, not a ceiling: retain every task-specific
contract, source/resource, dependency/license, build/CI and evidence gate. Substantive
coordinator-authored changes also require independent review. Session names alone
and reviews posted through the owner's GitHub account do not prove independence or
constitute a second human GitHub approval.

Use stable provider/session identities and durable T3 thread links in handoffs and
review evidence. Record task owner/status in state; record temporary session/worktree
assignments in the checkpoint. Never assume an old assignment is a live process.
Each handoff includes task ID, brief, base/head SHA, actual checkout, owned paths,
relevant spec/contract revisions, acceptance criteria and required gates, previous
findings and expected output. The reviewer verifies the revision and worktree before
review, including material uncommitted changes, and may inspect surrounding code and
requirements beyond the supplied diff. Focused rechecks retain the earlier findings
and access to the complete affected context; unchanged valid approvals remain valid.

T3 creates isolated delegated worktrees for concurrent writers. Shell tools and
existing build presets remain available. CLion MCP may assist only when its project
path and reviewed revision match the intended checkout; otherwise use that checkout's
file/shell tools. Do not assume the main IDE project represents a worker worktree.
Use bounded Claude reviews and avoid rereading global history for each worker. If a
provider's quota blocks a required review, checkpoint the pending gate and continue
only independent authorized work. Do not silently switch provider/model, buy usage or
use paid API fallback. Obtain explicit authorization for a change of those choices.

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

For a completed task, put its PR URL in `evidence`, keep all of the task's `notes`
together to 500 characters or fewer, and record separate review in the state `review`
field and PR. The note states outcome, proof level (simulated, real SQLite, CI or live),
open gates and next step. Update the matching area entry in `docs/evidence/README.md`;
add no files under `docs/evidence/` and no per-task history to `docs/TEST_STRATEGY.md`.
Put commands, results and details in the PR. Historical receipts will be available in git
history under tag `evidence-archive-2026-11` once EVIDENCE-001 lands; the summary is the
readable record. Update state and regenerate `PROGRESS.html`. The coordinator checkpoint
is `progress/coordinator-checkpoint.md`. Do not rerun completed checks just to improve
receipts, and do not put credentials or personal data in evidence.

Default to fake connectors and disposable databases; automated tests must not trigger
real chat/payment side effects. Keep live checks separate. Resolve routine issues within
scope; record unresolved product choices and provider capabilities explicitly.
