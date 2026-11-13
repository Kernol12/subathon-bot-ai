# Prepared proof publication v1 — MIGRATE-003

Authored contract and independent examples; specification v0.45 §§17–18.
Author: `codex:migrate003-contract-author-20261113:/root` (gpt-6.1-sol).
Inspection HEAD: `4daa305fb0f205bc62d1d5430df1575b026770ae`.
Parents: [migration v1](github-task-migration-v1.md), approved source `b506434`,
and [proof records v1](proof-records-v1.md), approved source `8920909`.
This packet is authored, not approved or implemented. Required independent contract,
test-before-red and code cycles remain unchanged. No runtime/schema/test edit or
new test execution is evidence for this packet.

## 1. Baseline history and current stage ownership

Completed tasks remain tree-free historical evidence, never republished as new current
work. MIGRATE-002, WORKFLOW-005 and APITEST-016 retain their exact original packets,
including NI files, revisions, authors, ordered reviews and rejected history. No task,
path or SHA allowlist belongs in the checker. A historical packet selected for current
binding still undergoes ordinary mode/blob checks and may fail because its files or
contracts changed; historical completion is not revoked by that observation. The
checker cannot infer GitHub task status and does not grant new execution authority.

From MIGRATE-003 forward, tests stage `paths` bind only test oracles, fixtures and
stable test-owned source. The separately reviewed NI seam files are **not** in this
current-binding path list. The exact tests revision still pins their original Git
objects; test review reports and actual red receipts explicitly name the NI paths,
original modes/blobs and revision. Full-history baseline inspection verifies those
objects when reviewing/reproducing red. Thus no NI source provenance is erased or
claimed unchanged at implementation HEAD. The implementation packet owns all current
runtime/schema changes and credits any actual NI source author whose code it retains.

Current `--complete` requires every required stage complete and every listed path to
match candidate HEAD normally. Every oracle remains bound to its tests packet; changed
or missing oracle/fixture/contract paths stale the corresponding stage even if the
implementation packet also lists them. They require renewed independent verification/
closing on the resulting affected packet. Every current implementation source must
be covered by its separately reviewed implementation packet. No overlap precedence,
supersession exception, new proof field or second grammar is introduced. Missing
historical NI objects cannot count as reproducible baseline evidence; current source
checking does not manufacture that baseline from implementation bytes.

This is prospective source-packet ownership, not reinterpretation of historical
packets. Original parent contracts and receipts remain at their approved Git revisions.
The explanatory notices in those documents point here; their general current-blob
rule stays intact. Existing historical NI-containing packets stay unchanged and get
no special `--complete` bypass. Synthetic tests create their own ordinary commits and
reviewed packets; shallow CI never requires repository historical task SHAs. No
assertion removal is authorized in 001–003; 004 cleanup remains separately reviewed.

The current003 contract packet includes a factual 002 completion summary. Hold that
summary unchanged through 003 publication; record 003 completion in the next authorized
004 metadata instead of staling this packet. Prefer separate review context for shared
summaries in future packets. A later summary update does not rewrite completed history.

## 2. Prepared public interface and ownership

Existing invocation without proof options retains its exact production behavior,
summary, four child checks and one-test progress-schema witness until 004. It keeps
reading state; no real canonical proof copies are introduced in 003.

Prepared invocation adds the following options to `tools/check_publication.py`:

```text
--proofs [--complete] [--bootstrap]
```

Existing required `--base`, `--head`, `--task`, repeated `--stage` and
`--schema-python` remain. `--proofs` explicitly selects prepared mode at fixed canonical locations
`progress/proof-policy.json` and `progress/proofs/`. There are no caller-selected
proof or archive filesystem paths. `--complete` and `--bootstrap` are prepared-only.
No environment fallback, live Projects access, auto-conversion or silent input default.
Selected stage names are unique; CLI order is retained. `--complete` requires the
selected stage set to equal the record's required stages and invokes existing
`proofs.require_complete(bundle, task_id)`; no missing stage is implicitly omitted.
Without it, selected pending packets may publish source checks with complete:false.
A missing selected packet refuses. Neither invocation means task done or PR merged.

