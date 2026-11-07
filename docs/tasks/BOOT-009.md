# BOOT-009 — Keep agent assignments and review overhead bounded

Kind: scaffold (workflow documentation). Module: Foundation.
Specification: v0.45 §17, DEV-02, TEST-04, TEST-05.
Status/owner/dependencies: progress/state.json.
Branch: task/BOOT-009-bounded-agent-workflow, from main ba376fb.

Scope/shared ownership: root AGENTS.md, docs/AGENT_WORKFLOW.md,
docs/COORDINATOR_PROMPT.md, handoff/review templates, this brief, task evidence,
coordinator checkpoint, progress/state.json and generated PROGRESS.html.
Owner authorized these documentation changes only; do not resume product work here.

Acceptance:

- Require substantial architectural prerequisites discovered during a task to be
  split into explicit tasks/branches/PRs with preserved scope and dependencies.
- Use a fresh implementation agent session for each executable task, with concise
  inputs and durable handoffs; the coordinator may remain active.
- Consolidate task evidence and batch related review changes within each existing
  stage; preserve independent review, test-first chronology and latest-head CI.
- Include the practices in both copyable coordinator prompts and reconcile the
  stale PR77 checkpoint so the next owner-requested continuation can resume safely.
- Obtain an independent documentation review, run project/progress/tooling checks,
  and publish this focused documentation PR without changing product behavior.

No production code, public contract, test assertion, dependency pin, workflow or
release requirement changes. Existing CI covers this documentation/progress change.
