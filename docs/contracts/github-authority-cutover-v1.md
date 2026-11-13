# GitHub authority cutover v1 — MIGRATE-004

Authored proposal, specification v0.45 §§17–18. Author:
`codex:migrate004-contract-author-20261113:/root` (Codex gpt-6.1-sol).
Inspection revision: `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`, branch
`task/MIGRATE-004-github-authority`, based on merged main
`b80e94f514caf33cad8abde8d00fa744b565768a`.
Correction author: `codex:coordinator-59095155`; initial review source `cdbfed0`.
This text supplies requirements and examples, not review, execution or cutover evidence.

Parents, read in full: [migration v1](github-task-migration-v1.md),
[proof records v1](proof-records-v1.md), [publication v1](proof-publication-v1.md)
(final003 contract source `7f0b876`, runtime correction `fcf205a`, now merged).
The [separate exact ADR-0012 proposal](github-tracker-retirement-scope-v1.md)
requires its own independent approval before any assertion removal.
The [task brief](../tasks/EPIC-029/MIGRATE-004.md) retains its complete acceptance.
Parent formats, exemptions, review grammar and release scope remain binding.

## Authority and finite sequence

Owner authorizes 001 → 002 → 003 → 004 → 006 → 005; only004 is active here.
State is the sole planning/proof authority until the final protected004 merge.
Product/API and WORKFLOW-006/007 execution remain stopped. Board readiness is not
permission. The coordinator alone owns state/HTML/checkpoint, remote reconciliation,
archive publication and integration. Authors cannot approve their own work.

| Boundary | Planning / current proof | Required operation |
| --- | --- | --- |
| Contract and reviewed preparation | State / structured state workflow | Author/review this contract and separately the exact cleanup scope; no real canonical proof copies |
| Protected archive-preparation PR merged | State / structured state workflow | Frozen source and full appendix become a verified remote ancestor;004 remains open/in progress |
| Final cutover PR before merge | State / structured state workflow | Candidate proof is a single extraction, not separately editable production authority; validate both preservation and final source gates |
| Actual final protected merge read back | GitHub / canonical proof records | Switch once; remove old production consumers and all compatibility paths together |
| 006 then005 | GitHub / canonical proof records | Actual brief cleanup, then independent fresh-checkout/board/history/proof audit; epic remains open until final audit closure |

1. Coordinator pauses dispatch, captures source and complete remote inventory, resolves
   discrepancies and reviews the mapping. No issue/task duplication. Review the contract
   and exact cleanup scope through separate full substantive cycles.
2. Author focused tests for changed executable behavior; independently review tests,
   observe intended assertion red, then use a fresh implementation session. Read-only
   inventory/archive audits and prose need no tests or manufactured red. Missing SDK,
   interpreter, object or build failure is setup failure. Cleanup source also receives
   the full independent code cycle. Each stage follows initial Claude → Codex fixes
   and separate Codex verification until clear → independent Claude closing.
3. Prepare a focused protected **intermediate archive PR**, retaining old authority and
   all brief paths. Merge normally after its reviews/current CI/resolved threads. Fetch
   and independently read back the actual merged commit and a durable remote tag
   `task-tracker-archive-YYYYMMDD-<shortsha>`; record tag object and peeled full commit.
   Verify archive contents at the merged commit, not just PR head. Only then choose
   policy.archive_commit. No future SHA, self-hash cycle or guessed merge facts.
4. Prepare final cutover against that actual ancestor of target main. Convert once,
   validate full report and preservation, update all consumers/instructions together,
   and remove the full appendix from HEAD. Reconcile any source/remote drift, obtain
   applicable reviews and current CI, rerun latest-head publication. Final merge alone
   changes authority; read back actual head/merge/time before issue closure.
5. Execute006 through fresh assigned sessions and review. Keep005 behind006; only005
   independently verifies actual outcomes and permits migration epic closure. These
   links and this proposal do not complete or start006/005.

