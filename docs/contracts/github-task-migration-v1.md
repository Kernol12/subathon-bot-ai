# GitHub task migration v1

Authored contract for EPIC-029 / MIGRATE-001, specification v0.45 §§17–18.
Author: `codex:migrate001-author-20261113:/root` (Codex gpt-6.1-sol).
Independent review pending. This document grants no approval or cutover.
Issue-derived [briefs](../tasks/EPIC-029.md) preserve every issue acceptance criterion.
The [consumer inventory](github-task-migration-inventory-v1.md) is part of this packet.

## 1. Authority and identity

| Information | Before MIGRATE-004 merge | After MIGRATE-004 merge |
| --- | --- | --- |
| Task ID, title, owner, status, priority, parent, prerequisites, activity | progress/state.json; issues are preserved planning mirrors | GitHub Issues/Projects; native issue containment and prerequisite links, not copied lists in Git |
| Scope, acceptance, technical constraints | Versioned briefs, specification/contracts | Same; mutable issue text cannot replace versioned acceptance |
| Current required gates, authors, revisions, review order/results | Structured workflow in state | Small versioned proof records below |
| Actual PR head, checks, merge/time | GitHub PR/check records, imported observations distinguishable | GitHub PR/check records; optional source-linked observations never override actual facts |
| Owner authorization, stops, session/path assignments | Coordinator checkpoint under latest owner instruction | Same; readiness/board movement never grants permission |
| Historical task receipts, names and limitations | Existing history and evidence archive | Immutable remotely available cutover archive plus exact converted proof/history |

GitHub issue identity is repository `Kernol12/subathon-bot-ai` plus positive issue
number and canonical HTTPS issue URL. Stable IDs retain the current uppercase grammar
`^[A-Z][A-Z0-9]*-[0-9]{3}$`; renaming titles never changes identity. One task maps to
one issue; duplicate IDs, duplicate issue identities or repository/URL disagreement
fail reconciliation. Never recycle IDs. The conversion report maps original titles,
renames and aliases to these identities. Native parent/child containment is distinct
from directed prerequisite edges. Epic closure requires its children and final audit;
a child does not depend on the epic that contains it. Board-only planning items must
be explicitly included and classified, not dropped by a tracker mirror overwrite.

Git keeps identity references only where needed for brief/proof retrieval, not a second
backlog. No permanent synchronization, generated legacy ledger facade, claim/lease
service or hosted authorization service. Offline CI never reads Projects or needs a
GitHub token. Coordinator dispatch remains explicit. All v0.45 release requirements,
seven integrations, three rate providers and unverified platform/live gates survive.

## 2. Small proof format, discovery and eligibility

MIGRATE-002 implements version 1 JSON records at `progress/proofs/TASK-ID.json` and
one small `progress/proof-policy.json`. These are proposed successor paths, not active
inputs yet. A record has only `version`, `task_id`, `issue`, `kind`, `brief`, `workflow`,
`evidence`, `limitations`, and optional `publication`. `issue` contains `repository`
and `number`; URL is derived. `kind` freezes gate eligibility, not board work status.
`brief` is the active versioned acceptance path or pinned archive URL. `evidence` is
existing relevant proof references; `limitations` retains existing evidence caveats.
No title/status/owner/priority/epic/dependency/activity projection belongs here.
No new artifact-manifest/digest or results-only grammar is introduced by migration.

Policy fields: `version: 1`, `legacy_done`, `legacy_done_sha256` with the exact frozen
list/fingerprint from workflow records v1; `required_records` is a sorted unique list
of stable IDs needing proof, not a task ledger. A claimed non-epic must be enrolled
before authoring. Every non-epic with an existing workflow or publication record at
cutover is enrolled, including planned tasks with supplied evidence. Every forward
claimed task outside the frozen exemption list is enrolled. Exempt tasks with supplied
records retain and validate those records; exemption is never a reason to discard proof.
Unstarted planning issues need no fake authors/revisions. Epics need no artificial
implementation stage; final epic closure has the separately reviewed audit receipt.

Loader deterministically enumerates immediate regular `*.json` children sorted by
filename. Reject symlinks, nested/escaped paths, malformed/unknown fields or versions,
filename/ID mismatch, duplicate JSON keys/identities and noncanonical filenames.
Validate every discovered record, even if not selected by publication. Required IDs
must each have exactly one record. An absent policy, missing required directory/record,
invalid list fingerprint or unavailable validator is failure, never an empty success.
Extra well-formed records must be explicitly enrolled or rejected, not silently ignored.
Policy exclusions are solely the frozen list and unstarted/unclaimed planning work;
"old", "done on GitHub", missing receipt or pre-cutover date creates no exemption.