Candidate proof inputs are fixed contained repository paths with nonsymlink ancestors
and regular policy/record files under loader discovery rules. They must be tracked at
candidate HEAD and match its clean working bytes. Base discovery always uses these
same canonical paths. Alternate-location options are usage errors, never a new bootstrap
universe. Disposable synthetic repositories use those canonical locations too; loader
CLI outside a repository remains separate conversion evidence.

Archive inputs are read only from regular Git blobs at policy.archive_commit:
`progress/state.json`, `docs/migrations/EPIC-029/archive/identities.json`, and
`docs/migrations/EPIC-029/archive/conversion-report.json`. The full reconciliation
appendix is distinct, at `docs/migrations/EPIC-029/archive/reconciliation.json`.
No corresponding HEAD file or private/untracked receipt is required. At cutover these
full archive files leave the active tree; only the bounded summary/pins remain there.

Keep `proofs.load`, `context` and `require_complete` and all existing public defaults.
Add only these loadable stdlib seams in `check_publication.py`:

```python
check_proof_policy(base: str, head: str, *, task_id: str,
                   bootstrap: bool = False) -> dict
check_proof_bindings(bundle: dict, task_id: str, stages: list[str], *,
                     head: str, complete: bool = False) -> list[dict]
```

Git revisions are resolved full commits. First returns the guard object in §5;
legacy_exempt means exactly task_id in the candidate policy's legacy_done. The selected
ID must have a record. Second returns the stage objects there.
They revalidate supplied bundles/inputs rather than trusting callers. Use existing
Git helpers and workflow grammar. No fake status-bearing task objects or copied review-order engine. ValueError/OSError mean refusal, not partial results.
Implementation may have private helpers; these are the bounded test seams.

Authorize one narrow shared JSON transport entry point in tools/proofs.py:
`read_json_bytes(payload: bytes, source: str) -> object`. It shares read_json's strict
UTF-8/no-BOM, duplicate-key, nonfinite/depth-error hooks and ValueError semantics;
read_json keeps its filesystem preflight then delegates to the same parser. Git
transport uses existing literal ls-tree helpers and blob bytes, not a second JSON or
review validator, no SDK, checkout writes or temporary materialization. Both candidate
and base tree transport call proofs.validate on the parsed objects; this is one Git transport adapter over the
shared parser/validator, not a new semantic loader. Immediate record
tree children must all be canonical ID.json regular blobs with modes100644/100755;
reject symlinks120000, submodules160000, nested directories040000, hidden/other names
and filename/ID mismatch. Missing policy/directory refuses except the explicit initial
bootstrap check. Archive blobs get the same regular-mode/strict-parser checks.

## 3. Checkout, policy history and one-time bootstrap

Prepared mode preserves actual ROOT, HEAD identity, detached-HEAD support, ancestor,
merge-state and clean-tree checks. Before either guard branch, base must equal the
resolved local `refs/remotes/origin/main`, and be an ancestor of actual HEAD. Refuse
missing ref and arbitrary older ancestors. Full-history operations use real objects,
no replacement refs; missing/shallow history is failure, never exemption. Immediately
before accepting a receipt and normal protected merge, coordinator freshly reads/fetches
target main and verifies equality to receipt.base; if moved, update/reconcile/rerun.
Offline success cannot authenticate target-tip freshness.

Inspect every tree of every newly reachable commit, including merge side history,
for all existing private-path exclusions. Added-then-deleted/renamed private paths
still refuse. No change to output bounds, deadlines or their honest process limits.

With base policy present, `--bootstrap` refuses. Load the complete base and candidate
bundles. Freeze archive pin, original ordered frozen 120 exemptions/fingerprint,
existing task ID/issue/kind; refuse enrollment shrink, record deletion or required-stage
shrink. For every existing packet, reviews retain the base array as an exact ordered
prefix. Existing verification/review/supplemental historical objects retain exact values
and presence; limitations and evidence retain their original ordered arrays as prefixes.
Existing workflow and every existing packet cannot disappear. Required-stage arrays
retain the base order as a prefix; added stages/packets may start pending and do not
invent completion. If archived workflow/publication was absent it may be added under
normal validation, while existing observed publication may advance only from open to
merged for the same PR/head with valid observed times; an already merged observation
is immutable. Stage authors/revision/paths may honestly advance without removing any
prior review. No historical verification/review/supplemental field may be invented,
removed or changed as a forward update. Metadata-only observations grant no approval.
A source-changed current packet must obtain fresh applicable verification/closing;
only selected work claims current binding, and the coordinator reviews all changed
record scope. No mutable GitHub issue content can silently replace acceptance.