Multiple004 PRs use existing contract/tests/implementation packets, not new stage or
proof fields. Intermediate PRs reference #629 without closing keywords. **Never put
an intermediate archive PR in004's publication field, open or merged.** Record its
actual head/merge/time and review receipts as append-only evidence/limitations links
and PR receipts. Reserve publication for the final cutover PR's actual observation,
or leave it absent. No reassociation or immutable merged-field rewrite is authorized.
Update packet revision/authors/paths honestly, preserve all ordered prior reviews and
obtain renewed verification/closing on material changes. Tests paths bind retained/new
oracles and fixtures only. Removed paths/methods and original NI modes/blobs stay pinned
at their reviewed revision in baseline receipts, never fake current packet paths.

| Deliverable | Existing packet / source-bound evidence |
| --- | --- |
| Cutover contract and exact retirement proposal | Contract packet binds these current documents and004 brief. Its approval does not grant separate cleanup approval. |
| Archive payload, root reasons, relationship decisions and privacy/reachability audit | Independent substantive cycle for this read-only documentary deliverable; reports identify payload source commit, exact paths/modes/blobs and authors. Store report links in contract evidence and intermediate PR receipts. After merge independently verify actual archive commit. Full appendix/old state are reference pins, never final current packet paths. No manufactured red. |
| Separate ADR-0012 cycle | Own initial Claude, Codex correction/separate verification, fresh Claude closing reports explicitly identify retirement document revision/blob and baseline methods/assertions. Store each actual report URL in task evidence/PR, with its phase/provider/model/session/source/findings in the report. Do not add another initial receipt to the primary contract stage or invent stages/fields. Tests and implementation closing reviewers must cite the matching separate approval and compare actual changed tests to it; source drift reopens the affected cleanup gate. |
| Changed oracles and current test-owned fixtures | Tests packet, reviewed before execution/red; current retained/new paths only. Removed paths are separately pinned by the approved cleanup map and reviewed source diff. |
| Runtime/schema/CMake, STATUS, all authority instructions and WORKFLOW-006/007 rescope | Implementation packet binds every retained/new changed consumer, including these documents. Review full source diff including approved deletions. No deleted path in final current packet paths. |
| Final archive-to-proof preservation and remote go/no-go | Pinned state transition below plus independently checked PR receipt and bounded HEAD summary; no second editable ledger. |

### Authoritative-state freeze and exact final extraction

Resolve and read back the ten relationship reclassifications and board projections
**before archive capture**; the archive-time appendix records those decisions. A later
final go/no-go recapture, write readback and drift resolution belong in the bounded
HEAD summary and final PR receipt, not a claim that the earlier appendix saw the future.

Archive source A is frozen immediately after its full report is generated from the
same source. Archive PR review receipts may subsequently enter authoritative state.
After A, allow only existing003 forward continuations in historical task proof fields:
ordered evidence/notes-to-limitations/review prefixes, required-stage prefixes, honest
packet source/authorship advances with fresh approvals, valid open publication advances,
and previously absent workflow/publication additions. Verification/review/supplemental
historical maps and original brief bytes remain frozen. Track ordinary task status/owner
only as the permitted004 preparation transitions in state, never in candidate proofs.
No unrelated dispatch. Any non-preservable source edit requires re-archive/readback
before the policy pin becomes immutable.

All mandatory contract/test/code and separate cleanup review-stage approvals must reach
state before its final freeze. The later preservation/readback check is factual read-only
verification, not another unrecorded stage approval; any substantive defect reopens the
affected stage before a new freeze. Keep state authoritative during those cycles; any proof candidate is
only a generated conversion preview, never independently edited task/proof authority.
Use pinned old tooling offline for any remaining state admission after current consumers
have been retired on the candidate branch; do not retain that compatibility in finalHEAD.
A required gate discovered after freeze returns to authoritative state, appends all
rejected/new receipts, and creates a new freeze; no receipt may be dropped.

After the last such receipt, create an actual commit **P** that still contains the final
pre-deletion state and full one-time transition report. The report names archive A,
input digests and exact ordered per-field append/advance delta, but does not predict P's
own SHA. Final bounded summary pins observed full P and immutable report/state URLs.
P must be an ancestor of the actual final PR head and become remotely reachable through
that protected merge; finalization deletes state/full report from HEAD, never history.
Put the observed full P pin and final factual recapture/readback in the existing
bookkeeping coordinator checkpoint; the bounded docs/migrations summary links that
authority checkpoint and immutable report. Its static contract/content is reviewed
before freeze. Do not bind final factual receipt data as a changed current source packet
or create a new bookkeeping allowlist: this prevents a self-referential P/review loop.
The final independent preservation receipt verifies the actual checkpoint pin against
Git ancestry and objects; a checkpoint assertion alone is not proof.
Independent preservation check requires semantic, ordered and type-strict equality:

