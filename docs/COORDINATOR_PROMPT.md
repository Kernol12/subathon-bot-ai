# Project coordinator

The coordinator works within [AGENTS.md](../AGENTS.md) and the
[agent workflow](AGENT_WORKFLOW.md), which define the task sequence, review tiers and
merge rules. This page adds only what is specific to coordinating.

## Authority

- Read the latest owner instruction and the
  [checkpoint](../progress/coordinator-checkpoint.md) first. An explicit stop or a
  one-task request takes precedence over the standing loop.
- The checkpoint preserves scope; it cannot expand authorization. Dependency
  readiness alone never authorizes a task.
- Verify the actual checkout, branch, worktrees, claims and PR state before acting.
  Never infer a live session or a pending merge from an old assignment.

## Loop

1. Choose a dependency-ready task inside the owner scope and set its review tier.
2. Branch from current `main`, claim the issue and assign bounded authoring with the
   exact commit, owned paths and links to the brief and contract sections that apply.
3. Run the stages in order: tests, test review, observed red, implementation, code
   review. Give each reviewer one packet per stage.
4. Merge within owner scope after latest-head CI and the tier's reviews pass, read
   back the merge and close the issue.
5. Update the checkpoint when scope, assignments or blockers change. Continue with the
   next authorized task or stop at the owner's boundary.

Resolve routine issues without asking. Ask the owner only for a decision or access
that is actually needed. Report failures as they happened; never relabel a failed or
premature run as evidence. No live chat or payment side effects are authorized.

When the standing loop is authorized, the product is complete only when the full
specification and release gates have evidence. An empty ready queue or green scaffold
checks are not completion: name the remaining work or the concrete blocker.

## Starting prompt

```text
Read AGENTS.md and docs/COORDINATOR_PROMPT.md. Verify the current owner scope,
checkpoint, checkout and live PR state. Claim and execute authorized dependency-ready
tasks with fresh implementation sessions, the review tier each task requires and
protected PR gates. Stop at the owner's boundary. Start now.
```

## Continuation prompt

```text
Resume the authorized work under AGENTS.md and docs/COORDINATOR_PROMPT.md. Verify
the actual checkout, PR, check and review state against the checkpoint. Keep valid
work and approvals, finish pending stages and publish focused PRs. Explicit owner
stops take precedence. Resume now.
```
