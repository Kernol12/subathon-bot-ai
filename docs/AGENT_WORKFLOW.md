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
using explicitly selected Opus review the test stage against the specification and
the finished implementation, following the review cycle below.
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
for each worker. Explicitly select Claude Opus and record the actual provider/model
with the reviewer identity; do not substitute Sonnet. If availability or quota blocks
a required Opus or Codex review, checkpoint the pending gate and continue only independent
authorized work. Do not silently switch provider/model, buy usage or use paid API
fallback. Obtain explicit authorization for a change of those choices; Codex readiness
does not satisfy the pending Opus gate.

## Substantive review cycle

Apply this cycle separately to every substantive review stage, including contracts,
test/oracle review, implementation and substantive coordinator-authored changes:

1. An independent Claude Opus reviewer assesses the stage against its requirements
   and records findings on the actual revision.
2. A Codex author fixes the findings. A separate independent Codex reviewer checks
   the resolutions, related edge cases and affected requirements. Repeat Codex fixes
   and independent Codex review until no blocking findings remain. If the initial
   Opus review finds no blockers, still obtain independent Codex verification before
   the closing Opus review; do not invent changes merely to exercise the cycle.
3. An independent Claude Opus reviewer performs the closing review of the resulting
   revision. Codex readiness is a handoff to this gate, never its replacement. If Opus
   finds blockers, return to the Codex correction/review loop, then request Opus
   closing review again. The stage closes only with the required Opus approval.

Material changes or new blocking findings after Opus closing approval return to the
Codex correction/review loop and then a new Opus closing review on the resulting
revision. A fresh initial Opus review is unnecessary for the same affected stage.

Keep authors and reviewers independent: a reviewer who authors a correction cannot
approve that correction. Batch related findings within a stage and retain valid prior
approvals, while reviewing material changes and their affected context. No fixed
review-count limit waives unresolved findings or a required approval.

Complete the test-stage cycle before observing intended red; implementation then
receives its own review cycle. Preserve every task-specific contract, source/resource,
dependency/license, build/CI, evidence and protected-merge gate. The cycle does not
combine stages or replace their checks. Routine non-substantive metadata may use
independent Codex review unless an explicit gate requires Opus. Changes to requirements
or material evidence claims are substantive even when they appear in metadata. When
classification is uncertain, treat the change as substantive; any reviewer may escalate
it to the Opus gate.

## Scope and sessions

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
reviews and remaining gates.

Merge only when latest-head required CI and relevant local checks pass, applicable
independent reviews approve, discussions are resolved and GitHub permits the protected
merge. Recheck after new commits. Never bypass branch protection, hide failures, or
weaken baseline assertions. If access or a gate is unavailable, leave the PR open and
record the concrete blocker. Instructions alone do not configure branch protection.

## CI and concurrent work

Assess CI coverage for each change and add workflows as executable modules, suites,
builds or packaging land. Keep actions/dependencies pinned and permissions limited;
run stable reviewed suites on relevant PRs. Keep intentional red baselines and live
credential checks out of passing product gates. Record actual workflow results and gaps;
new workflows do not automatically become required checks.

Keep commits focused and do not overwrite another writer's work. Batch related reviews
within a test-first stage, not across test and implementation gates. Re-review changed
behavior, tests, contracts, build/CI or material evidence claims; unchanged approvals
remain valid. Correct genuine test defects only with specification-based reasoning and
a separate reviewer.

## Done and evidence

Test-authoring done means reviewed tests and intended red, not implemented behavior.
Implementation done requires relevant passing acceptance/regression checks and separate
code review. Distinguish simulation from live service, hardware, browser and package
evidence; a green scaffold is not product proof.

For a completed task, put its PR URL in `evidence`, keep all of the task's `notes`
together to 500 characters or fewer, and record separate review in the state `review`
field and PR. The note states outcome, proof level (simulated, real SQLite, CI or live),
open gates and next step. Update the matching area entry in `docs/evidence/README.md`;
add no files under `docs/evidence/` and no per-task history to `docs/TEST_STRATEGY.md`.
Put commands, results and details in the PR. Historical receipts will be available in git
history under tag `evidence-archive-2026-11` once EVIDENCE-001 lands; the summary is the
readable record. Update state and regenerate `PROGRESS.html`. The coordinator checkpoint
is `progress/coordinator-checkpoint.md`. Do not rerun completed checks just to improve
receipts, and do not put credentials or personal data in evidence.

Default to fake connectors and disposable databases; automated tests must not trigger
real chat/payment side effects. Keep live checks separate. Resolve routine issues within
scope; record unresolved product choices and provider capabilities explicitly.
