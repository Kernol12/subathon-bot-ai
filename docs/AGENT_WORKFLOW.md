# Agent workflow

## Task sequence and roles

For new behavior use: contract/examples → authored tests → independent test review →
observed intended red → implementation → independent code review → relevant checks.
Missing dependencies, compilation failures and skipped tests are not valid red evidence.
An independent reviewer is a different agent/session/person from the author; authors
never approve their own work.

The coordinator selects and decomposes tasks, assigns shared paths and integrates state.
Authors produce bounded work and factual evidence; reviewers assess requirements, oracles,
implementation and gaps. Status lives only in `progress/state.json`; use
`python3 tools/progress.py context TASK-ID` for a read-only task view. Task briefs define
acceptance, and `docs/tasks/README.md` is navigation. Read the brief and scoped
`AGENTS.md` before editing. Production tasks require dependencies, specification refs,
contract revision, acceptance criteria and shared-file ownership.

## T3 roles and handoffs

Sol is the Codex coordinator: it selects tasks, assigns paths, integrates shared
files and owns state, evidence, CI inspection and protected merges. A Codex worker
authors contract examples/tests and captures reviewed intended red. A fresh Codex
implementation session receives that approved baseline. Separate Claude sessions
review the test stage against the specification and the finished implementation,
following the review cycle below. The coordinator chooses reviewer models.
These are two default review stages, not a ceiling: retain every task-specific
contract, source/resource, dependency/license, build/CI and evidence gate. Substantive
coordinator-authored changes also require independent review. Session names alone
and reviews posted through the owner's GitHub account do not prove independence or
constitute a second human GitHub approval.

Use stable provider/session identities and durable T3 thread links in handoffs and
review evidence. Record task owner/status in state; record temporary session/worktree
assignments in the checkpoint. Never assume an old assignment is a live process.
Keep handoffs concise: identify task/brief, base/head SHA, actual checkout and owned
paths; link the relevant specification/contracts, acceptance criteria, required gates
and prior reviews. State the requested review and open findings/resolutions. For
rechecks, send the changed revision/paths, findings and resolutions rather than repeat
the full history. Keep the complete affected context available through durable links.
The reviewer verifies the revision and worktree before review, including material
uncommitted changes, and may inspect surrounding code and requirements beyond the
supplied diff. Retain earlier findings and unchanged valid approvals.

T3 creates isolated delegated worktrees for concurrent writers. Shell tools and
existing build presets remain available. CLion MCP may assist only when its project
path and reviewed revision match the intended checkout; otherwise use that checkout's
file/shell tools. Do not assume the main IDE project represents a worker worktree.
Keep review context focused on the affected stage and avoid rereading global history
for each worker. The coordinator selects available reviewer models for the stage's
complexity, risk and remaining quota, and records the actual provider/model with the
reviewer identity. Honor explicit owner constraints. Model selection within the
authorized providers is a routine coordinator choice, including changing models
between reviews; no particular model is mandatory. If no authorized model is
available for a required Claude or Codex review, checkpoint the pending gate and
continue only independent authorized work. Do not waive review, change providers,
buy usage or use paid API fallback without owner authorization. Codex readiness does
not satisfy a pending Claude gate.

## Substantive review cycle

Apply this cycle separately to every substantive review stage, including contracts,
test/oracle review, implementation and substantive coordinator-authored changes:

1. An independent Claude reviewer assesses the stage against its requirements
   and records findings on the actual revision.
2. A Codex author fixes the findings. A separate independent Codex reviewer checks
   the resolutions, related edge cases and affected requirements. Repeat Codex fixes
   and independent Codex review until no blocking findings remain. If the initial
   Claude review finds no blockers, still obtain independent Codex verification before
   the closing Claude review; do not invent changes merely to exercise the cycle.
3. An independent Claude reviewer performs the closing review of the resulting
   revision. Codex readiness is a handoff to this gate, never its replacement. If the
   reviewer finds blockers, return to the Codex correction/review loop, then request
   Claude closing review again. The stage closes only with the required approval.

Material changes or new blocking findings after Claude closing approval return to the
Codex correction/review loop and then a new Claude closing review on the resulting
revision. A fresh initial review is unnecessary for the same affected stage.

Keep authors and reviewers independent: a reviewer who authors a correction cannot
approve that correction. Batch related findings within a stage and retain valid prior
approvals, while reviewing material changes and their affected context. No fixed
review-count limit waives unresolved findings or a required approval.

Complete the test-stage cycle before observing intended red; implementation then
receives its own review cycle. Preserve every task-specific contract, source/resource,
dependency/license, build/CI, evidence and protected-merge gate. The cycle does not
combine stages or replace their checks. Routine non-substantive metadata may use
independent Codex review unless an explicit gate requires Claude. Changes to requirements
or material evidence claims are substantive even when they appear in metadata. When
classification is uncertain, treat the change as substantive; any reviewer may escalate
it to the Claude gate.