Offline CI cannot detect an arbitrary unrecorded board claim. The coordinator must
compare board claims/closure against enrollment before dispatch/completion; MIGRATE-004
and MIGRATE-005 audit the complete universe. Changes to eligibility/policy, enrollment
removal or kind changes require reviewed justification against archived source, not
an ad hoc default-to-legacy branch. CI validates source consistency; humans verify
actual identity, completeness and authority. This limitation must remain explicit.

## 3. Review grammar and current source binding

`tools/workflow.py` remains the sole semantic review-order engine. MIGRATE-002 adapts
input loading, reusing workflow records v1 in [WORKFLOW-003](../tasks/WORKFLOW-003.md)
and [AGENT_WORKFLOW](../AGENT_WORKFLOW.md). Preserve required stages and kind minima:
contract → contract; test drafts/reviews/baselines → tests; implementation → tests plus
implementation; other executable kinds → implementation. Explicit extra stages/gates
remain required. Stages may be absent until authoring, but pending stages block closure.

Preserve actual authors, review providers/models/session IDs, ordered receipts, report
URLs, findings/blockers, observed timing, revision/path bindings and rejected/superseded
history. Conversion must not invent independence or approvals. Initial Claude, Codex
fixes with independent Codex verification until blockers resolved, then independent
Claude closing remains required for each substantive stage. A clean initial still
needs Codex verification then closing. Reopened/materially changed packets repeat
verification/closing; rejected closing needs a new approved verification before closing.
Authors cannot approve their own corrections. Final closing and preceding approved
verification must bind the current stage revision. Preserve earlier valid approvals
only where unchanged; preserve old rejections as history, not current approvals.

A stage retains `authors`, full lowercase 40-hex `revision`, unique project-relative
regular-file `paths`, and ordered `reviews` with existing fields. Resolve the actual
Git commit and compare each selected file's Git mode/blob at recorded revision and
candidate head, using literal paths; missing, nonregular, escaped or stale bindings
refuse publication. Unchanged reviewed files may pass across unrelated commits.
Receipt-only bookkeeping is excluded from its own packet, including the new proof
record/policy metadata when used solely to record that packet. Schema/loader/policy
behavior edits themselves require separately bound substantive source review; exclusion
cannot hide requirement changes. No self-referential digest or synthetic future commit.

Publication continues to check actual checkout/root/HEAD, base ancestry, clean state,
merge state and every outgoing tree (including side history and subsequently deleted
private files), selected packet bindings, repository checks and actual admitted schema
validation with an exact success witness. Missing interpreter/dependency, empty-success
interpreter or unavailable required input refuses; it is setup failure, not intended
red or acceptance. Preserve private-path exclusions and stated limits: no identity,
review-quality, secret-completeness, live CI, board or authorization authentication.

## 4. Independently specified cases for 002/003

These expected outcomes derive from §§1–3, not from converter output. Reuse focused
existing workflow/publication/schema cases; add only newly needed loader behavior.

| Input/operation | Required outcome |
| --- | --- |
| One enrolled canonical synthetic contract record, valid current ordered approvals | Load; eligible closure only after actual final PR merge |
| Planned issue, no claim/evidence/enrollment | No invented record required; not execution permission |
| Enrolled ID missing; missing policy; corrupt JSON; duplicate key/ID/issue; wrong filename/version/unknown field; symlink or escaped path | Fail clearly; no implicit exemption/partial success |
| Frozen exemption member, no forward proof | Preserve historical limitation; do not fabricate review |
| Nonmember missing proof, including historical done | Fail or unresolved reconciliation blocker; never add exemption automatically |
| Supplied record for exempt task malformed | Fail; exemption does not suppress validation |
| Author used as reviewer; invalid provider/order; pending required stage; rejected closing without later verification/closing | Cannot close; preserve rejected history |
| Earlier initial then current approved verification/closing; unchanged selected blobs | Accept grammar and source binding; no authentication claim |
| Closing approves old revision; current selected blob/mode differs or is missing | Refuse current closure/publication |
| Only receipt metadata/unrelated source changes; selected files unchanged | Existing binding remains valid; actual checkout checks still apply |
| Unavailable or empty-success schema interpreter; schema-invalid record despite semantic pass | Refuse publication; no manufactured success |
| Private path added then deleted in outgoing history, including merge-side history | Refuse; clean final tree is insufficient |
| Board moved to Done or intermediate contract/test PR merged | No task lifecycle closure or new authority |
| Both prepared and existing path tested before cutover | Explicit temporary mode selects synthetic/extracted disposable records; production still reads state only |
| Fresh checkout after cutover, no state/HTML and no Projects access | Ordinary proof/schema/repository/scaffold checks pass with reviewed inputs |

