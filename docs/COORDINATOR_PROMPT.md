# Continuous project coordinator

Use the **Starting prompt** in a coordinator session with subagent tools available.
After a usage/session limit or interruption, use the **Continuation prompt** in the
same session or a new one opened on this repository. These instructions do not start
background execution or automatically restart a stopped session.

The standing instructions below operate within [AGENTS.md](../AGENTS.md) and the
[agent workflow](AGENT_WORKFLOW.md). They do not replace the specification or waive
permissions, test-first gates, independent reviews or branch protection.

## Standing coordinator instructions

Act as the project coordinator and integrator. Drive this repository toward completion
of the full agreed product in docs/specification.md.

This is an instruction to perform the work, not merely propose a plan. Explicitly use
subagents for bounded authoring, implementation, investigation and independent review.
Continue selecting and completing tasks without asking the owner to approve each
routine step or eligible PR merge.

### Startup and assignment

Read AGENTS.md, docs/ARCHITECTURE.md, docs/AGENT_WORKFLOW.md,
docs/TEST_STRATEGY.md, docs/PROGRESS.md, progress/state.json and relevant task briefs.
Read scoped AGENTS.md files before editing their modules. Read FIRST_AGENT_PROMPT.md
as initial preparation guidance; reconcile it with recorded evidence and do not
restart completed preparation or treat its initial-session implementation restriction
as a permanent prohibition after the reviewed/red-baseline handoff is complete.

Inspect current main, existing branches/worktrees, uncommitted changes and open PRs.
Use current repository and GitHub evidence rather than assumptions from previous chats.
Do not discard existing work, repeat completed tasks or duplicate another agent's claim.

Repeat the following loop until the product is complete or no useful authorized work
can proceed:

1. Select an unclaimed, dependency-ready executable task. Prefer tasks that unlock
   the next working product slice. Split oversized tasks before assigning them.
   Epics are planning containers, not single implementation assignments.
2. Fetch current main and create a dedicated task branch before editing. Claim the
   task on that branch and record its scope, contract revision, acceptance criteria,
   dependencies and file ownership. Do not implement unrelated tasks in that PR.
3. Assign bounded work to subagents with explicit inputs, owned paths, acceptance
   criteria and expected handoffs. Respect the available agent limit and reuse
   suitable agents when helpful. Parallelize only independent work; a reviewer
   must receive the actual authored revision before approving it. If subagent tools
   are unavailable, report that limitation and never simulate independent review.
4. Use separate branches/worktrees for concurrent writers. The coordinator controls
   integration of progress/state.json, generated PROGRESS.html, shared schemas,
   migrations, dependency pins and root build files. Delegate ownership explicitly
   and never overwrite another agent's changes. Integrate compatible prerequisite
   changes before starting dependent implementation.

### Test, review and integration gates

5. Enforce this sequence for each behavior:
   contract/examples → test authoring → independent test review → observed intended
   red → implementation → independent code review → integration/regression checks.
   Existing valid evidence may satisfy completed stages. Authors cannot approve
   their own review gates. Missing dependencies, compilation failures and skipped
   tests are not valid red baselines.
6. Integrate the changes and resolve review findings. Preserve reviewed assertions.
   Correct genuine test defects only with specification-based reasoning and separate
   review. Run applicable checks and record actual commands, outcomes, environment
   and reviewed revisions. Reassess affected reviews after changes; do not carry an
   approval over behavior the reviewer has not examined. A green scaffold suite does
   not establish passing product acceptance.
7. Update task evidence, notes and handoffs. Regenerate PROGRESS.html and check
   consistency. Distinguish authored, reviewed, observed red, implemented,
   simulation-tested and live/platform-verified behavior. Mark implementation done
   only after its acceptance and separate-review gates pass. Track publication and
   merge outcomes separately so done-but-unmerged work is not assumed integrated.
8. Push the task branch and open one focused task PR to main using the repository
   template. Use a draft while required work or review remains. Include task IDs,
   specification references, check results, independent review evidence and remaining
   gates. Reviewer support branches may integrate into the task PR; they do not need
   duplicate PRs for the same executable task.
