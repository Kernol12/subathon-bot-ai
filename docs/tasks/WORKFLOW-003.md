# WORKFLOW-003 — Structured coordination and publication

Module: Quality and tooling. Implementation; specification v0.45 §§17–18.
Depends on WORKFLOW-002, CI-001 and RATECONTRACT-002. Owner requested all six
workflow suggestions and a new PR. No product behavior or further product task.
Contract revision: workflow records v1 below. Existing review gates remain mandatory.

The six improvements are structured review gates, publication reconciliation,
bounded task context, coherent stage-review packets, measured review usefulness
before any policy reduction, and lower duplicate CI work without coverage changes.

## Ownership and acceptance

Coordinator owns this brief, instructions/workflow/progress/architecture docs,
existing evidence summary, task navigation, state/schema/HTML/checkpoint and CI YAML.
Test author owns new `tests/tooling/test_workflow.py` only. Coordinator also owns a
setup-only adjustment in `tests/tooling/test_progress.py`: remove workflow_policy
only in tests replacing tasks with synthetic fixture records, leaving every existing
assertion unchanged. Retain the policy for tests mutating the real task list and
keep test_baseline_is_valid on the complete real state. Review this fixture isolation
with the new tests. Fresh implementation
session owns `tools/workflow.py` and the integration in `tools/progress.py` only.
No new dependencies, assertion deletions, production schema/migration/SDK changes.
The developer-only progress/state.schema.json receives the new optional record
shapes; existing structural schema cases remain unchanged and run in selected CI.

Review contract and test stages independently before executing the new tests.
Observe meaningful assertion failures through existing progress.validate/context/main
(never an import failure for the not-yet-existing workflow module), then hand
the frozen tests to a fresh implementer. Independently review implementation and
documentation/CI as one coherent implementation packet, retaining separate gates.
Run tooling discovery, project/render/UI consistency and scaffold checks. Open a
focused PR with actual receipts and current-head CI. Stop after this workflow task.

## Workflow records v1

`progress/state.json` remains the only editable task-status source. Add optional
top-level `workflow_policy`: `{version: 1, legacy_done: [task IDs],
legacy_done_sha256: SHA256}`. The digest is lowercase hex SHA256 of sorted IDs joined
with LF and one final LF (empty list: empty bytes), checked by validation. Its frozen list
is exactly the tasks already done at base `f2a134e`; retain their original receipts,
without manufacturing structured historical reviews. Validate unique known done IDs.
The repository's 120-ID baseline fingerprint is
`04afa2e0c32d4d88183e8b104f9d65402c81bd4e32521406c3d36a98e881e573`.
The list and fingerprint are frozen reviewed policy, not automatic grandfathering;
changing either is a substantive policy change requiring independent approval.
With this policy, every non-epic task outside that list in `in_progress`, `in_review`
or `done` requires a `workflow` record. Planned/ready/blocked tasks may have one;
any supplied record is validated. Without this policy, workflow/publication records
remain optional; the contract separate-review rule below still applies.
"Tracked" means a non-epic task outside legacy_done when workflow_policy is present.
Supplied workflow/publication records are validated even when no policy exists.

A task's `workflow` has `required_stages` (unique nonempty list) and `stages` (object
whose keys are a subset of that list). Stages are added only when authoring starts;
unstarted stages need no invented author, paths or revision. Stage names:
`contract`, `tests`, `implementation`.
Minimum stages by task kind: contract → contract; test_draft/test_review/test_baseline
→ tests; all other non-epic kinds → implementation, with implementation-kind tasks
also requiring tests. Additional stages are allowed. Documentation-only work should
use contract kind instead of inventing acceptance/red evidence. Each stage contains:

- `authors`: unique nonempty session identities; all substantive authors of the packet.
- `revision`: lowercase 40-hex Git commit of the current packet.
- `paths`: unique nonempty project-relative existing files owned by that packet.
- `reviews`: ordered array of receipts, empty while awaiting initial review.

Exclude progress/state.json, PROGRESS.html and progress/coordinator-checkpoint.md
from stage paths: recording receipts/publication/progress is routine bookkeeping,
not a packet revision. Material requirements or evidence claims in metadata still
need independent review with the affected packet, but adding an actual approval
receipt never invalidates that approval. Keep packet revisions tied to reviewed
source/contract/test/schema/docs/CI changes, not the recording commit.