## Structured packets and bounded handoffs

The [workflow-records v1 contract](tasks/WORKFLOW-003.md) defines the forward review
policy and developer schema. `workflow_policy` freezes the 120 tasks done at the
migration base; their receipts and limitations remain historical. New non-epic
claims require structured `workflow` stages. Planned stages may be absent until
authoring starts. Never invent authors, revisions, reviews or results to fill fields.
The validator checks recorded relationships; it cannot authenticate sessions,
review quality, Git contents, remote checks or authorization.

Before requesting a review, assemble one coherent packet for that stage: brief and
specification/contract revision, authors, exact source revision and owned paths,
acceptance/oracles, resource/ownership constraints, affected build/dependency/license
changes, actual evidence and unresolved findings. Batch related concerns within the
stage; keep contract, tests-before-red and implementation approvals distinct.
Use `python3 tools/progress.py context TASK-ID --stage tests` (or contract /
implementation) for the derived packet, next gate and observed review metrics.
Read linked affected context, not global activity history at every handoff.
Append `--compact` for a smaller handoff with the selected packet, all review links,
historical blocker IDs and gate metrics. Full mode retains detailed findings and
closing-only metrics; compact output states those omissions explicitly. Neither
view authenticates reviews or authorizes stopped work.

Record each review's actual phase/provider/model/session/revision/verdict, HTTPS
report and finding IDs. Prefix new findings by review identity, and reuse an ID only
for the same issue. Add observed start/finish times when available; omit them rather
than estimate. Metrics report review counts, distinct blockers, closing-only findings
and observed review duration. Use them to evaluate future low-risk policy proposals;
no score or threshold reduces the current review gates automatically.

Reviewers stay read-only. If a reviewer authors a correction, preserve their old
reports and findings in PR/git history; the packet must declare all actual authors
and obtain a fresh independent cycle. Superseded reports must not be presented as
current approvals or deleted from history. Keeping corrections with the author avoids
this recovery path. Progress JSON, HTML and checkpoint bookkeeping are excluded from
stage paths; recording a receipt does not revise its reviewed packet. Material
requirements/evidence changes still need their applicable independent review.

## Scope and sessions

Before implementation, record each required dependency capability and its actual
contract/source/observed support. A baseline that fails at an NI seam does not prove
downstream compatibility. Resolve an unknown combination by bounded inspection or
an explicit feasibility prerequisite, without adding a universal planning-review stage.

Epics are planning containers. If a substantial architecture prerequisite emerges, split
it into dependency-linked tasks and focused PRs before implementing the expansion. Preserve
requirements and valid work; do not split every small fix or silently shrink scope.
Use a fresh implementation session for each executable task. Supply its ID, brief,
reviewed context, branch/worktree, owned paths and next action; keep the coordinator as
needed. Use durable links rather than relying on chat history, and never invent a reviewer.

## Branches and pull requests

Before edits, start from current `main`, create one branch per executable task and claim
the task in state. Use separate branches/worktrees for concurrent writers; the integrator
coordinates `state.json`, shared schemas, migrations, dependencies and root CMake.
Open a focused PR to `main` using `.github/pull_request_template.md`; use a draft while
required work or review remains. Include task IDs, specification scope, exact checks,
reviews and remaining gates. A draft still triggers CI: keep unreviewed test source
local until its review gate passes. Observe intended red locally before publishing
the implementation; do not disable existing CI or call premature runs reviewed proof.

Merge only when latest-head required CI and relevant local checks pass, applicable
independent reviews approve, discussions are resolved and GitHub permits the protected
merge. Recheck after new commits. Never bypass branch protection, hide failures, or
weaken baseline assertions. If access or a gate is unavailable, leave the PR open and
record the concrete blocker. Instructions alone do not configure branch protection.

## CI and concurrent work

Park a blocked draft branch until its prerequisites are ready, suspend unnecessary
watches, and integrate current main once before resuming. Urgent authorized security
or correctness maintenance still proceeds. Shared progress remains coordinator-owned;
avoid repeated branch refreshes solely to reconcile generated progress files.

Assess CI coverage for each change and add workflows as executable modules, suites,
builds or packaging land. Keep actions/dependencies pinned and permissions limited;
run stable reviewed suites on relevant PRs. Keep intentional red baselines and live
credential checks out of passing product gates. Record actual workflow results and gaps;
new workflows do not automatically become required checks. Superseded PR runs may
be cancelled; main pushes and manual runs use independent concurrency groups. Keep
authenticated SDK admission, pinned inputs and complete test inventories. Caching,
reusable setup or selective execution requires its own reviewed trust/coverage design.