New behavior uses contract → authored tests → independent full test review → observed
intended assertion red → fresh implementer → independent code review. Missing modules,
compilation/dependency failures or skipped bodies are not red. Docs/inventory/audit need
no manufactured red. No hundreds of per-task fixtures or deleted-button/file assertions.

## 5. Sequence, extraction and single cutover

The coordinator owns shared state/HTML/checkpoint, identity/remote reconciliation and
integration. Each executable task has its own branch, fresh author session and focused
protected PR(s). Successor readiness alone never expands a worker assignment.

1. **001:** register all seven records/briefs, claim only 001, author this contract and
   inventory; full independent contract cycle before completion/merge.
2. **002:** reviewed loader/schema/converter contract/tests, intended red and fresh
   implementation; keep real proof in state. Exercise bounded synthetic/disposable
   extraction; never commit a second editable production proof copy.
3. **003:** prepare publication/project/schema/CMake/CI consumers with an explicit
   opt-in nonproduction proof input. Both paths verified; current production default
   remains unchanged. Compatibility has a named removal in 004, not indefinite drift.
4. **004:** freeze dispatch and remote task edits for the cutover observation; capture
   latest complete state, GitHub issue/Project/PR snapshots and exact source revisions.
   Reconcile every task, relationships, names, planning-only issue and proof, including
   these seven tasks. Publish and verify the immutable archive remotely **before**
   deleting/relying on any old path. Extract production proof once, validate/report,
   change defaults/instructions and retire old ledger/dashboard in the same merge.
   Remove compatibility. Recheck remote drift immediately before merge; any unexplained
   drift blocks. Only that protected merge changes authority.
5. **006:** classify/compact/archive/remove briefs under §7; no early deletions in 004.
6. **005:** independent readback/audit of actual merged cutover and cleanup, fresh
   offline checkout checks, live board configuration/closure inspection and reviewed
   corrections until no unexplained drift. Only then final lifecycle closure.

The one-time reconciliation receipt is a versioned bounded report under
`docs/migrations/EPIC-029/`, not a permanent tracker or fixture suite. Its archive-backed
appendix maps each source task/issue identity, title/history, parent and prerequisites,
brief destination, proof destination/eligibility, evidence limitations and disposition.
Record source/remote snapshot pins, observation times, converter revision, canonical
input/output digests, all discrepancies/resolutions and field-preservation comparisons
(including original order/activity/history and rejected reviews in the archive).
Compute totals from the frozen inputs; reconcile set equality and edge equality,
not expected hardcoded counts. Explicitly explain approved board-only additions and
all exclusions. Keep raw/private receipts in ignored `local-evidence/EPIC-029`; public
report contains only reviewed synthetic/redacted/allowlisted data. No personal paths,
credentials or supporter data. Hash matching alone does not establish semantic fidelity.

Use a dedicated archive tag plus full immutable commit URLs containing final state,
schema, HTML, tool versions, briefs and history. Record exact tag object/commit and
verify remote fetch/readback and required paths; a local tag or moving branch is
insufficient. Existing `evidence-archive-2026-11` remains intact. Preserve the frozen
120-ID fingerprint `04afa2e0c32d4d88183e8b104f9d65402c81bd4e32521406c3d36a98e881e573`.
The original 496 task objects/order/history and restored newer PR126 facts are input
invariants, not a ceiling on newly registered tasks. PR126 reviewed head is
`afc46d64458889e346b3756599361da92dcb47a1`, merge
`4153a7ab2b3245e36fd5450b8897d2df659a53ee`, at `2026-11-13T13:27:08Z`;
owner/coordinator supplied verification found all 21 returned checks SUCCESS. Retain
actual restored completion metadata and old failed/not-run observations. Do not copy
stale main over it. Classic protection 404 does not negate active ruleset 26170263;
recheck actual PR/resolved-thread/21 app-bound checks at publication; no gate waived.