Freeze brief references too, with one relocation: a local archived path P may become
`https://github.com/Kernol12/subathon-bot-ai/blob/<policy.archive_commit>/P` only when
that exact regular path exists in the pinned archive. No branch URL, changed path,
different SHA or arbitrary active-brief swap is allowed. Active compaction keeps stable
per-ID entry paths; shared consolidations can retain those entry briefs with versioned
links to common requirements. Any different pointer move needs a separately reviewed
contract correction. Evidence remains an exact prefix; old local historical references
load tree-free and remain traceable through the archive, not silently rewritten.006
repairs current GitHub/navigation/evidence-index links and appends pinned references
where needed; immutable historical proof references preserve their original spellings. Do not silently erase a pending/rejected packet to bypass review.
Newly enrolled archived IDs must match archived source kind and full reconciled identity;
genuinely new IDs need a per-ID brief containing exactly one each of these whole-line
stanzas (literal ASCII labels, case-sensitive): `Task ID: NEW-001`,
`Work type: implementation`, `Issue: https://github.com/Kernol12/subathon-bot-ai/issues/123`.
Values must equal the record's canonical ID/kind/derived URL. Duplicate/conflicting
stanzas refuse; no inference from display labels or a mutable issue body. The brief
must be in a selected complete reviewed packet. Existing archived IDs use the archive
mapping and do not need retroactive stanza edits.
No normal-policy edits can authorize exemption changes or forward work on exempt IDs.

With neither policy nor records at base, require explicit `--bootstrap`. Presence of
only one is corruption. Reject if any ancestor of base already introduced those
canonical proof locations: deletion of both cannot reset eligibility. This is the
single initial introduction, including explicit disposable rehearsal. A rollback does
not permit another bootstrap; re-cutover would require a new reviewed amendment,
not a perpetual base-absent fallback.

In both modes require the full pinned archive commit, its `progress/state.json`,
complete identities object (proof-records conversion shape) and complete converter
report at the fixed archive-tree paths above. In bootstrap, archive commit must be an ancestor
of current target base, not an outgoing/future commit. The 004 coordinator uses this order: first a focused protected archive-preparation PR
adds the frozen source, identities, converter report and separate reconciliation
appendix. Merge by normal merge commit after its gates, then independently fetch/read
back required objects and record the actual archive tag object/full commit. The final
cutover PR pins that existing ancestor commit and removes the full appendices from
HEAD, retaining only the bounded summary. It cannot close 004 before the final cutover
merge. There is no hash cycle: conversion-report contains no archive_commit field, so
its independently derived content can be committed before the pin is known; final
proof policy pins the actually observed archive commit. No guessed future SHA.
Coordinator publishes and independently fetches/readbacks archive objects before use;
003 uses synthetic local pins and does not claim remote availability. The guard does
not fetch or authenticate that coordinator evidence. No future commit is predicted.

Parse archive inputs strictly; require the source's complete task inventory and
workflow_policy, exact identity ID-set including epics, unique issue mappings and
archived kinds. Run existing `proofs.convert` over that source and mapping, using
policy.archive_commit; compare the supplied report to its entire report object,
including counts, archive-only/enrolled IDs, mappings, every field comparison and
empty discrepancies. A manually supplied all-true summary is insufficient. This
validates extraction, not historical current-tree paths: use tree-free archive grammar,
not today's retired-path validation. The independently reviewed archive/reconciliation
and coordinator inventory establish completeness against the frozen pre-cutover source;
CI alone cannot detect an omitted board-only task.

