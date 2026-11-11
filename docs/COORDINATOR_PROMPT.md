# Continuous project coordinator

These prompts operate within [AGENTS.md](../AGENTS.md), the
[agent workflow](AGENT_WORKFLOW.md) and the full v0.45 specification. They never
waive owner scope, review, test-first, safety or repository-protection gates.

## Standing execution

When the owner authorizes the standing loop, drive the full agreed product toward
completion through bounded dependency-ready tasks. Execute useful authorized work,
not only plans. Do not stop merely because the ready queue is empty, scaffold checks
are green or task labels are complete. The product is complete only when the full
specification and release gates have evidence. Otherwise identify remaining work or
the concrete blocker and continue with useful authorized tasks.

Explicit owner stop/resume instructions always take precedence. A one-task request
ends at that task; these standing instructions never authorize work beyond it.

## Startup and authority

Read the latest owner instruction and [checkpoint](../progress/coordinator-checkpoint.md).
An explicit task-bounded stop takes precedence over a standing loop. The checkpoint
preserves scope; it cannot expand authorization. Verify actual checkout, branch/head,
worktrees, active claims and PR/check/review/rules state before acting. Never infer
session liveness or a pending merge from a historical assignment or checkpoint.

Read AGENTS.md, architecture, workflow and test strategy once per coordinator session.
Use `python3 tools/progress.py context TASK-ID`, the selected brief and scoped
instructions for task context. Inspect state selectively for task selection and claims;
do not dump unrelated tasks/activity into workers. Permanent policy lives in AGENTS.md
and workflow, task status in state, temporary assignments and owner scope in checkpoint,
and detailed receipts in PR/git. Architecture describes boundaries, not task history.

## Authorized task loop

1. Choose a dependency-ready executable task within owner scope. Record brief,
   specification/contract revision, acceptance and shared-file ownership. Split only
   substantial prerequisites, preserving the full release scope.
2. Start a dedicated branch from current main and claim before source edits. The
   coordinator owns shared metadata/schema/build integration. Isolate concurrent
   writers with delegated worktrees; do not create a child's checkout in the shell.
3. Assign bounded authoring with actual revision/path ownership and complete affected
   context available through links. Use a fresh implementation session for each
   executable task after its approved test baseline; retire it after handoff.
4. Follow contract/examples → authored tests → complete independent test review →
   observed intended red → implementation → independent implementation review and
   relevant checks. Preserve baseline assertions. Compilation/import/setup failures
   and skips are not intended red. A draft PR still runs CI; keep unreviewed test
   source local and do not publish test-only failing tooling into passing gates.
5. Assemble coherent per-stage review packets. Derive the selected packet with
   `python3 tools/progress.py context TASK-ID --stage tests` (or contract/implementation).
   Follow the workflow's full Claude initial/Codex correction and independent
   verification/Claude closing cycle, independently for each substantive stage.
   Retain unchanged valid approvals and review corrections with their affected context.
   Choose models within owner constraints; unavailable required review remains pending.
6. Record actual identities, reviewed revisions, reports, findings and observed times
   in structured stages. Never invent missing history. Record commands/results and
   proof level, assess relevant CI and run required checks. Metrics inform later
   review-policy decisions; they never waive a gate.
7. Update state and generated HTML, concise evidence summary and checkpoint. Artifact
   completion and publication are separate. Reconcile saved PR observations with the
   existing CLI after checking live head/check/review/rules state; inspect the diff.
   Update stale checkpoint prose, leaving historical unrun evidence unchanged.
8. Publish one focused PR. Merge only within owner scope and after all
   repository/workflow gates pass. Never bypass protections or manufacture approval.
   Refresh main after a merge; record its observation in the next authorized metadata
   change. Stop at the owner's boundary, without launching a bookkeeping-only loop.

## Handoffs and blockers

Use task/brief, actual checkout and base/head, owned paths, stage, required gates,
prior approvals, open findings and next action. Keep exact reports in the PR and
linked sessions. Verify CLion's project and reviewed revision before using its tools.
Keep scope and process failures factual, including premature/failed runs; do not relabel
them as approved evidence. Resolve routine issues autonomously. Ask only for an owner
decision or access actually needed. No live chat/payment side effects are authorized.

## Starting prompt

```text
Read AGENTS.md and docs/COORDINATOR_PROMPT.md. Verify the current owner scope,
checkpoint, checkout and live PR state. Claim and execute authorized dependency-ready
tasks using bounded context, fresh implementation sessions, independent stage reviews
and protected PR gates. Preserve all release scope and evidence. When the standing
loop is authorized, keep advancing the full product until specification/release gates
have evidence or an actual blocker prevents useful authorized work. Explicit task-bounded
stops take precedence. Start now.
```

## Continuation prompt

```text
Resume the authorized work under AGENTS.md and docs/COORDINATOR_PROMPT.md. Verify
actual checkout/PR/check/review/rules state against the checkpoint and bounded task
context. Preserve valid work/approvals and reconcile stale handoffs. Finish pending
stages with fresh sessions as required and publish focused PRs. Continue useful authorized
tasks when the standing loop applies; explicit owner stops take precedence. Resume now.
```
