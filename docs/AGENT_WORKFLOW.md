# Agent roles, sequencing and handoffs

## Required sequence for a behavior

Contract/examples → test authoring → independent test review → observed intended red
→ implementation → independent code review → integration/regression checks.

A separate reviewer means a different agent/session/person from the author. A single
CLion agent may work sequentially across tasks, but cannot approve its own review gate.
Use docs/templates/REVIEW.md and name the author/reviewer in evidence. Do not claim a
review has happened merely because an agent wrote a checklist.

| Role | Scope / handoff |
| --- | --- |
| Coordinator/integrator | Plan dependency-ready tasks; assign shared files; merge reviewed changes; preserve scope and progress data |
| Test author | Derive expected results from specification; write deterministic tests; record gaps and baseline revision |
| Test reviewer | Check oracle/requirements/boundaries; approve or request test changes separately |
| Implementer | Satisfy reviewed tests; add meaningful lower-level/regression tests; preserve baseline integrity |
| Code reviewer | Check behavior beyond existing assertions, domain/thread/data boundaries and evidence |

## Task records

Status is authoritative in progress/state.json. Detailed task briefs under docs/tasks/
contain acceptance steps, not a competing status database. Use the state schema and
CLI in docs/PROGRESS.md. New tasks need IDs, module, kind, dependencies, specification
references and a brief. Split large feature tasks before assigning them. Include precise
failure cases and applicable test suites. Status values are ready/planned/in_progress/
in_review/blocked/done; test and platform evidence are separate fields.

Use the [epic/child-task index](tasks/README.md) to select a bounded assignment.
Production slices have separate baseline records that require contract/examples,
test authoring, independent review and observed red before implementation can start.

## Architectural prerequisites discovered during work

Reassess the task boundary when implementation or review reveals a substantial new
prerequisite: for example a native process supervisor, reusable subsystem, new runtime
or a separate contract/build/verification effort. Before implementing that expansion,
create explicit task records and briefs with their own acceptance criteria and
test-first prerequisites, update dependencies without cycles, and use separate task
branches and focused PRs. A small internal adjustment within the existing contract
and acceptance scope can remain in the current task; do not split every bug fix.

Preserve the original requirements and valid work. Checkpoint the dependent task,
mark it `blocked` when adding an unfinished prerequisite (as required by the progress
validator), and leave its PR draft while acceptance is incomplete. Start the prerequisite
from current main and integrate it before resuming dependent implementation. Reuse reviewed
artifacts only where their revisions and scope remain applicable. Record the split
and file ownership in the handoff. If the original task has no independently acceptable
slice, keep it unfinished rather than merging partial behavior as complete.

The coordinator may perform this decomposition autonomously within agreed scope.
Raise requirement conflicts or changes to product behavior explicitly; task splitting
does not authorize reduced guarantees or unrelated features.

## Agent session boundaries

Use a fresh implementation agent/session for each executable task. With subagent
tools, the coordinator starts the fresh agent itself; the owner need not manually
open a session per task. Retire the implementation agent after its task and handoff
are complete rather than reusing its accumulated history for the next task. The
coordinator may remain active across tasks. Do not restart solely because time passed
or discard useful active work after an interruption.

Give the fresh agent the task ID, brief, specification/contract revisions, relevant
reviewed tests/red evidence, branch/worktree, owned paths, acceptance criteria and
next action. Link durable context rather than forwarding an entire previous chat.
Reuse a reviewer for related revisions of the same task when useful, keeping author
and reviewer identities separate; start fresh author/reviewer assignments for the
next task. If fresh sessions are unavailable, checkpoint and report the limitation;
do not invent a fresh identity or self-certify an independent review.

## Branches and pull requests

Before editing, fetch the current `main` and create one dedicated branch per executable
task, for example `task/EPIC-010-TASK-ID-summary`. Claim the task on that branch and
record the branch, scope and shared-file ownership. Epics are planning containers:
split them into dependency-ready tasks and use focused PRs rather than one large epic PR.

Push the branch and open a pull request targeting `main`, using the repository PR
template. Include task/epic IDs, specification references, actual check results,
applicable independent reviews, remaining gates and updated progress evidence.
Use a draft PR while required work or reviews remain.

Agents are authorized to merge their task PRs without further owner confirmation when:

- Required CI checks pass for the latest PR revision, and relevant local checks pass.
- Applicable specification-linked test-first and quality gates are satisfied, including
  separate test/code reviews where required. Authors cannot certify their own
  independent review. A green scaffold check alone does not approve product behavior.
- Review discussions are resolved, required approvals are present, and GitHub reports
  the PR mergeable under the repository's branch protection and rulesets.
