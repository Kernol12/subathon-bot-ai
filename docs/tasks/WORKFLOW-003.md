# WORKFLOW-003 — Structured coordination and publication

Module: Quality and tooling. Implementation; specification v0.45 §§17–18.
Depends on WORKFLOW-002, CI-001 and RATECONTRACT-002. Owner requested all six
workflow suggestions and a new PR. No product behavior or further product task.
Contract revision: workflow records v1 below. Existing review gates remain mandatory.

## Ownership and acceptance

Coordinator owns this brief, instructions/workflow/progress/architecture docs,
existing evidence summary, task navigation, state/schema/HTML/checkpoint and CI YAML.
Test author owns new `tests/tooling/test_workflow.py` only. Fresh implementation
session owns `tools/workflow.py` and the integration in `tools/progress.py` only.
No new dependencies, assertion deletions, production/schema/migration/SDK changes.

Review contract and test stages independently before executing the new tests.
Observe meaningful assertion failures against working existing tooling, then hand
the frozen tests to a fresh implementer. Independently review implementation and
documentation/CI as one coherent implementation packet, retaining separate gates.
Run tooling discovery, project/render/UI consistency and scaffold checks. Open a
focused PR with actual receipts and current-head CI. Stop after this workflow task.

## Workflow records v1

`progress/state.json` remains the only editable task-status source. Add optional
top-level `workflow_policy`: `{version: 1, legacy_done: [task IDs]}`. Its frozen list
is exactly the tasks already done at base `f2a134e`; retain their original receipts,
without manufacturing structured historical reviews. Validate unique known done IDs.
With this policy, every non-epic task outside that list in `in_progress`, `in_review`
or `done` requires a `workflow` record. Planned/ready/blocked tasks may have one;
any supplied record is validated. Without this policy, workflow/publication records
remain optional; the contract separate-review rule below still applies.

A task's `workflow` has `required_stages` (unique nonempty list) and `stages` (object
with exactly those keys). Stage names: `contract`, `tests`, `implementation`.
Minimum stages by task kind: contract → contract; test_draft/test_review/test_baseline
→ tests; all other non-epic kinds → implementation, with implementation-kind tasks
also requiring tests. Additional stages are allowed. Documentation-only work should
use contract kind instead of inventing acceptance/red evidence. Each stage contains:

- `authors`: unique nonempty session identities; all substantive authors of the packet.
- `revision`: lowercase 40-hex Git commit of the current packet.
- `paths`: unique nonempty project-relative existing files owned by that packet.
- `reviews`: ordered array of receipts, empty while awaiting initial review.

Each receipt requires `phase` (initial/verification/closing), `provider`
(claude/codex), nonempty `model`, `reviewer`, `revision`, `outcome`
(approved/changes_requested), HTTPS `evidence`, and unique string arrays `findings`
and `blocking_findings` (subset of findings). Reviewer must not be any stage author.
Approved receipts have no blockers. Changes-requested receipts have at least one
blocker. Optional `started_at`/`finished_at` must appear together, be timezone-aware
ISO instants and be ordered. They describe observed review time, not estimates.

Initial is first, exactly once and Claude. Verification is Codex and may repeat.
Closing is Claude and needs the immediately preceding receipt to be an approved
verification of its own revision;
after a changes-requested closing, another approved verification must precede the
next closing. A verification after closing invalidates that prior closure until a
new closing approval. An approved closing can be followed by another correction /
verification / closing cycle; no fresh initial is needed. Initial may approve an
earlier revision. A stage is complete only when its last receipt is an approved
closing at the stage revision and its preceding verification approves that same
revision. Receipts are recorded claims; these checks cannot authenticate identity,
Git contents, whether a finding is truly resolved, or a remote CI outcome.

