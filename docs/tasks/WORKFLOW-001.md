# WORKFLOW-001 — T3 coordination and derived task context

Module: Quality and tooling. Kind: implementation. Specification v0.45 §§17–18;
workflow tooling only, no product behavior or release-scope changes. Depends on
EVIDENCE-002 and APITEST-012. Contract revision: context CLI v1 below.

## Ownership and scope

Coordinator owns this brief, AGENTS.md, CLAUDE.md, docs/AGENT_WORKFLOW.md,
docs/COORDINATOR_PROMPT.md, docs/PROGRESS.md, docs/tasks/README.md,
progress/state.json, PROGRESS.html, progress/coordinator-checkpoint.md and the
existing foundation entry in docs/evidence/README.md. Test author owns only
new context cases in tests/tooling/test_progress.py. Fresh implementation worker
owns tools/progress.py. Claude independently reviews tests and final changes.

## Context CLI v1 contract and literal examples

`python3 tools/progress.py context TASK-ID` validates existing state, then prints
one JSON object to stdout without writing state, HTML or generated context files.
Unknown IDs fail nonzero with `Unknown task: TASK-ID` on stderr and no stdout.
Invalid state follows existing validation/error handling. Existing commands retain
behavior. Output keys:

- `specification_version`, `state_updated_at`: exact recorded strings.
- `task`: complete selected record, preserving notes, spec_refs, owner, verification,
  review and evidence; no historical activity or unrelated full task records.
- `dependencies`: direct dependencies in declared order, each containing `id`,
  `status`, `owner`, `brief`, `evidence`, `verification`, `review`.
- `dependents`: direct dependent task IDs in source order.
- `shared_brief_tasks`: other task IDs with the same brief, in source order.
- `available`: boolean using exactly the dashboard rule: non-epic, planned/ready,
  all dependencies done. Availability grants no permission to resume stopped work.
- `limitations`: nonempty text explicitly identifying recorded information only;
  live GitHub/CI, session liveness and checkout revisions are not verified.

Literal fixture: CTX-001 done; CTX-002 planned depends on CTX-001;
CTX-003 planned depends on CTX-002 and shares CTX-002's brief. Context CTX-002
has dependencies [CTX-001], dependents [CTX-003], shared_brief_tasks [CTX-003],
available true. CTX-003 unavailable while CTX-002 planned. Active, blocked, done
and epic records are unavailable even when dependencies are done.

## Acceptance and gates

Independently reviewed focused tests before intended behavior red; argparse rejects
context while the existing CLI works (not a compile/setup failure). Fresh worker then
implements against unchanged assertions, reusing dashboard availability logic.
Independent final code/docs review, tooling discovery, check_project, render --check,
relevant existing scaffold checks and latest required CI precede protected merge.
No new dependencies, test deletions, product bodies or claim-schema changes.

Document Sol coordination/Codex author and fresh implementer/Claude stage reviews,
precise handoffs, verified checkouts for CLion, no silent quota fallback, scoped
instruction loading and stable session identities without claiming they prove review
independence. Shorten checkpoint while preserving prior PR108 receipts in git/PR;
retain stop-after-APITEST-012 except this explicitly authorized workflow task.
After this task's normal merge stop; API-012 remains planned and unclaimed.