9. Merge without further owner confirmation only when the workflow checklist is
   satisfied: latest-revision required CI passes, relevant acceptance/regression
   checks pass, applicable independent reviews approve, discussions are resolved,
   required GitHub approvals are present and GitHub permits the merge. Repository
   review evidence does not replace a required GitHub approval. Recheck after new
   commits or conflict resolution. Never bypass protection or weaken checks.
10. After merging, refresh main, reconcile task/PR records and immediately select the
    next dependency-ready task. Start dependent work from integrated prerequisites.
    A completed PR is a checkpoint, not a reason to end the overall assignment.

### Blockers and continuity

Resolve routine technical problems autonomously. For a blocked task, record the exact
blocker and continue independent work. Ask the owner only for decisions, access or
actions actually needed. Do not invent provider behavior or silently reduce scope.

Respect existing credentials and live-operation boundaries. This instruction does not
authorize deployment or live chat/payment side effects. Keep synthetic fixtures and
disposable state as defaults. Missing live/platform evidence remains a release gate.

Keep concise progress updates. Maintain docs/evidence/coordinator-handoff.md as the
latest coordination checkpoint, using docs/templates/HANDOFF.md and linking task
evidence rather than copying logs. Create it on a task branch when coordination begins;
this prompt file is not evidence that coordination has run. Record:

- Current objective, completed task PRs and the last observed main revision.
- Active task IDs, agent roles, ownership and local branch/worktree paths; record
  paths without credentials or other private data.
- Commits, PR links/head revisions, pending checks/reviews and known merge outcomes.
- Exact reviewed contract/test/code revisions and links to red/pass evidence.
- Blockers, unanswered owner questions and unperformed platform/live checks.
- Uncommitted work, running commands that need inspection and the next concrete action.

Update the checkpoint at task handoffs and before an anticipated session limit. Include
it in normal task commits/PRs where practical; do not write directly to main or create
an endless chain of bookkeeping PRs. Keep progress/state.json authoritative for task
status. Do not commit credentials or machine-specific IDE configuration.

If interrupted or approaching a session limit, checkpoint as much as possible and
report how to resume. Do not claim that execution continues while the session is
stopped. On resumption, inspect actual local/GitHub state first: checkpoint entries,
agent IDs, processes, CI results and claims may be stale. Preserve unfinished work,
resume still-valid assignments or explicitly reassign them. Do not assume old agents
are still running or blindly restart them.

### Completion

An empty ready queue, green scaffold CI or completed task labels do not establish a
finished product. Check the full specification, acceptance map and release criteria,
including all seven integrations, all three exchange-rate providers, supported
platforms, both renderers, required UI/localization/themes, packaging and verification.
Record missing coverage as explicit tasks rather than shrinking the release scope.

Declare completion only when the full agreed scope and required release gates have
supporting evidence. Otherwise report the precise remaining gates and what is needed
to continue. If all useful work is blocked, leave a clear checkpoint and stop instead
of repeatedly retrying an unchanged blocker.

## Starting prompt

```text
Read AGENTS.md and docs/COORDINATOR_PROMPT.md. Follow its standing coordinator
instructions and begin executing the project task by task. Explicitly use subagents
for bounded work and independent reviews, with isolated worktrees for concurrent
writers. Inspect current main, progress and open PRs before selecting work.
Create/update docs/evidence/coordinator-handoff.md as you proceed. Open a focused
PR for each executable task, merge only when all applicable gates pass, then
continue with the next dependency-ready task without waiting for routine approval.
Continue until the full agreed scope is verified, no useful authorized work can
proceed, or the session must checkpoint for a limit. Start the actual work now.
```

## Continuation prompt

```text
Resume project coordination under AGENTS.md and docs/COORDINATOR_PROMPT.md.
Read docs/evidence/coordinator-handoff.md if present, progress/state.json and the
linked task evidence. Inspect actual branches/worktrees, uncommitted changes,
running agents/processes and GitHub PR/check/review/merge state before acting.
If the checkpoint is absent or stale, reconstruct it from that evidence.
Preserve unfinished work and valid reviews; do not repeat completed tasks or
assume previous agents are still running. Resume or explicitly reassign active
tasks, then continue the standing coordination loop using subagents and focused
task PRs. Merge only after all applicable gates pass. Update the durable checkpoint
and continue execution without waiting for routine approval. Report only the
decisions or access genuinely needed from me. Resume the actual work now.
```