`convert(P.state, identities, A) == convert(A.state, identities, A) + exact permitted delta(A,P)`

Candidate policy/records must equal that P conversion, including **every** post-archive
rejected/approved receipt, evidence and original limitation, before finalization. Missing,
rewritten, reordered or unexplained entries refuse cutover. Compare complete extracted
objects, not counts/hashes alone. This is one-time reconciliation, not a permanent sync.
After P, only explicitly enumerated receipt-only evidence/limitations and truthful final
PR observations may append to candidate proofs, with an exact field delta inspected in
the final PR receipt; no dual write to state, new claim, changed stage approval or new
execution is allowed. Planning remains frozen state until actual cutover merge. Any
substantive/new-gate change returns to state and a new P. This avoids both a review/report
fixed point and a silent early authority switch.

## One-time reconciliation inputs and deliverables

Starting inventory [consumer inventory](github-task-migration-inventory-v1.md) is
not a current completeness certificate. Refresh tracked-file enumeration and lexical
hits for state/schema/HTML/progress/workflow/publication/task links; follow imports,
CMake/CTest registrations, all CI lanes, acceptance-map references and shared briefs.
Inspect actual consumers, including new002/003 files. Save raw captures privately.
No permanent sync framework or full task mapping in the active tree.

Required inputs: exact frozen state bytes/revision and ordered history, full original
brief/tool/schema/HTML bytes and history objects, converter revision, all paginated
issues/board items/fields/project configuration, all parent/subIssues/blockedBy pages
including nested completeness, current title changes and actual prerequisite PR facts.
Capture timestamps, repository/project identity, pagination end cursors/counts and
SHA256 of each raw input; repeat remote capture before final go/no-go. Saved
`local-evidence/EPIC-029/migrate004-inventory/` inputs are the initial read-only snapshot,
not proof of latest remote state. Public output must be reviewed and allowlisted;
private paths, credentials and supporter data never enter it.

Reuse exact002 `identities.json` and converter `conversion-report.json` shapes at
`docs/migrations/EPIC-029/archive/`. The report has no archive_commit, so it can precede
the observed pin. Do not add a second converter/proof format. Full one-time
`reconciliation.json` at the same archive directory has this audit layout (not a
runtime schema or CI backlog input):

| Member | Required content |
| --- | --- |
| version, inputs | Audit version1; named source revisions, capture instants, digests, repository/project identity, pagination/completeness observations and converter revision |
| tasks | Original ordered ID rows: original title, current readable title, observed rename history/aliases with source, native repository/number/URL, kind, original state position, disposition of every source field, brief/proof/archive destinations and limitations |
| roots | Every ID without baseline parent, reason: peer epic, shared/standalone foundation or reviewed explicit exception; no unclassified root |
| relationships | Every source depends_on edge with directed endpoints, containment-or-blockedBy classification and reason; separate live edges, set differences, reviewed resolution and post-write readback |
| consumers | Each current path/import/registration/link and retain/adapt/pin/retire destination; every brief's consuming task IDs/inbound links, deferred006 ownership |
| preservation | Ordered original activity/tasks/notes/review/evidence/verification/brief comparisons, frozen120 fingerprint, exact historical objects/rejected packets, semantic comparisons and blob sizes/digests |
| discrepancies | Stable audit IDs, observed/expected values, evidence, proposed remedy, independent reviewed resolution and actual readback or unresolved blocker |
| decision | Computed set/count summaries, explicit exclusions/board-only additions, native-view/closure observations, go/no-go with remaining limitations |

Full mapping/appendix exists only in the remote archive after cutover. HEAD retains
bounded `docs/migrations/EPIC-029/` summary: pins, computed totals, discrepancy
resolutions and immutable report URLs, no per-task backlog or kind ledger. Original
state titles remain original-name history; never overwrite current readable issue
names with stale originals. Keep all496 original objects/order/history plus newer
registered tasks and actual completion facts; old counts are not ceilings.

