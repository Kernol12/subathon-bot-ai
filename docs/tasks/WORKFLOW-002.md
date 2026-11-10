# WORKFLOW-002 — Opus stage reviews with Codex correction cycles

Module: Quality and tooling. Kind: implementation. Specification v0.45 §§17–18;
documentation only, no product behavior or release-scope changes. Depends on
WORKFLOW-001. Contract revision: review cycle v1 in docs/AGENT_WORKFLOW.md.

## Ownership and scope

The owner authorized this guidance update only. Product work remains stopped after
API-012; APITEST-001/API-001 and session-clock binding remain unauthorized.
The fresh Codex author owns AGENTS.md, CLAUDE.md, docs/AGENT_WORKFLOW.md and
docs/COORDINATOR_PROMPT.md. The coordinator owns this brief, docs/tasks/README.md,
progress/state.json, PROGRESS.html, progress/coordinator-checkpoint.md and the
existing foundation entry in docs/evidence/README.md. Reviewers inspect read-only.

## Acceptance and gates

- Each substantive review stage starts with independent Opus review. Codex fixes
  findings; a separate Codex reviewer checks the findings, related edge cases and
  affected requirements until no blocking findings remain. Independent Opus closing
  review is required; further blockers repeat the Codex cycle and Opus closing gate.
- Keep Opus as the Claude reviewer, explicitly select the model and do not substitute
  Sonnet. Codex readiness never substitutes for required Opus approval.
- Preserve separate test-before-red and implementation stages and every task-specific
  source/resource, dependency/license, build/CI, evidence and protected-merge gate.
  Batch related review work within a stage. Routine metadata may use independent
  Codex review unless substantive requirements/evidence or an explicit gate requires
  Opus. No fixed review-count limit may waive unresolved findings.
- Use concise revision/path/finding handoffs with prior approvals and complete
  affected context available. Preserve independent author/reviewer identities.
- Initial Opus review, independent Codex verification and closing Opus review of this
  documentation; project/link/progress checks and latest required CI before normal
  protected merge. Record actual results without inventing red/native evidence.

No production code, tools, tests, dependencies or provider settings change. No new
runtime behavior requires a test-first red. Stop after this guidance task.
