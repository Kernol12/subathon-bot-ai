# Agent workflow

> GitHub owns task planning since the final protected MIGRATE-004 cutover.
> Canonical proof records own current review/source evidence. Pre-cutover history is
> historical, as recorded, not re-verified, under the owner decision in the cutover contract.


## Task sequence and roles

For new behavior use: contract/examples → authored tests → independent test review →
observed intended red → implementation → independent code review → relevant checks.
Missing dependencies, compilation failures and skipped tests are not valid red evidence.
An independent reviewer is a different agent/session/person from the author; authors
never approve their own work.

The coordinator selects and decomposes tasks, assigns shared paths and integrates canonical proof/GitHub facts.
Authors produce bounded work and factual evidence; reviewers assess requirements, oracles,
implementation and gaps. GitHub owns status, assignment, native containment and prerequisites; use
`python3 tools/proofs.py context TASK-ID` for a read-only task view. Task briefs define
acceptance, and `docs/tasks/README.md` is navigation. Read the brief and scoped
`AGENTS.md` before editing. Production tasks require dependencies, specification refs,
contract revision, acceptance criteria and shared-file ownership.

## T3 roles and handoffs

Sol is the Codex coordinator: it selects tasks, assigns paths, integrates shared
files and owns shared proof integration, evidence, CI inspection and protected merges. A Codex worker
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
review evidence. Record task owner/status on GitHub; record temporary session/worktree
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
policy and developer schema. The canonical proof policy freezes the 120 tasks done at the
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
Use `python3 tools/proofs.py context TASK-ID --stage tests` (or contract /
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
this recovery path. Canonical proof/policy and checkpoint bookkeeping are excluded from
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
the task on GitHub and enroll its pending proof before executable authoring. Use separate branches/worktrees for concurrent writers; the integrator
coordinates canonical proof/policy, shared schemas, migrations, dependencies and root CMake.
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
or correctness maintenance still proceeds. Shared proof integration remains coordinator-owned;
avoid repeated branch refreshes solely for proof bookkeeping.

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
code review. Every required stage must have current closing approval and current
source mode/blob bindings. An open PR is not merged. Before closure the coordinator
reads back the actual final PR head/merge/time, latest required CI, protection and
resolved discussions, and records truthful issue/PR completion receipts (PR URL, head, merge_commit,
merged_at and observed_at). Missing
publication evidence blocks closure. `--complete` is a pre-publication gate: absent
or open publication is allowed there and proves no closure. Record merged observations
in the next authorized change; never create a completion-only proof PR loop.

State retirement loses these automated checks; preserve their procedural duties:
- Acceptance result values: coordinator verifies source/head-bound passing execution
  receipts and required suites without disguised setup failures or skips before closure.
- Per-task spec_refs: immutable arrays and successful pre-freeze admission are archived;
  coordinator reviews active brief specification/contract references at dispatch/closure.
  Repository spec ID uniqueness and complete acceptance-map checks remain automated.
- Implementation-after-observed-red ordering: coordinator verifies actual independently
  reviewed intended-red receipts before dispatch; test packet approval proves no run.
- Completed-note aggregate cap: coordinator checks concise completion notes (normally
  500 characters or fewer); proof limitations preserve original notes without truncation
  or an aggregate cap.
- State done-without-publication refusal: actual final merged PR/current gates/truthful
  issue/PR readback is the closure carrier; pre-publication completion cannot replace it.

Put detailed commands, results, proof level, open gates and next step in the PR.
Update the matching area in docs/evidence/README.md; add no per-task receipt files there.
The checkpoint remains progress/coordinator-checkpoint.md and records owner scope,
assignments and blockers, never a planning ledger. Preserve historical receipts and
all rejected reviews. Do not rerun completed checks merely to improve receipts.

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
current recorded stages and an admitted interpreter prepared using the
[isolated schema setup](schema-validation.md):

```sh
python3 tools/check_publication.py --base origin/main --head HEAD \
  --task TASK-ID --stage implementation --schema-python .venv/bin/python
```

Select only the stages whose recorded revision/paths should match the candidate;
repeat `--stage` for additional bindings. The checker verifies clean checkout and
commit identity, those bindings, every outgoing commit's forbidden paths, independent
repository checks and actual canonical proof/policy schema aggregate. It neither installs the SDK
nor authenticates reviews or grants publication/merge authority. Keep diagnostic
stderr private. Review untracked/ignored inputs and the public payload separately;
path checking is not complete secret scanning. Current-head CI and owner/repository
gates still apply.

Before handoff or resumption inspect actual checkout/branch, PR head/state, required
checks, reviews, rules and the latest owner stop. GitHub native prerequisites require
actual applicable merged PR readback; dependency readiness grants no authorization.
No reconcile/import command, permanent synchronizer, claim/lease or results service
is introduced. Optional publication observations remain strict source-linked facts,
never predictions. Keep historical not_run checks historical.

At dispatch read issue identity, assignment and native prerequisites, then the versioned
brief/spec/contracts and canonical proof context if enrolled. Unenrolled historical
kind/identity comes from the pinned full archive appendix. New IDs require exact
Task ID / Work type / Issue stanzas and their own complete reviewed brief-binding
packet before publication. Declare that packet at claim: contract for contract kinds,
tests for implementation/test kinds, otherwise implementation. Preserve kind minima
and explicit extra stages. Missing records never imply exemption. The frozen 120
historical exemptions do not authorize forward completion. Board Kind and Epic selects
are display-only; proof kind and native containment govern. Manual Done movement is
neither proof nor permission. MIGRATE-005 audits preservation and closure receipts;
WORKFLOW-006/007 remain stopped and their future policy is not activated here.

Archive pin, full brief/source inventory admission, protected remote readback and
historical availability are coordinator gates. Missing history blocks cutover. Rollback
requires paused dispatch, capture of intervening facts and a protected reviewed restore
PR; GitHub stays authority until that rollback merge. No automatic product resume.

For the remainder of EPIC-029, the latest owner-directed cutover review scope and
two-fix-round limit in `contracts/github-authority-cutover-v1.md` take precedence.
Historical records do not authorize future execution on unfinished tasks.