Bootstrap must preserve all extracted records, exact original limitations/evidence,
optional historical objects and original required stages/reviews. Forward continuation
may append evidence/limitations/reviews and honestly update packet authors/revision/paths
without shrinking stages. Compare baseline packets to the full pinned source, not just
hash/count/booleans; reject fabricated or reordered history. Additions follow the same
archived-ID/new-ID admission as normal mode. Normal mode also checks archive-origin
history against this baseline, preventing a forged already-corrupt base from replacing
provenance. The report is existing one-time conversion output, not a new results engine.

All unrelated archived records load tree-free. Do not require their retired paths to
match today's HEAD. Current bindings apply to selected task/stages only (without retroactive historical current-tree checking); all-record grammar/set/history checks still run. Supplied exempt
records validate and report historical exemption, but `require_complete` still refuses
missing workflow. Prepared publication never reports forward completion for an exempt
ID: absent a separately reviewed policy amendment, `--complete` refuses such selection
even if receipts were added. Reporting the frozen exemption is not starting new work.

## 4. Independent repository and actual-schema checks

Factor `tools/check_project.py` without removing any meaningful assertion:

```python
check_repository(root: Path) -> dict
check_tracker(data: dict, root: Path) -> dict
```

First owns specification decision uniqueness, complete acceptance-map/version coverage,
every mapped test/evidence path, evidence README-only layout, preset format/references,
README/docs Markdown containment/local links and TimeCredit source declaration count.
It reads no tracker or rendered HTML. Return exactly `{"decisions": N,
"time_credit_declarations": N}`. Tracker owns state validation, specification version
and task refs, exact render currency, offline assets/unique anchors and HTML links;
return exactly `{"tasks": N}`. Root is explicit for both, including progress helpers.
Default CLI calls both and keeps current messages/behavior. Add `--repository-only`:
only repository checks, exit0 stdout `PASS: repository checks\n`; errors retain
`Project check failed: ...` stderr/exit1. Prepared publication keeps default check_project (which calls BOTH functions) plus
existing progress validate/render while state is authoritative, preserving task spec
refs, HTML asset/anchor/link checks and state/spec version checks. Repository-only is
independently tested as the prepared cutover consumer;004 alone removes tracker
children/default compatibility. No tests of deleted buttons.

Add `schema.proofs` CTest with `schema-validation;acceptance;tooling` labels and
TIMEOUT120, same env and `Python3_EXECUTABLE -I -S -B` as existing nodes, running
`tests/schema_validation/test_proof_schemas.py`. Existing schema.repository (8) and
schema.publication (3) remain; keep their assertions and exact old witness. Existing
schema-validation matrix lanes execute the added node through their current label
selection. Preserve all action/package/hash pins, SDK admission, check names, app
bindings and required branch protections. No live Projects/token in ordinary CI.

Prepared publication's fixed proof-schema driver loads the existing
ProofSchemas class, runs exactly its one aggregate test with actual SDK metadata
admission, and refuses failures/errors/skips/expected failures/unexpected successes.
After that suite succeeds, in the SAME process use the class's admitted Draft and
offline registry to validate the actual canonical policy and every actual record
against shipped schemas. Read these fixed ROOT-relative files through the shared
strict parser; never take an environment fallback or caller-selected alternate input.
Only after BOTH the full aggregate and selected-input validation succeed emit the
one-test witness. This narrow fixed driver is owned by003 implementation; it leaves
the aggregate oracle file/assertions unchanged. CTest schema.proofs runs the aggregate
without selected production inputs, so no canonical production proof copy is needed
before004. Retain both original production driver and its one-test progress witness.
Exact successful stdout bytes for EACH schema child are
`{"tests":1,"failures":0,"errors":0,"skipped":0}\n`. Missing SDK/interpreter, `/bin/true`,
extra output, schema-invalid semantic-valid input and unavailable offline refs refuse.
Use the admitted `build/workflow005-schema-venv/bin/python` spelling made absolute,
never resolve its venv symlink, install dependencies or switch interpreters for green.

