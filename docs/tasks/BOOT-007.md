# BOOT-007 — Add a resumable coordinator prompt

Kind: scaffold (workflow documentation). Module: Foundation.
Specification: v0.45 §17, DEV-02, TEST-04, TEST-05.
Status/owner/dependencies: progress/state.json.
Branch: task/BOOT-007-coordinator-prompt.

Scope/shared ownership: docs/COORDINATOR_PROMPT.md, README.md, this brief,
docs/evidence/coordinator-prompt.md, progress/state.json and generated PROGRESS.html.
No product code, contract, assertion or release requirement changes.

Acceptance:

- Provide a standing coordinator prompt with explicit subagent delegation, bounded
  tasks, isolated writers, independent review and one gated task PR at a time.
- Continue selecting dependency-ready tasks after eligible merges; preserve all
  specification and release gates without assuming completed tasks need repeating.
- Include copyable starting and continuation prompts with durable checkpoints and
  reconciliation of local/GitHub state after interruption. Session limits do not
  imply automatic background execution or lost review evidence.
- Link the prompt from the README without replacing initial environment guidance.
- Record actual documentation/tooling checks and publish a focused task PR.
