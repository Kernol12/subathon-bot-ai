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

## Done and evidence

Test-authoring done means reviewed tests and intended red demonstrated, not feature done.
Implementation done requires relevant passing acceptance/regression checks and code review.
Record exact commands, exit/result, environment, date, commit and remaining gates. Distinguish
simulation from live service, hardware/browser and package tests. No claim of full correctness
follows merely from a green suite. Use docs/templates/HANDOFF.md before stopping.

## Development settings

Default to fake connectors and disposable databases; no real chat/payment side effects in
automated default tests. Record live checks separately without credentials or personal data.
Follow dependency scopes and instruction files. Routine fixes can proceed; unresolved product
choices are recorded explicitly. No project-hosted authorization broker is planned.