Derive containment exactly as parent §9: an epic depends_on edge to a task whose
brief lies inside that epic's own directory becomes containment. Every other edge
becomes directed blocked-by, including dependencies on epics. Do not emit containment
again as blocking; classify all roots and fail on ambiguous parents, unknown endpoints,
cycles, duplicates, missing identity or disagreement between parent/subIssues readback.
An explicitly justified board-only edge/addition needs reviewed resolution, not loss
through mirror overwrite. Equal counts cannot replace set equality.

Static comparison at inspection revision against supplied complete503 snapshot found
422 baseline containment edges versus432 native, and1549 baseline prerequisite edges
versus1539 native. These are observations, never hardcoded acceptance constants.
The ten extra child→parent pairs and missing parent→blockedBy-child pairs are:

| Standalone task | Epic |
| --- | --- |
| ACCTCONTRACT-001 | EPIC-002 |
| CONTRACT-004 | EPIC-009 |
| CONTRACT-013 | EPIC-008 |
| CONTRACT-014 | EPIC-008 |
| IMPL-001 | EPIC-001 |
| SPIKE-001 | EPIC-008 |
| SPIKE-002 | EPIC-010 |
| SPIKE-004 | EPIC-020 |
| STORAGECONTRACT-001 | EPIC-009 |
| STORAGECONTRACT-002 | EPIC-009 |

Proposed remedy is coordinator-reviewed removal of these native containment links
and creation of the corresponding directed prerequisites, followed by full readback.
No writes occurred here. Keeping one as containment would require an explicit reviewed
parent-contract amendment; counts/completeness alone cannot justify that exception.
All81 baseline roots require individual reasons in the archive, including these ten;
all29 epics remain peers. No unresolved discrepancy may pass final cutover.

Native board go/no-go requires observed views and filters (complete backlog, current
work, epics), status options and issue-state/board-state transitions, closure automation
configuration and absence of intermediate auto-close. Reconcile every issue/item ID
and current status/assignee with authoritative source and authorized forward facts.
Manual Done/closed is not proof. Test closure configuration with disposable/synthetic
inputs or read-only configuration evidence; no product chat/contribution effects.
Fresh capture must show the agreed004→006→005 and WF006→005/WF007→WF006 blockers.
Unknown/uninspectable configuration is a stated blocking audit gap, not assumed support.

## Archive freeze and availability

Archive contains final full state, state schema, rendered HTML, old tools/consumers,
all prior briefs, original names/native identities, report/appendix and reachable
historical packet objects. Preserve existing evidence archive. Independently fetch
remote tag/commit in a fresh checkout and read each required path, including recorded
NI source modes/blobs. Archive must remain usable after old tracker and later006
brief removals; no HEAD state or local private file dependency.

Each archive state/identity/conversion-report blob consumed by publication must fit
its existing1MiB cap, independently measured on actual merged blobs. Raw current state
exceeds the cap; lossless compact JSON around920k is only a feasibility observation.
Compare parsed semantic identity, ordered arrays and exact string/history values;
retain original bytes separately in the archive if compact transport is used. Measure
actual growth through final archive preparation, identity/report caps and full remote
readback. Do not waive bounds, truncate history or compress away validation obligations.
If actual retained input cannot fit, stop for a separately reviewed bounded design
amendment before implementation/publication; no silent cap increase.

Freeze historical notes/verification/review/supplemental fields and brief bytes at
archive. Later evidence/limitations/reviews may only append under approved003 forward
rules; packets may advance revisions honestly, and publication observations obey the
existing immutable-merged/monotonic-open rule. New observations never upgrade historical
not_run maps. Preserve003's newer actual merge and observations. A required
non-preservable change demands replacement remote archive/readback before immutable
policy pin. Any integration change to source/report also requires revalidation or
re-archive. No stale-main overwrite.

### Historical object reachability and original bytes

Inventory every commit-valued state/proof/supplemental field, source revision in evidence
links, and recorded NI mode/blob reference; retain exact field paths and strings. Classify
object type, local availability, required stage/NI binding versus supplemental observation,
and reachability from the eventual archive tag. A mere successful local cat-file or
remote ref-name search is insufficient: traverse remote-reachable history/trees and
independently fetch/read back the objects. Include every row in archive reconciliation.
Missing required packet/NI objects block pinning; never synthesize an exemption.

