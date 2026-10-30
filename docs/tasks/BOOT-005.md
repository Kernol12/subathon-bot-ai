# BOOT-005 — Decompose every release epic

Kind: scaffold. Module: Foundation. Specification: v0.45 §17, DEV-02, TEST-04.
Authoritative status, owner and dependencies: progress/state.json.
Branch: task/BOOT-005-split-epics. Domain contract revision: unchanged.

Scope: all 27 epic briefs and their new child briefs, a backlog navigation/lifecycle
guide, progress/state.json, generated PROGRESS.html and planning evidence. Shared
files are owned by this planning task. Do not implement product behavior, change
the specification or weaken the existing draft acceptance baseline.

Acceptance:

- Every epic has bounded children with specification references, scope, concrete
  acceptance/failure cases, dependencies and explicit verification ownership.
- Production children wait for separately reviewed tests and an intended red run;
  feasibility/live/platform work remains distinct and unverified until executed.
- Preserve all seven integrations, all three rate providers and the complete release
  scope. Retain existing task IDs and recorded evidence without claiming completion.
- Epic completion depends on its required children; the dependency graph is acyclic
  and the generated progress dashboard remains consistent.
- Run existing tooling/scaffold checks, publish the branch and open a PR. The owner
  requested a PR for review, so leave it open rather than merging it in this task.

Handoff: record exact checks, planning decisions, deferred capability questions and
the PR link. No independent product review is established by this planning work.
