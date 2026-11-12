# Bounded coordination — v2 proposal

Specification v0.45 §§17–18; owner-authorized workflow improvements, 12 November
2026. This is a forward developer-workflow contract, not product capability.
WORKFLOW-004 reviews this contract under the existing full substantive cycle.
WORKFLOW-006 activates its new record/review policy only after reviewed implementation
and protected merge. Earlier tasks, receipts and explicit task gates remain historical;
do not reinterpret APITEST-016's supplemental evidence or weaken baseline assertions.
On WORKFLOW-004 merge, its accompanying AGENT_WORKFLOW guidance activates only the
capability/branch/report practices in §5 and private-evidence rules in §4. WORKFLOW-005
adds guidance for its implemented commands. WORKFLOW-006 owns record/review activation;
WORKFLOW-007 and CI-002 document their implemented execution and CI interfaces.

## 1. Review material and execution results

New or materially changed contracts, code, test expectations, execution machinery,
build/dependency/license/CI trust boundaries and material evidence claims retain
Claude initial → Codex corrections/separate verification → fresh Claude closing.
Authors cannot approve their own material. Corrections retain all prior reports,
rejected closings and unchanged valid approvals. A report count never waives a gate.

A newly claimed task may explicitly declare a **results-only** gate if ALL apply:

- The producer source, execution runner, configuration, independent expected-result
  rules and required binary/loaded-identity admission have applicable current
  substantive approvals for the exact artifact identity.
- Execution uses the approved recipe, synthetic/disposable state and an approved
  environment, without live credentials or external chat/payment effects.
- No code, oracle, material recipe, required proof limit or requirement changes are
  introduced while preparing or reviewing the results.
- The task-specific contract permits this gate; an older explicit full cycle is not
  automatically replaced. Missing/uncertain eligibility uses the full cycle.

Such a gate closes with mechanical integrity/classification checks plus ONE independent
Claude results review. That reviewer checks the actual evidence, independently derived
expectations, attribution, custody and limitations, and may require substantive review.
The reviewer must not be an author of the result producer, runner, expectations or
material result interpretation. A changed helper is code even inside a results stage.
Eligibility is rechecked mechanically before results review and again at closure:
the current result manifest must bind the exact identities approved for source,
runner, expectations, configuration and required admission. The results reviewer
confirms this match. Missing or mismatched approvals refuse results-only closure
and return the changed material to the full applicable cycle; an unchanged-code
rerun still needs a fresh eligible results review, never reuse of a prior result.
Unresolved findings keep the gate open. A material correction returns to the applicable
full cycle; the resulting execution evidence then needs its own eligible results review.
Never infer review authenticity from a URL, hash or coordinator-authenticated boolean.

At claim time list anticipated substantive and results gates, capabilities required
from dependencies, owned paths and acceptance. Unstarted stages need no fabricated
authors or revisions. New substantial execution infrastructure is a prerequisite task.
Routine receipt/publication/status updates do not revise the approved source packet
or create another review stage. Changed requirements/material proof claims do.

Examples: unchanged approved runner + newly observed logs may use results-only review;
a new signal-normalization helper requires the full substantive cycle; an old task
requiring initial/verification/closing results reviews retains that requirement.

## 2. Artifact identity and one record authority

WORKFLOW-006 adds first-class optional admission/results records to the existing
task workflow, with declared required gates. Its detailed reviewed contract must
define the strict JSON shapes, migration, CLI/schema agreement and completion rules.
Use the existing workflow module as the one review-order validator. Consumers do
not implement a second Claude/Codex grammar. Reviewer/session authentication and
actual Git/remote-state checking remain external coordinator responsibilities.

Approval currency binds the relevant source revision/owned-file identity, binary,
runner, configuration and classification manifest. A stage and its approving receipts
name the same manifest digest; material artifact change makes that approval stale.
The canonical digest convention must be explicit and independently tested. Hashes
bind bytes, not provenance or trust. Changing status prose must not alter execution
identity; requirements and executable/expected-result content remain review material.
WORKFLOW-006 owns that single canonical manifest digest function in tools/workflow.py
and extends WORKFLOW-005's publication check to use it. WORKFLOW-007 and subsequent
consumers import the same function; they do not invent a second digest convention.

The completion predicate includes every declared required source/admission/result gate.
A pending required identity or baseline-result gate cannot disappear behind an approved
implementation stage. Standard schema and semantic validator agree on supported keys;
unknown gate/receipt fields fail with actionable diagnostics. Full reports/raw logs
stay in PR/private evidence, with short structured references instead of duplicate
complete workflows. Preserve historical supplemental records without claiming they
are retroactively authenticated or fully enforced by new tooling.

Required examples: same source SHA + different operator digest is stale; rejected
closing → corrected verification → approved closing is valid history; pending required
results prevents done; adding a factual receipt alone preserves packet identity.

## 3. Reusable execution support

WORKFLOW-007 owns one small maintained stdlib-only Linux execution runner, configured
by bounded task data rather than hardcoded workstation paths or task-specific Python.
First review its detailed command/schema/process contract; reuse existing passive
ELF/seal primitives where their selected trust model applies. Do not build a generic
loader emulator or move product transport assertions into the runner.