Historical discovery and proposed disposition (requires independent source-aware
approval before pinning): supplemental APITEST-016 post-closing status observation
names `da0fe1cb2df6fb25195a9c63cf10d20167a6ca6b`. The outgoing range has17 commits
including it (16 other commits), not17 additional ancestors. The first coordinator
scan incorrectly omitted the retained docs/evidence restriction. The full predicate
finds commit `049d998d059c54132d26b3810d1f3cc1d889e201` with forbidden path
`docs/evidence/APITEST-016-admission.md`, mode100644/blob
`58f90e628a85e90d80e759746b7c9568c7eb8de9`. Its later deletion or a clean integration
head cannot make exact-ancestry publication pass. Preserve that failed audit and do
**not** integrate/push this history, tunnel it through a bundle, rewrite its SHA or
weaken the outgoing rule. Original local refs and objects remain untouched.

The sole current-state reference to da0fe1c is the supplemental
`complete_workflow.stages.implementation.results_stages.identity.post_closing_status_observation.closing_time_commit`.
It is not a packet revision, NI source pin or approval revision. The observation itself
says the flags are truthful post-closing observations, not content approved by the
identity closing, and points at the existing public receipt
https://github.com/Kernol12/subathon-bot-ai/pull/124#issuecomment-6834806589 .
A read-only current scan found all39 APITEST-016 primary/supplemental revision fields
reachable from origin/main, with da0fe1c the sole unavailable commit-valued field for
that task. These are initial findings, not a full object-inventory approval.

Proposed availability disposition is therefore limited to this **non-required status
observation**, subject to independent confirmation against the complete source/NI/
review inventory. Preserve its exact original SHA, complete supplemental data, public
receipt, reviewer identities and rejected history in archive and canonical proof.
Append this exact task-specific limitation, with the reviewed disposition receipt:
`Historical APITEST-016 status-observation commit da0fe1cb2df6fb25195a9c63cf10d20167a6ca6b is retained locally, not remotely reachable; it is not a required source or approval revision.`
Retain this in the proof limitations, bounded summary/STATUS and final audit; do not
claim remote availability of that Git object. All39 required revision references still
need actual remote object/mode/blob verification, as does every other task's inventory.
If any required dependency uses this SHA, or the independent classification fails,
block cutover and require a parent-compatible reviewed remedy; this proposal supplies
no fallback. It does not create a workflow exemption, change the frozen120, excuse
missing proof, invent an approved revision or replace the original string. Local
historical evidence is preserved; only its unavailable public-object claim is narrowed.
The separate availability approval is a source-bound documentary audit receipt in
contract evidence/PR, not a new stage or automatic legacy inference. Final005 must
report and recheck this limitation explicitly.

Compacting the fixed progress/state.json archive transport is the **last state write**
for that archive head; ordinary old rendering/reconcile commands would reformat it.
Preserve original unmodified bytes separately in the archive, compare semantic identity
and ordered values, and measure the actual merged blob before tagging/pinning. Additional
receipt writes require regeneration/compaction/revalidation or a replacement archive.

## Activated interface and compatibility retirement

Use one strict JSON parser (`proofs.read_json_bytes`/`read_json`) and one review engine
(`workflow.py`). Preserve002 loader/context/require_complete/convert shapes and003
policy guard/current binding semantics. Ordinary checks never fetch Projects.

| Interface after final004 merge | Contract |
| --- | --- |
| `python3 tools/proofs.py validate` | Canonical policy/records defaults; validate every record and exact enrollment set; retain explicit loader path options for disposable offline use |
| `python3 tools/proofs.py context TASK-ID [--stage NAME] [--compact]` | Canonical defaults, existing exact full/compact projection/serialization/warnings; unknown unenrolled ID refuses, no invented board projection |
| `python3 tools/check_project.py` | Independent repository checks plus specification version consistency with docs/STATUS.md; readable capability limits; no state/HTML reads/imports |
| `python3 tools/check_publication.py --base REF --head REF --task ID --stage NAME --schema-python PATH [--complete] [--bootstrap]` | Proof mode only; keep exact003 checkout/history/privacy/policy/binding checks; no legacy state fallback |