Each receipt requires `phase` (initial/verification/closing), `provider`
(claude/codex), nonempty `model`, `reviewer`, `revision`, `outcome`
(approved/changes_requested), HTTPS `evidence`, and unique string arrays `findings`
and `blocking_findings` (subset of findings). Reviewer must not be any stage author.
Approved receipts have no blockers. Changes-requested receipts have at least one
blocker. Optional `started_at`/`finished_at` must appear together, be timezone-aware
ISO instants and be ordered. They describe observed review time, not estimates.
Normalize terminal Z for the Python3.10 floor. Equal instants are allowed. Receipt
order is array order; timestamps need not be ordered across receipts (reviews may
overlap). Finding IDs are stage-unique: prefix new IDs by reviewer/review identity;
reuse an ID only when referring to the same original finding. A reviewer who later
authors a correction must be added to authors, invalidating that session's approvals;
assign a different reviewer to avoid self-approval.

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

Done tracked tasks need every required stage present and complete, plus the existing evidence,
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
This predicate applies to the prerequisite, not to the dependent being selected.
Publication head is an observation at observed_at, not a promise to equal future
PR heads. Latest-head CI and actual merge readiness remain separate live checks.

`python3 tools/progress.py reconcile TASK-ID --pr-json FILE` imports one saved
`gh pr view --json url,state,headRefOid,mergedAt,mergeCommit` object. It accepts only
OPEN or MERGED with the above fields, rejects malformed/mismatched evidence URLs,
null merge data for MERGED, non-null merge data for OPEN, and merged→open regression
or a changed immutable merged record. A repeated valid snapshot is allowed.
mergeCommit is an object with lowercase40-hex oid for MERGED, null for OPEN;
mergedAt is timezone-aware ISO for MERGED, null for OPEN. Retain its exact string.
observed_at is UTC import time from the tool's clock, explicitly not the snapshot's
save time or a new live GitHub observation. A repeated snapshot refreshes this import
time. Existing publication.pr cannot change; merged pr/head/merge_commit/merged_at
are immutable (equivalent merged_at instants count as equal). Replacement PRs need
an explicitly reviewed metadata correction, not automatic reassociation.
It records publication and observed_at, sets top-level updated_at to import time,
preserves activity, and validates the candidate before writing
state/HTML, and changes no status, notes, verification, reviews, authorization or
checkpoint. Errors leave both files unchanged. It performs no network, git or merge
operation; saved JSON is an operator-supplied observation, never live verification.
Serialization/rendering must succeed before either output file is replaced.
Write both complete outputs through temporary sibling files and os.replace; remove
temporary files on failure. Validation/serialization/render failures change neither
output. OS failures between the two replacements cannot promise pairwise atomicity;
report failure and reconcile on resumption. Do not claim a database transaction.

The coordinator separately verifies current GitHub head/checks/protections and
reconciles checkpoint prose at handoff. Task-state publication for the same PR is
normally finalized in the next authorized metadata change, not an endless sequence
of self-referential completion PRs. Historical `not_run` values are not rewritten to
passed merely because a later PR merged. Reconcile actual PR118/119 metadata here.

## Derived context and review measurement

Existing `context TASK-ID` output keys/behavior stay unchanged. Add optional
`--stage NAME`. It requires that name in required_stages and adds `stage_context` with
`name`, `packet` (the complete stage record), `complete`, `next_gate`, and `metrics`.
For an unstarted required stage, packet is null, complete false, next_gate authoring
and metrics are empty/zero as below. Without workflow, or for an unknown stage, fail
with exit1, stderr diagnostic, no stdout and no writes. For a started stage:
next gate: empty history → initial; after initial or unapproved verification →
verification; approved verification at current packet revision → closing; approved
closing at current packet revision → complete; otherwise → verification.

Metrics: `review_count`; `blocking_findings` (distinct IDs across the stage);
`closing_only_findings` (IDs first appearing at a closing review); both finding
fields are lists in first-appearance order. `elapsed_seconds`
(nonnegative numeric seconds summing observed paired timestamps, null if none);
`timed_reviews` (count). Empty metrics have review_count/timed_reviews0, both lists
empty and elapsed_seconds null.
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
all normative boundaries/remaining feasibility risks and links to the existing
evidence summary/history. Historical receipts remain accessible through git, the
published archive and evidence summary; retain any unique normative statement or
move it into the appropriate permanent section. Update coordinator guidance, progress
docs and PR template for coherent per-stage packets, reconciliation and measured
review outcomes. Do not duplicate stage state in a new manually maintained file.
Publish independent pre-PR review reports as PR comments to supply HTTPS evidence;
retain durable T3 links and actual identities, never substitute a fabricated reviewer.