Own exact argv/environment, relevant artifact checks, finite wall-clock/output limits,
exclusive private scratch, signal-safe child supervision, actual wait status, cleanup
and structured observations. Unknown ownership/status is unknown, never success.
Expected red is a named independently reviewed classification, not any nonzero exit.
Separate the execution manifest from the review record: the runner checks the approved
manifest binding; workflow validation owns review order and the coordinator owns authority.
The runner imports WORKFLOW-006's canonical digest function for that binding.

Review the runner once and re-review material changes. Each new consumer still needs
its own source/resource/dependency/license/passive/loaded-identity gates where required;
runner reuse does not admit an unknown binary or widen a mapped-library closure.
Keep the immutable native profile, all44 assertions and historical task operators intact.
Initial adoption may use harmless reviewed synthetic child fixtures; it does not rerun
historical one-attempt baselines or establish a new product/consumer admission.

A future task may predeclare at most one retry of unchanged disposable synthetic work
after an attributable infrastructure failure with known reaped children and clean state.
Preserve every attempt; product failures remain failures. Artifact/oracle changes require
their applicable review. Unknown ownership, unexpected effects, admission mismatch or
an explicit single-attempt restriction forbids automatic retry. No live effects qualify.

## 4. Compact handoff and publication

WORKFLOW-005 adds `context TASK-ID --compact`, optionally with `--stage NAME`.
Default output remains compatible. Compact output contains identity/brief/specification,
owner/status/scope, dependency IDs/status/publication, selected stage identity, unresolved
findings, retained approval references, next gate and limitations. Exclude raw task
supplements, unrelated stage histories, activity and duplicate packets. Histories remain
accessible in the existing full mode and linked reports. Never silently truncate findings.
If current records do not identify resolved findings, label recorded blockers historical
instead of inventing an unresolved/resolved state. Compactness is selection, not a byte cap
that hides actionable material. Preserve deterministic output and read-only behavior.

Add one documented publication check using existing project/progress/render checks and
the actual progress schema validation in its existing admitted isolated environment.
Do not add schema dependencies to ordinary tooling or silently install packages. Require
an explicit outgoing commit range and inspect every newly reachable commit for forbidden
private evidence paths, including deleted files. WORKFLOW-005 checks ONLY the existing
task.workflow.stages entries' recorded revision/paths against the candidate tree;
it introduces no new artifact-binding format or schema fields. Missing/invalid recorded
stage bindings refuse when that stage is selected. WORKFLOW-006 subsequently adds
manifest currency checks using its own record schema and single digest function.
The detailed WORKFLOW-005 contract defines the CLI and independent examples before tests.

Raw evidence belongs in ignored persistent private storage from creation. Public receipts
use explicitly permitted fields and exclude raw local paths, environment and process/maps
metadata. A path/history guard is not a complete secret scanner: disclose that limitation,
review public payloads and never claim absence of every possible secret from path checks.
Do not commit private artifacts and delete them later as a publication strategy.

## 5. Coordinator practice and CI

Before implementation, inspect required dependency capabilities and their actual
contract/source/observed support. Missing behavior in an NI baseline is not proof that
downstream integration is feasible. Resolve uncertainty through bounded source inspection
or an explicit feasibility prerequisite, not a new universal planning-review stage.

Park blocked draft branches until prerequisites are ready; suspend unnecessary watches,
then integrate current main once before resuming. Preserve urgent security/correctness
maintenance within authorized scope. Keep shared state coordinator-owned and regenerate
HTML. Do not split the entire task database, add caching or selective CI in this program.
Checkpoints contain current authorization, actual checkout, live assignments, blockers
and next action with links. Reports lead with verdict/new findings and link appendices;
progress messages state changed facts, current blocker and next action in plain language.

CI-002 depends on CI-001 and may proceed independently of WORKFLOW-004 publication
when setup failures block that publication. Its own detailed contract/tests/code
still need existing full substantive review; it grants no early record/review-policy
activation. CI-002 improves rejected APT-layout diagnostics before refusal, retaining Ubuntu-hosted
guards, signing trust, exact pins and unknown-layout refusal. Log bounded field/stanza
reasons and runner image identity without credentials. Keep schema/tool/behavior failures
distinct from package/DNS/infrastructure failures; allow at most one automatic retry for
an explicitly allowlisted transient acquisition signature, retaining its failed attempt.
Unclassified or mixed failures, unknown layout, integrity/signing and acceptance
failures never qualify. No continue-on-error or hidden retry.

Configure required status checks to match the existing all-checks merge policy only after
fresh authenticated inspection of actual rules/check names and successful current-head CI.
Derive the reviewed expected list from every currently reporting PR workflow job/matrix
declaration and its actual head checks, including the existing security check required
by the all-checks policy. Bind Actions checks to GitHub Actions' observed app ID and
external security checks to their actual expected app ID. Do not trust arbitrary new
context names from unrelated apps. Added/renamed workflows require an explicit reviewed
required-check inventory/settings update; removed names cannot silently remain required.
Preserve PR requirements, discussion resolution, bypass restrictions and unrelated rules.
Record settings before/after and verify enforcement; do not manufacture human GitHub
approval from AI reports. If administration is unavailable, checkpoint that concrete blocker
and complete the independently authorized repository work. Normal protected merges only.