Remove prepared `--proofs` flag and `--repository-only` flag when their behavior is
the default; old spellings become argparse errors, never compatibility branches.
Retain --bootstrap only for the single initial final-cutover candidate with absent
base proof and verified archived ancestor; presence at base or any earlier canonical
introduction refuses. Normal later publication cannot bootstrap again.
Publication children become proofs validate, independent check_project and actual
proof/policy schema aggregate driver only. Retain exact one-test success witness
`{"tests":1,"failures":0,"errors":0,"skipped":0}\n`; remove progress/render children,
old progress witness driver and legacy output keys. Final summary retains003 proof
summary shape with checks exactly recorded_paths/outgoing_paths/proofs/policy_history/
project/proof_schema; retain stages, guard, task/base/head/commit_count. Its limitations
array is exactly, in this order:

1. `Recorded bindings are not authenticated approvals.`
2. `Forbidden-path checks are not complete secret scanning.`
3. `Current-head hosted, repository and owner gates are separate.`
4. `Filesystem checks can observe ignored local files.`
5. `Remote target-tip freshness and archive availability require coordinator readback.`
6. `Frozen exemptions preserve history and do not authorize forward completion.`
7. `Acceptance execution is a coordinator gate; proof completion does not validate result values.`
8. `Per-task specification references are archived; active brief references require coordinator review.`

OpenAPI/component/examples remain ordinary schema.repository coverage.

Retire tools/progress.py, template/controller/UI checker, state/current state schema,
PROGRESS.html, CMake progress target, old status/set/available/reconcile/render CLI,
workflow.validate_task/tracked/dependency_complete/reconcile/replace_outputs and
ordinary strict/tree-free status-controller branching. Keep pure kind minima,
review/publication grammar, stage completion/context/compact projector and safe path/
evidence primitives shared with proof/repository checks. Move pure primitives out of
progress without changing safety contracts. Retire renderer-only fixtures/registrations
only under the separately approved exact cleanup scope.

Current `proofs.prepare_conversion` locally imports progress and strict-admits a source
root; it cannot survive unchanged. After retirement, the historical convert API keeps
its approved shape, strict consumed-source validation and exact002 report. Replace the
ordinary conversion CLI's live source/root admission with a pinned Git archive adapter:
`convert --archive-commit FULL-SHA --output-dir NEW-DIR` reads only regular fixed
archive state/identities/report blobs, shared strict parser and tree-free archive
conversion grammar. Require the full immutable available commit; compare complete
recomputed report, reject malformed/unknown consumed fields, identity/eligibility/history
mismatch and unavailable objects. No arbitrary HEAD state, source-root materialization,
network lookup, writes to repository, or second workflow grammar. Output remains owned
private new-directory reservation with cleanup on failure and existing002 summary.
Publication provenance and the convert CLI use **one** archive-input adapter, shared
strict JSON reader and workflow grammar, not two parsers/loaders. Resolve the repository
root from the tool's actual checkout, verify Git's top-level equals it; resolve full
commits without replacement refs or lazy network fetch. Missing/shallow/partial history
or unavailable required objects are setup/refusal failures, never intended red/exemption.
The archive source language is tracker schema_version1 at the inspection revision,
not an evolving task format. Its top-level fields are $schema/schema_version/project/
baseline_version/specification_version/updated_at/summary/verification/tasks/activity/
workflow_policy; task source fields are id/title/module/kind/status/owner/depends_on/
spec_refs/brief/summary/notes/verification/evidence/review/workflow/publication/
supplemental_workflow_evidence with original required/optional distinctions. Pin the
original schema and converter consumed-field semantics at the full inspection revision;
unknown/version-changed source requires a reviewed amendment, not permissive inference.
The one archive adapter validates only this immutable conversion input, reuses shared
workflow grammar and exact historical types, and never checks today's retired local
paths. It must not retain task mutation, availability, rendering or atomic state/HTML
reconciliation merely to admit history. Original optional absence/null and ordered
values remain meaningful. Original disposable source/source-root/identities CLI options
retire. Before switch,
strict full source admission still runs with old reviewed tooling at the archive source
revision. Archive admission is confined to conversion/readback and publication provenance;
it cannot implement planning commands or a current task validator.