Keep commits focused and do not overwrite another writer's work. Batch related reviews
within a test-first stage, not across test and implementation gates. Re-review changed
behavior, tests, contracts, build/CI or material evidence claims; unchanged approvals
remain valid. Correct genuine test defects only with specification-based reasoning and
a separate reviewer.

## Done and evidence

Test-authoring done means reviewed tests and intended red, not implemented behavior.
Implementation done requires relevant passing acceptance/regression checks and separate
code review. Under structured tracking, every required stage must have current closing
approval. Artifact `done` and publication are separate: an open PR is not merged, and
a tracked prerequisite satisfies dependencies only after its recorded publication is
merged. Legacy dependency semantics remain intact. Distinguish simulation from live
service, hardware, browser and package
evidence; a green scaffold is not product proof.

For a completed task, put its PR URL in `evidence`, keep all of the task's `notes`
together to 500 characters or fewer, and record separate review in the state `review`
field and PR. The note states outcome, proof level (simulated, real SQLite, CI or live),
open gates and next step. Update the matching area entry in `docs/evidence/README.md`;
add no files under `docs/evidence/` and no per-task history to `docs/TEST_STRATEGY.md`.
Put commands, results and details in the PR. Historical receipts are available in git
history under tag `evidence-archive-2026-11`; the summary is the
readable record. Update state and regenerate `PROGRESS.html`. The coordinator checkpoint
is `progress/coordinator-checkpoint.md`. Do not rerun completed checks just to improve
receipts, and do not put credentials or personal data in evidence.

Default to fake connectors and disposable databases; automated tests must not trigger
real chat/payment side effects. Keep live checks separate. Resolve routine issues within
scope; record unresolved product choices and provider capabilities explicitly.

Create raw evidence in ignored persistent private storage. Public receipts include
only explicitly permitted fields, excluding raw local paths/environment/process maps.
Inspect outgoing commits, not only the final tree: deletion does not remove private
bytes from earlier commits. Path checks are not a complete secret scanner. Review
public payloads and keep complete private evidence accessible to authorized reviewers.

Lead review reports with the verdict and new findings; link detailed appendices rather
than repeat unchanged history. Progress updates state changed facts, the current blocker
and next action in plain language. Checkpoints contain current authorization, actual
checkout, live assignments, blockers and next action with links, not copied receipts.

## Publication reconciliation and resumption

Before publishing a candidate, run the local publication check with explicit refs,
current recorded stages and an already prepared admitted schema interpreter:

```sh
python3 tools/check_publication.py --base origin/main --head HEAD \
  --task TASK-ID --stage implementation --schema-python build/schema-venv/bin/python
```

Select only the stages whose recorded revision/paths should match the candidate;
repeat `--stage` for additional bindings. The checker verifies clean checkout and
commit identity, those bindings, every outgoing commit's forbidden paths, existing
repository checks and actual progress-schema validation. It neither installs the SDK
nor authenticates reviews or grants publication/merge authority. Keep diagnostic
stderr private. Review untracked/ignored inputs and the public payload separately;
path checking is not complete secret scanning. Current-head CI and owner/repository
gates still apply.

Before handoff or resumption, inspect actual branch/worktree, PR head/state, checks,
reviews and repository rules. Save the bounded GitHub observation, for example:

```sh
gh pr view PR-NUMBER --json url,state,headRefOid,mergedAt,mergeCommit > /tmp/task-pr.json
python3 tools/progress.py reconcile TASK-ID --pr-json /tmp/task-pr.json
```

The import records publication and its UTC import time, updates state/HTML and leaves
artifact status, notes, reviews, verification and activity unchanged. It authenticates
nothing and performs no network/git/merge operation. Check actual latest-head gates
separately. Review the resulting diff and reconcile checkpoint prose yourself; the
command cannot decide authorization or whether a stale instruction is safe to execute.
Reject a changed immutable merged record or automatic reassociation with another PR.
Validation/render/serialization failures preserve both outputs; OS failure between two
file replacements requires reconciliation, not a claim of pairwise atomicity.

The same PR cannot commit knowledge of its own future merge. Record open publication
as an observation while preparing it; finalize the observed merge in the next already
authorized metadata change. Do not create an endless series of completion PRs or start
a product task to update bookkeeping. Historical `not_run` checks remain historical;
a merge alone never rewrites them to passed. Keep the checkpoint short: current owner
scope, observed checkout, roles, pending gates, concrete blockers and next action, with
links to state/brief/PR. Preserve old handoffs in Git rather than copying them forward.