Done tracked tasks need every required stage complete, plus the existing evidence,
separate-review and kind-specific acceptance/baseline requirements. Add contract
kind to the existing separate-approved-review requirement, except the frozen
legacy_done contracts. CONTRACT-001 has no separate review record; preserve that
historical limitation rather than inventing approval or breaking the old baseline.
Unchanged valid approvals remain valid for their packet; a substantive packet edit
updates its revision and repeats verification/closing. The coordinator still checks
actual source/context and all task-specific gates; hashes are not self-verification.

## Publication and safe reconciliation

Publication is separate from artifact status: `done` means the task's bounded work
is complete, not merged. Optional task `publication` is:
`{state: open|merged, pr: HTTPS URL, head: 40-hex, merge_commit: 40-hex|null,
merged_at: timezone-aware ISO|null, observed_at: timezone-aware ISO}`.
Open requires null merge fields; merged requires both, with observed_at >= merged_at.
The PR URL must occur in task evidence. Tracked done tasks require publication, which
may still be open. Dependency satisfaction for a tracked task additionally requires
its publication to be merged. Legacy tasks keep their original dependency semantics.
Apply this predicate consistently in validation, availability and context/HTML.

`python3 tools/progress.py reconcile TASK-ID --pr-json FILE` imports one saved
`gh pr view --json url,state,headRefOid,mergedAt,mergeCommit` object. It accepts only
OPEN or MERGED with the above fields, rejects malformed/mismatched evidence URLs,
null merge data for MERGED, non-null merge data for OPEN, and merged→open regression
or a changed immutable merged record. A repeated valid snapshot is allowed.
It records publication and observed_at, validates the candidate before writing
state/HTML, and changes no status, notes, verification, reviews, authorization or
checkpoint. Errors leave both files unchanged. It performs no network, git or merge
operation; saved JSON is an operator-supplied observation, never live verification.
Serialization/rendering must succeed before either output file is replaced.

The coordinator separately verifies current GitHub head/checks/protections and
reconciles checkpoint prose at handoff. Task-state publication for the same PR is
normally finalized in the next authorized metadata change, not an endless sequence
of self-referential completion PRs. Historical `not_run` values are not rewritten to
passed merely because a later PR merged. Reconcile actual PR118/119 metadata here.

## Derived context and review measurement

Existing `context TASK-ID` output keys/behavior stay unchanged. Add optional
`--stage NAME`. It requires that stage on the task and adds `stage_context` with
`name`, `packet` (the complete stage record), `complete`, `next_gate`, and `metrics`.
Next gate: empty history → initial; after initial or unapproved verification →
verification; approved verification at current packet revision → closing; approved
closing at current packet revision → complete; otherwise → verification.

Metrics: `review_count`; `blocking_findings` (distinct IDs across the stage);
`closing_only_findings` (IDs first appearing at a closing review); both finding
fields are lists in first-appearance order. `elapsed_seconds`
(sum of observed paired timestamps, null if none); `timed_reviews` (count).
First appearances use chronological receipt order, not phase names alone.
No unrelated activity/history or new context files. Existing limitations still apply.
Use observed metrics to evaluate future low-risk review-policy changes; no threshold
or automatic gate waiver is introduced. Do not change the current review cycle.

## CI and documentation packet

Every existing workflow keeps all job bodies, pins, permissions, names and checks.
Add workflow-level concurrency keyed by workflow, event, and PR number (or unique
run ID for other events), cancelling only superseded pull_request runs. Main pushes
and manual runs have unique groups. Scaffold push runs only on main, retaining all
PR runs and main pushes; this removes duplicate feature-push/PR scaffold runs.
No path filters, skipped suites or cache trust changes. SDK caching/reusable setup
needs a separately scoped design before changing artifact admission.

Keep AGENTS.md permanent; point current authorization/assignments to the checkpoint.
Shorten architecture to structural boundaries and contract navigation, preserving
links to the existing evidence summary/history. Update coordinator guidance, progress
docs and PR template for coherent per-stage packets, reconciliation and measured
review outcomes. Do not duplicate stage state in a new manually maintained file.