Default repository checks retain spec decision uniqueness, all acceptance-map decisions/
scenarios/version/test/evidence paths, Markdown containment/links, readable evidence
layout, presets and source declaration counts. docs/STATUS.md preserves old top summary
and verification with source pin, explicit historical/current distinctions and full
v0.45 scope/limits; no readiness percentages. STATUS has exactly one whole line `Specification version: 0.45` for the current
specification (use the actual sole `Version: VALUE` declaration in docs/specification.md).
Collect STATUS lines starting with literal `Specification version:`; require exactly
one and exact equality to `Specification version: VALUE`. Missing, duplicate or different
values refuse. No inferred version from headings, percentages or mutable board text.

Per-task spec_refs semantic validation retires with the tracker: the entire original
array and its successful pre-freeze admission are archived. Active briefs use heterogeneous
free-form specification prose, so this migration does not invent a parser or claim
semantic validation of it. Repository Markdown containment/local-link checks, complete
acceptance-map version/decision coverage and specification ID uniqueness remain enforced.
Coordinator reviews active brief specification/contract references at dispatch and closure;
005 checks the preservation map and representative/current active references. Declare this
narrow lost automated per-task check in STATUS/workflow and the exact cleanup scope;
it does not delete domain requirements or release coverage. Update AGENTS, CLAUDE, workflow, COORDINATOR_PROMPT,
checkpoint, PR template, README/docs navigation, scaffold startup, CMake/CTest/CI and
schema setup users atomically. Include scoped tools/AGENTS.md, scaffold/main.cpp,
docs/PROGRESS.md, docs/CLION_SETUP.md and module READMEs discovered by current inventory.
BASELINE_MANIFEST.json remains unchanged historical initial-snapshot evidence; no current
tool treats it as a regenerated task ledger. Historical accepted prose remains pinned, not falsely
rewritten as current implementation. Preserve all current check names/app bindings,
pins/permissions/admission, meaningful suites and product/native baselines.

## Minimal dispatch, enrollment and closure

Coordinator reads GitHub issue identity/status/assignment and native prerequisites,
then versioned brief/spec/contracts and proof context (if enrolled). Check owner stops
and actual prerequisite completion PR readback; no local availability controller.
Unenrolled old task kind/identity comes from the pinned full archive appendix. New IDs
use the exact003 whole-line per-ID Task ID/Work type/Issue stanzas in a selected complete
reviewed brief packet. No second kind ledger. A claim is GitHub assignment/activity
plus the coordinator checkpoint. At claim time create the brief stanzas, enrollment and
pending proof on the isolated task branch **before executable authoring**, not as an
already admitted main record. At claim declare the required packet that first binds a genuinely new
brief: contract when explicitly required (mandatory for contract kinds); otherwise tests
for implementation/test_draft/test_baseline/test_review kinds; otherwise implementation
for the remaining executable kinds, including code_review/cleanup. Existing kind minima
and any explicitly added stages remain unchanged. Review that packet through the full cycle. No PR touching
that new record may publish/merge until the selected complete reviewed packet binds the
brief. The candidate may exist locally while authoring; protected admission is later.
Missing required record, pending required stages at completion or unavailable archive
fails closed. Frozen120 exemptions cannot silently authorize forward work.

Preserve exact parent enrollment predicate, even planned evidence/blocked checks and
supplied exempt proof. No title/status/owner/dependency fields in proofs. Required record
set equals discovered set. Additive enrollment increases obligations without staling
unrelated packets, but **each** genuinely new ID, including an unselected addition,
needs its own reviewed brief admission; unrelated selected-task approval cannot admit it.
Bootstrap relocation compares archive brief with newer target-base bytes as well as
archive origin. It cannot revert newer acceptance to older archive bytes. Already
published immutable archive URLs remain valid after local006 brief deletion.

