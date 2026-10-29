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

## Concurrent work

Use one branch/worktree per writer. Keep commits focused. Coordinate progress/state.json,
shared schemas, migrations, dependency pins and root CMake through the integrator. Do not
cherry-pick incompatible interface versions or overwrite uncommitted work. The package
contains no Git history; initialize a private repository before starting parallel work.

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