Schemas check closed shapes/types/enums/patterns; semantic loader owns checksums,
complete enrollment/discovery, URL rules, kind minima, review grammar/independence,
ordering and completion. Git guard owns historical equality, source currency and
provenance. Schema success cannot replace either semantic or Git checks. Ordinary
shallow CI validates candidate schema/grammar/examples; only explicit full-history
local publication claims the guard. Adding CMake registration is ordinary passing
infrastructure after review, not manufactured red from missing registration.

## 5. Success and failure shape

Prepared CLI returns exit0, indent2 ensure_ascii=False JSON plus LF, in this key order:
`task`, `base`, `head`, `commit_count`, `stages`, `guard`, `checks`, `limitations`.
No file writes/ref/index changes or network. Base/head are resolved full SHAs.

Each stage is exactly the existing name, revision, path_count, complete shape.
There is no baseline_supersessions field or per-task exception table. Current packets
bind normal listed files; baseline NI provenance remains in immutable review/red receipts.
Guard is exactly `{"mode": "existing" | "bootstrap", "record_count": N,
"archive_record_count": N, "legacy_exempt": boolean}`; counts describe candidate
and extracted non-epic records. mode existing means a base canonical policy existed;
legacy_exempt is the selected task's membership, never merely a nonempty exemption list. No claim that bootstrap compared an existing policy.
Checks keys in order: `recorded_paths`, `outgoing_paths`, `proofs`, `policy_history`,
`progress`, `render`, `project`, `schema`, `proof_schema`, each `"passed"`.
Limitations retain the four existing strings verbatim, followed by:
`"Remote target-tip freshness and archive availability require coordinator readback."`
and `"Frozen exemptions preserve history and do not authorize forward completion."`
No local diagnostics/private absolute paths or arbitrary child output in JSON.

Failures: exit1, empty stdout, `Publication error: ...` stderr with actionable cause;
argparse misuse exit2. No partial success report. Subprocess failures preserve private
stderr diagnostics under existing limits. `--complete` still cannot authenticate actual
final PR merge, review identity/quality, hosted CI, board completeness or authorization.

## 6. Independent bounded oracle inventory

These are expected outcomes, not generated implementation results. Synthetic IDs,
reviewers and commits below never represent real approvals. Reuse existing fixture,
frozen 120-ID list, APITEST-016 sample and reviewed schema oracles, with focused additions.

| Case / independent input | Expected result / primary owner |
| --- | --- |
| P1: no new options, existing synthetic fixture | Exact existing summary/witness and production path; existing publication tests |
| P2: valid contained prepared bundle, base==origin/main, unchanged selected contract, pending approvals | Success complete:false; ordinary source/guard integration |
| P3: same bundle with all required complete stages selected | Success only with current bindings for all stages; `require_complete` reused |
| P4: mutate selected blob, chmod, missing/nonregular file or unavailable revision | Refuse; retain existing literal-path/content/mode cases |
| P5: metadata-only append or unrelated source change | Current selected bindings pass; changed selected oracle/contract fails |
| B1: synthetic future tests packet binds unchanged oracles/fixtures, NI files exist at its pinned revision; complete implementation replaces NI source | All ordinary current stage bindings pass; no exception table or repository historical SHA needed |
| B2: B1 with changed oracle, missing required stage/closing, missing/nonregular current source or stale implementation | Refuse; implementation overlap cannot rescue a changed tests binding |
| B3: completed002/WF005/APITEST016 packets retained with retired/changed source | Load/history checks pass; selecting stale historical packets as current refuses normally, no historical approval rewrite |
| B4:003 reviewed NI seams before implementation | Imports/CLI load; intended positive assertions fail named NI; default production stays passing. NI paths are review/red provenance, not current tests paths |
| H1: delete record/shrink enrollment/stages; alter identity/kind/exemption/archive; edit/reorder/drop old review or historical object; drop/reorder limitations/evidence; swap brief arbitrarily | Refuse each single mutation, even unrelated to selection |
| H2: additive archived ID matches full mapping/kind/history; new ID has exact reviewed canonical stanzas; same archived local brief becomes pinned same-path URL | Pass; wrong mapping/kind/stanza/unreviewed brief/different brief path or SHA fails |
| H3: unrelated archived completed task has retired paths and full preserved history | Load/guard pass; selecting it as current fails missing/current binding, never retroactive full-tree validation |
| H4: frozen exempt ID without workflow | Historical load/report allowed; require_complete and forward --complete refuse; malformed supplied exempt record still fails |
| A1: base lacks both inputs, explicit bootstrap, complete pinned source/mapping/report and exact extraction | Bootstrap success/counts; no existing-policy claim |
| A2: missing mapping/archive/report, one-sided absence, false counts/comparisons, omitted source task, extraction mismatch, bootstrap omitted or used with existing policy | Refuse; unavailable input is setup failure, not intended red |
| A3: older ancestor --base, missing origin/main, archive not ancestor of target, both inputs deleted after prior introduction; alternate-location options over an existing canonical policy | Refuse before base-absent bypass; updated target requires coordinator rerun; rollback does not reset bootstrap |
| G1: private file added then removed or side-merge-only private path | Refuse every-tree check; retain existing fixture cases |
| S1: valid proof/schema examples plus actual selected inputs in admitted interpreter | Two separate exact one-test witnesses plus original eight/three schema checks preserved |
| S2: existing semantic-valid/schema-invalid state case; fixture-local proof schema that rejects only the selected synthetic ID while aggregate examples still pass; empty-success/missing executable, missing SDK/ref, extra witness byte or skip | Refuse actual schema/admission/witness; setup errors not assertion red |
| R1: call repository-only with deliberately invalid tracker/render but intact repository artifacts | Independent checks pass; default/explicit tracker checks still refuse |
| R2: corrupt acceptance map/test path/evidence link/preset/Markdown target | Independent and default checks refuse; no assertion removed |