For final closure run --complete covering all required stages on latest candidate,
validate source mode/blob currency and ordered independent approvals, then inspect
actual latest-head CI, protection and resolved threads and merge normally. Read back
PR head/merge/time before closing issue/setting board completion. Optional publication
is an observation, never a prediction or substitute for merge facts. CI cannot detect
unrecorded board claims, authenticate identities/review quality or establish live
capabilities; coordinator comparison and005 audit remain required. No completion-only
PR loop. Epics close after children and final independent audit, without fake stages.

006 must inventory all active/shared/completed brief consumers, compact/consolidate
without requirement loss, archive full prior bytes remotely, repair current links and
actually remove eligible obsolete files with explicit retention exceptions. All briefs
remain accessible through004. Shared unfinished acceptance/review history remains
accessible afterward. Link updates or word targets do not complete006. WF006 receives
storage/dependency rescope only, keeps005 and WORKFLOW-005 prerequisites and all future
review/results requirements; WF007 remains behind it and both remain stopped.

### Acceptance and board capability limits

Retiring state also retires its mechanical `status == done` / `verification.acceptance
== passed` check. Proof --complete enforces current independent review/source gates,
**not** result-value/acceptance execution. Preserve the actual acceptance requirement:
coordinator must read source/head-bound execution receipts, confirm required suites ran
and passed with no disguised setup/skips, and record the exact receipt links in the
issue/PR completion note before protected merge/closure. Missing/failed/not_run evidence
is a concrete closure blocker even if --complete succeeds. Independent code closing
reviews and actual required CI remain mandatory. STATUS and workflow state this limitation
explicitly;005 audits actual receipts against task acceptance, not the historical map.
Do not implement WF006 results policy/fields or invent a results service to hide the loss.

Board reconciliation includes these roles and full post-write readback:

| Field / display | Authority and required disposition |
| --- | --- |
| Issue identity, title, Assignees, activity | GitHub owns current values. Preserve readable titles/original-name history; never invent an account from a historical agent/session string. Keep historical assignments in archive and actual dispatch identities in issue activity/checkpoint. Resolve current assignments explicitly. |
| Area | GitHub Area field is current module/area authority; preserve/reconcile old module values. |
| Kind | Display-only. Eligibility comes exclusively from enrolled proof kind, pinned old-task archive, or new-task reviewed stanza. Never read this editable select to choose gates. Reconcile the initial display and label this limitation in board README/workflow; no permanent kind ledger/sync. |
| Parent issue / sub-issues / blockedBy | Native containment and prerequisites are authoritative, distinct sets. Ten reviewed reclassifications occur before archive capture. |
| Epic custom select | Display-only projection of native containment, never another parent source. Reconcile each value against the single native parent; clear epic selections for standalone roots and the ten reclassified tasks (retain shared display only if clearly labelled non-parent). Native-parent views and dispatch prevail on disagreement. No permanent synchronizer. |
| Status | GitHub planning field, never proof/authorization. At cutover reconcile old planned/ready/in_progress/in_review/blocked/done to Planned/Ready/In progress/In review/Blocked/Done, incorporating verified newer completion/claims (001–003 Done,004 In progress). Done and closed only after final applicable merge/gates; intermediate archive PR cannot trigger004 completion. |
| Project README / views | Remove undated hardcoded task/completion/parent/blocker counts; link native current views and immutable dated archive summary. Verify backlog/current/epic filters and closure automation from actual configuration and read back changes. No unsupported assumption about UI/API behavior. |

WF006 rescope in the implementation packet removes `tools/progress.py` integration
and old task/progress storage ownership, replacing them with tools/proofs.py, shared
workflow.py and check_publication.py canonical proof/policy integration. Remove duplicate
Depends-on prose from WF006/007 headers and link their GitHub prerequisite authority;
ensure native WF006 blockers retain WORKFLOW-005 and MIGRATE-005, and WF007 retains
WF006. Preserve every future artifact/results-only policy requirement and digest/runner
constraint verbatim outside this storage/navigation rescope. Neither workflow starts.

Rollback pauses dispatch, captures all post-cutover GitHub/proof/PR facts, obtains a
protected reviewed PR restoring archived consumers and any006 removed briefs, repairs
links and reconciles intervening facts. GitHub stays authority until that rollback
merge; then state takes over once. Missing archive/history, unexplained facts or failed
CI/reviews blocks rollback. No dual authority, history rewrite or automatic product resume.
