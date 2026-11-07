# Continuous project coordinator

Use the **Starting prompt** for a new coordinator session and the **Continuation prompt**
after interruption. These instructions operate within [AGENTS.md](../AGENTS.md), the
[agent workflow](AGENT_WORKFLOW.md), and the complete v0.45 specification. They do not
waive review, test-first, safety, permission or branch-protection gates.

## Standing instructions

Drive the full agreed product toward completion. This is an instruction to execute
bounded work, not only propose plans. Use subagents for bounded authoring, implementation,
investigation and independent review. The same author must not approve a review gate.

### Startup and task selection

Read `AGENTS.md`, `docs/ARCHITECTURE.md`, `docs/AGENT_WORKFLOW.md`,
`docs/TEST_STRATEGY.md`, `docs/PROGRESS.md`, `progress/state.json` and the selected brief.
Read scoped `AGENTS.md` files before editing. Inspect the actual branch, worktree, active
claims, and open PR/check state; do not assume past chat or a checkpoint is current.

Follow the current priority named in `AGENTS.md` (now EPIC-028 test cleanup); otherwise
choose a dependency-ready executable task that advances a product slice. Split epics and
substantial architectural prerequisites into bounded tasks with explicit dependencies,
acceptance criteria and focused PRs before implementation. Preserve all requirements;
planning changes do not authorize a reduced release scope.

### Execution loop

1. Read the brief, relevant specification/contracts, reviewed tests and evidence. Record
   task ID, scope, contract revision, acceptance criteria and shared-file ownership.
2. Start from current `main`, create a dedicated task branch, and claim the task in state.
   Use separate branches/worktrees for concurrent writers; coordinate shared files through
   the integrator.
3. Assign bounded tasks with exact owned paths and durable context. Use a fresh
   implementation session for every executable task; the coordinator may persist.
   Preserve actual author/reviewer separation and retire each implementation session after
   its handoff.
4. For behavior work follow: contract/examples → test authoring → independent test review
   → observed intended red → implementation → independent code review → relevant checks.
   Missing dependencies, compilation failures and skipped tests are not valid red evidence.
5. Keep reviewed assertions intact. Fix genuine test defects only with specification-based
   reasoning and a separate reviewer. Review the actual changed revision; unchanged valid
   approvals remain applicable.
6. Run relevant checks and capture exact commands, outcomes and environment. Assess CI
   coverage for every change; keep intentionally red baselines and live credential checks
   separate from passing product CI. Do not claim unrun or unsupported behavior.
7. Update `progress/state.json`, regenerate `PROGRESS.html`, and follow the after-task
   summary and evidence rules in `docs/AGENT_WORKFLOW.md` (including the 500-character
   total note limit and updating the matching area in `docs/evidence/README.md`).
8. Open one focused PR per executable task. Merge only after latest-head required CI,
   relevant checks, independent reviews, resolved discussions and repository protections
   all pass. Recheck after new commits. Then refresh `main` and select the next ready task.

### Continuity and blockers

Maintain `progress/coordinator-checkpoint.md` at task handoffs, prerequisite splits and
session interruptions. Record observed branch/PR revisions, assignments and ownership,
completed work, actual checks, review gates, blockers and next action. Link task records
and detailed evidence instead of copying logs. Earlier detailed coordination history is
preserved in `https://github.com/Kernol12/subathon-bot-ai/blob/evidence-archive-2026-11/docs/evidence/coordinator-handoff.md`. On resumption verify the real state
first; never assume an agent or process is still active. Resolve routine issues autonomously and
ask the owner only for decisions or access actually needed. No live chat/payment effects
are authorized by these instructions.

Do not stop at an empty ready queue, green scaffold suite or completed labels. The product
is complete only when the full specification and release gates have evidence. Otherwise
record the exact remaining work or blocker and continue with useful authorized tasks.

## Starting prompt

```text
Read AGENTS.md and docs/COORDINATOR_PROMPT.md. Follow the standing instructions and
start executing dependency-ready project tasks, beginning with the current priority in
AGENTS.md. Read architecture, workflow, test strategy, progress state, the checkpoint
progress/coordinator-checkpoint.md and the selected brief. Inspect actual branches,
worktrees, claims and PR state. Use subagents for bounded work and independent reviews, with a
fresh implementation session per executable task. Preserve every test-first and merge
gate, keep one concise task note and PR evidence, and update progress/coordinator-checkpoint.md.
Open focused PRs and continue with the next task after required gates pass. Start now.
```

## Continuation prompt

```text
Resume under AGENTS.md and docs/COORDINATOR_PROMPT.md. Read
progress/coordinator-checkpoint.md if present, plus progress/state.json and linked task
records. Inspect actual branches, worktrees, claims, processes and PR/check/review state
before acting. Preserve valid work and reviews; explicitly reassign stale tasks rather
than assuming agents remain active. Continue the standing task loop with fresh
implementation sessions, independent reviews and all acceptance/merge gates intact.
Update the checkpoint at meaningful boundaries and continue without routine owner
approval. Ask only for decisions or access genuinely needed. Resume now.
```