Rollback before cutover leaves state authoritative and discards disposable outputs.
After cutover, pause dispatch and freeze board edits, capture all intervening issue,
proof and PR changes, then prepare a reviewed protected rollback PR restoring archived
tracker consumers plus reconciled intervening facts. State becomes authoritative only
at that rollback merge; until then GitHub remains authoritative. Never simply restore
old state and lose post-cutover work, run dual writable authorities, delete remote
history, rewrite merges or bypass required checks. Report snapshot drift/failed remote
availability as blockers. MIGRATE-005 verifies archive reachability and this procedure.

## 6. Assertion cleanup boundary (ADR-0012)

No assertion removal occurs in 001–003. 004 may retire only ledger-specific validation,
serialization/render/controller/UI fixtures and checks whose entire contract is the
retired JSON task model/HTML snapshot: affected portions of test_progress.py,
test_compact_context.py, check_progress_ui.mjs, progress schema fixtures and render-only
project/scaffold expectations. Before removal, a separate approved cleanup review must
name exact methods/assertions at pinned revisions and map each to retirement or a
retained proof/schema/relationship check. This category list is not deletion approval.
If a method mixes old UI with meaningful workflow/publication/repository assertions,
preserve or adapt the meaningful assertions rather than deleting the method wholesale.

Retain reviewer independence/order/rejected history, required proof/kind/eligibility,
current source binding, checkout/outgoing-history/privacy, actual schema/interpreter
failure/witness, acceptance-map completeness, specification IDs, evidence/link checks,
presets and all product/security/accounting/native baselines. Explicitly replace old
progress-schema coverage with real proof/policy schema valid/invalid examples; retain
OpenAPI/component/examples coverage and selected dependency admission. Any unrelated
assertion removal needs another scoped task and separate approved ADR-0012 review.
Record original/revised assertion baseline and archived receipt in 004 PR/report;
no skip/WILL_FAIL/blanket continue-on-error or weakened expectations. 006 prose cleanup
has no test deletion scope. 005 uses existing focused cases and a one-time audit.

## 7. Mandatory task-brief cleanup

006 enumerates every task Markdown file and inbound consumer, including shared paths,
GitHub issue links, navigation, acceptance-map, evidence and review reports. Classify
active (unfinished scope), shared (any unfinished consumer/gate), completed historical
(all consumers closed) and navigation. Do not infer eligibility from filename/one
completed task. Maintain one-time per-path old → active replacement/pinned archive map,
all consuming task IDs and explicit retained exceptions. Remotely publish full prior
contents at immutable revision before removal; repair every current mutable link first.
Historical immutable reviews stay pinned, preserving their original context.

Compact active briefs to stable scope, versioned acceptance, specification/contracts,
technical dependency constraints, ownership boundaries and review/test gates. Remove
status/current assignment/planning-dependency/progress duplication now owned by GitHub.
Consolidate only genuinely same work with explicit per-ID acceptance coverage and
source-review currency assessment. No word target, silent requirement loss or invented
capability. Keep shared/completed briefs active while any unfinished acceptance/gate
needs them; report each exception's actual consumer. Never compact specification,
contracts or scoped instructions as task briefs. Material acceptance edits repeat
applicable substantive review. Remove eligible obsolete active files after link repair;
archiving alone is not completion. 005 audits every disposition/link and coverage.

## 8. WF006 rescope and lifecycle closure

In 004, update WORKFLOW-006 storage assumptions to proof/policy schema/loader and
publication consumers; `tools/workflow.py` still owns the future single canonical
artifact-manifest digest and results eligibility grammar. Replace progress integration
with proof integration, depending on MIGRATE-005 plus preserved WORKFLOW-005 capability.
Retain all intended admission/results gates, artifact currency, explicit pending results,
ONE-independent-Claude eligible results-only policy, historical compatibility and
WORKFLOW-007 → WORKFLOW-006 dependency. Migration implements none of that behavior or
policy activation. WORKFLOW-006/007 and product work remain stopped pending separate
authorization; this storage/dependency rescope alone is allowed.

Intermediate contract/test/preparation PRs use references, not auto-closing task issue
keywords. Each executable issue closes only after all versioned acceptance/gates and
its applicable final completion PR have actually merged; code Done/board Done is no
substitute. Contract-only 001 may close after its reviewed contract PR merges. Tests
for 002/003 do not close implementation tasks. 004's cutover merge does not close 006,
005 or the epic. 005 records independent observed audit and any final corrections;
final applicable audit PR merge and remote readback precede epic closure. A PR cannot
commit its future merge: record it operationally/next authorized change, with no endless
status-only completion PR. Current-head CI/protection and resolved discussion remain
mandatory. No reviews, checks or audit results are claimed by this authored packet.