All ordinary unit/CI cases use synthetic Git histories with literal independent inputs;
no actual historical task SHA is required in shallow CI. Historical source comparisons
and archive readback remain full-history operational gates. Do not generate hundreds of real records or expected values
from candidate code. Schema integration must demonstrate actual validation, not only
mock child exit codes. Existing 154 tooling + proof-schema1 + schema8 + SDK-publication3
passes are historical 002 evidence, not runs or new-behavior evidence in this session.

## 7. Next sessions and loadable red boundary

After contract closing approval, a separate tests author owns focused additive
publication/project/SDK tests and shared synthetic fixture changes. Keep old assertions,
including production summary/witness, unusual literal paths, all outgoing privacy,
checkout/merge/dirty-state and real validator refusals. Test expected results come
from §6 and the parents, not the implementation. Preserve scoped AGENTS and ownership.

That author may add public Python signatures in §§2/4, the shared read_json_bytes
NI entry point (existing read_json stays functional until fresh factoring) and
CLI parsing for prepared options; each unimplemented new operation raises
`ValueError('PROOF_PUBLICATION_NOT_IMPLEMENTED')`. Production branches remain functional.
CLI prepared mode emits `Publication error: PROOF_PUBLICATION_NOT_IMPLEMENTED` with
exit1/empty stdout after parsing. Tests prove imports/options/setup independently;
positive behavior assertions then observe named NI after the full independent test
review cycle. Negative safety tests must reach actual safety checks; NI is not evidence
of refusal. Repository-only NI remains isolated from default production until fresh
implementation. Each negative has a passing positive control and one independent mutation, and must
exclude the NI marker rather than count it as refusal. Tests packet paths bind only
its oracles/fixtures/stable test-owned files; review reports/red evidence separately
pin NI paths and exact modes/blobs at the same tests revision. Implementation packet
owns current source and credits every actual retained NI author, without pretending
the oracle author approved implementation. No new schema NI: both proof schemas already exist.

Coordinator records exact tests source/paths/authors, all ordered review receipts and
actual assertion red. Fresh implementation then owns runtime/schema-driver/CMake/CI
changes and gets a separate full independent code cycle and relevant checks. Current
stage is contract authoring only. 004 removes explicit prepared/legacy mode selection,
old state/render consumer compatibility and schema witness dependency through separately
reviewed cutover/ADR-0012 work; no permanent second authority or legacy facade. Product
and WORKFLOW-006/007 execution remain stopped; full v0.45 scope is unchanged.
