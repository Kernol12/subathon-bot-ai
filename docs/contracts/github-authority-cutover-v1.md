# GitHub authority cutover v1 — MIGRATE-004

Authored proposal, specification v0.45 §§17–18. Author:
`codex:migrate004-contract-author-20261113:/root` (Codex gpt-6.1-sol).
Inspection revision: `820544f1093f1649b0a7b03b5e7fcd12ed9cfcab`, branch
`task/MIGRATE-004-github-authority`, based on merged main
`b80e94f514caf33cad8abde8d00fa744b565768a`.
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
proof fields. Intermediate PRs reference #629 without closing keywords. A merged
intermediate publication observation is historical, not004 completion. Preserve any
immutable merged observation; record subsequent PR facts in append-only evidence/PR
receipts rather than replacing it. Update current packet revision/authors/paths honestly,
retain every prior review as ordered prefix and obtain renewed verification/closing
for material changes. Tests paths bind retained/new oracles and fixtures only; removed
paths/methods and NI modes/blobs stay pinned at their original reviewed revision in
baseline receipts. Implementation packets cover current runtime/schema/consumer changes.
There is no task/SHA exception or fake current path. If final policy/source changes
stale a packet, re-review; receipt bookkeeping does not bind itself.

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
project/proof_schema; retain stages, guard, task/base/head/commit_count and honest
limitations. OpenAPI/component/examples remain ordinary schema.repository coverage.

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
v0.45 scope/limits; no readiness percentages. Validate its declared specification version
against docs/specification.md, without inventing a new results/status engine. Preserve
archived task spec refs and validate active brief references/links; do not discard spec
coverage with the tracker. Update AGENTS, CLAUDE, workflow, COORDINATOR_PROMPT,
checkpoint, PR template, README/docs navigation, scaffold startup, CMake/CTest/CI and
schema setup users atomically. Historical accepted prose remains pinned, not falsely
rewritten as current implementation. Preserve all current check names/app bindings,
pins/permissions/admission, meaningful suites and product/native baselines.

## Minimal dispatch, enrollment and closure

Coordinator reads GitHub issue identity/status/assignment and native prerequisites,
then versioned brief/spec/contracts and proof context (if enrolled). Check owner stops
and actual prerequisite completion PR readback; no local availability controller.
Unenrolled old task kind/identity comes from the pinned full archive appendix. New IDs
use the exact003 whole-line per-ID Task ID/Work type/Issue stanzas in a selected complete
reviewed brief packet. No second kind ledger. Enroll claimed non-exempt work before
authoring; missing required record, pending kind-minimum stages or unavailable archive
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

Rollback pauses dispatch, captures all post-cutover GitHub/proof/PR facts, obtains a
protected reviewed PR restoring archived consumers and any006 removed briefs, repairs
links and reconciles intervening facts. GitHub stays authority until that rollback
merge; then state takes over once. Missing archive/history, unexplained facts or failed
CI/reviews blocks rollback. No dual authority, history rewrite or automatic product resume.