- Task evidence and generated progress are current; remaining release/platform gates
  are explicit and do not block the task's acceptance criteria.

Check these conditions again after any new commit or conflict resolution. Never bypass
branch protection, force-push `main`, or delete, skip or weaken assertions to make CI
pass. The documented intentionally red draft baseline remains a separate gate; it is
not a waiver for failed required checks or missing acceptance of implemented behavior.
If checks, permissions or reviews are unavailable, leave the PR open and report the
specific blocker. Record the PR link and merge outcome in the handoff.

GitHub branch protection/rulesets should enforce required checks and reviews; written
instructions alone do not configure that enforcement.

## Evolving continuous integration

For every task, assess whether CI checks the changed behavior adequately. Add or update
GitHub Actions as new executable modules, test suites, build targets and packaging
introduce checks that should run automatically. Include workflow changes in the task
scope and review. Reuse the same selected commands and presets locally and in CI;
keep dependencies/actions pinned and permissions limited to what the job needs.

Run stable, reviewed acceptance and regression suites automatically on relevant PRs.
Add platform, browser and packaging jobs as those capabilities become executable.
Keep intentional red baselines separate and visible until the implementation passes;
do not add known-red work to a passing merge gate or hide failures by skipping tests,
failure inversion or blanket continue-on-error. Credential-dependent live checks remain
separate documented operations, not side effects of default CI.

Record the CI coverage assessment, actual workflow results and remaining gaps in each
task handoff/PR. Create explicit follow-up tasks for missing infrastructure or platform
coverage; a gap that blocks task acceptance must be resolved before completion/merge.
Check new or changed workflows on the PR before merging. Keep check names stable and
avoid filters that can omit a required check. Record any required branch-protection or
ruleset update explicitly: adding a workflow does not make its check required. Do not
claim repository enforcement unless the actual configuration has been verified.

## Concurrent work

Use one branch/worktree per writer. Keep commits focused. Coordinate progress/state.json,
shared schemas, migrations, dependency pins and root CMake through the integrator. Do not
cherry-pick incompatible interface versions or overwrite uncommitted work. Base task
branches/worktrees on the repository's current `main`.

## Review and baseline changes

Implementation must not skip/delete/weaken reviewed assertions to get green. Correct
wrong tests through a specification-linked change plus independent review. For a changed
contract record the old/new revision, affected tasks and migration/compatibility impact.
Test-first is per module: begin with core contracts/tests and extend to later modules
when their contracts exist. Never invent provider behavior to make an early test possible.

Batch coherent related changes into one review packet within each stage. Supply
the exact revision, changed paths, relevant requirements, findings and check evidence.
A packet may cover several related tests or fixes; do not request a new review for
every bookkeeping edit. Keep the sequence intact: test approval and intended red
precede implementation, then a different reviewer checks the actual code. Do not
batch across these gates or defer a blocking finding until after merge.

After revisions, identify the affected review scope and obtain review of changed
behavior, contracts, tests, build/CI or material evidence claims. Unchanged reviewed
artifacts retain their valid approvals. Pure formatting, link or clerical changes
need normal accuracy checks, not a separate new metadata-review stage unless they
alter a reviewed claim. Latest-head CI and all required GitHub approvals still apply.

## Done and evidence

Test-authoring done means reviewed tests and intended red demonstrated, not feature done.
Implementation done requires relevant passing acceptance/regression checks and code review.
Record exact commands, exit/result, environment, date, commit and remaining gates. Distinguish
simulation from live service, hardware/browser and package tests. No claim of full correctness
follows merely from a green suite. Use docs/templates/HANDOFF.md before stopping.

Keep one current task handoff as the entry point for scope, decisions, status,
reviewed revisions, commands/results, remaining gates and next action. Link detailed
logs and review records; summarize each finding and its resolution in the applicable
review record instead of creating a new narrative for every coordination step.
Separate records are appropriate for distinct baselines, architecture decisions or
substantial independent audits. Preserve historical red/pass receipts and review
attribution; consolidation is not permission to delete or rewrite them. Do not rerun
completed checks merely to obtain cleaner evidence.

The coordinator checkpoint links task handoffs and records integration/ownership
state. Progress JSON holds task status, and PR descriptions summarize the result;
avoid copying full histories or logs into each of them. Update at meaningful task,
review, integration and interruption boundaries rather than after every message.

## Development settings

Default to fake connectors and disposable databases; no real chat/payment side effects in
automated default tests. Record live checks separately without credentials or personal data.
Follow dependency scopes and instruction files. Routine fixes can proceed; unresolved product
choices are recorded explicitly. No project-hosted authorization broker is planned.
