# Prepared proof publication v1 — MIGRATE-003

Authored contract and independent examples; specification v0.45 §§17–18.
Author: `codex:migrate003-contract-author-20261113:/root` (gpt-6.1-sol).
Inspection HEAD: `4daa305fb0f205bc62d1d5430df1575b026770ae`.
Parents: [migration v1](github-task-migration-v1.md), approved source `b506434`,
and [proof records v1](proof-records-v1.md), approved source `8920909`.
This packet is authored, not approved or implemented. Required independent contract,
test-before-red and code cycles remain unchanged. No runtime/schema/test edit or
new test execution is evidence for this packet.

## 1. Resolve the NI baseline conflict first

The parent's “every required current mode/blob” rule cannot literally apply to all
bytes in a tests packet after implementing its NI seams. Actual MIGRATE-002 tests
revision `8aa9596917355ac70efe3b2132fffa0b227a2562` binds eight paths. Four are
oracles: `tests/tooling/test_proofs.py`, `tests/support/frozen_proof_policy.py`,
`tests/support/apitest016-supplemental-b506434.json` and
`tests/schema_validation/test_proof_schemas.py`. Four are deliberate NI source:
`tools/proofs.py`, `tools/workflow.py`, `progress/proof.schema.json` and
`progress/proof-policy.schema.json`. Approved implementation revision
`2009aa701bbcf6bc1c28b78ecdf4721124f50095` replaces those four NI files and binds
all four plus `tools/progress.py` and `tools/proof_shapes.py`.

Dropping the NI paths or rewriting the tests revision destroys baseline provenance.
Blindly letting implementation own every overlap would let implementation replace
an oracle. Requiring the old NI blobs at HEAD makes valid completion impossible.
Choose instead this **closed migration-only supersession rule**, without new proof
fields, digests, manifests or a second review grammar:

| Task | Tests packet baseline eligible for supersession | Replacement owner |
| --- | --- | --- |
| MIGRATE-002 | Exactly the four NI paths above, at exactly `8aa9596917355ac70efe3b2132fffa0b227a2562` | Implementation packet at exactly `2009aa701bbcf6bc1c28b78ecdf4721124f50095` |
| MIGRATE-003 | Only `tools/check_publication.py` and `tools/check_project.py`, if included in its independently reviewed tests packet as the loadable NI boundaries in §7 | Same task's subsequently reviewed implementation packet |

For each selected tests path, first read its regular Git mode/blob at the exact
tests revision. A missing baseline object is failure even if HEAD has a replacement.
If mode/blob still equals HEAD, ordinary binding applies. Otherwise supersession is
allowed only for a table entry, with all these conditions:

- Tests and implementation are required stages of this same task, both complete by
  `workflow.stage_complete`; preserve every ordered receipt, including rejections.
- Tests revision is a strict ancestor of implementation revision; both are ancestors
  of candidate HEAD. The named path occurs in both packets. Implementation's regular
  mode/blob equals HEAD exactly; its entire packet passes current binding too.
- No tests/fixtures/contract path can be superseded, even if implementation lists it.
  No path outside the closed table is eligible. 003 tests reviewers explicitly review
  its NI seams, unchanged production behavior and oracle separation before red.
- Original tests authors/revision/paths/reviews remain unchanged. The source of the
  historical NI baseline is still the tests revision, never the replacement revision.
  A new tests packet revision must receive its own full review cycle; supersession
  cannot make changed oracles current. 002's pinned pair cannot be substituted.

This is source ownership only. It does not make pending approvals complete, waive
independence, authorize execution, or authenticate identities. Every current source
is checked against either its own selected packet or the explicitly required complete
implementation packet. Every oracle remains bound to its tests packet. All historic
NI bytes and rejected approvals remain readable in full Git history. Missing history,
missing approvals, changed replacement source, arbitrary overlap or altered tests
refuse. New tasks/paths need a separately reviewed contract amendment; no opt-out flag.
No existing baseline assertion is removed in 001–003; 004 cleanup remains ADR-0012
scope with separate approval.

This explicitly amends migration §3's selected current-blob clause and proof-records
“One workflow grammar, two path boundaries” current-blob clause, only for the table.
It changes no JSON format or `require_complete` grammar. Original parent revisions
remain immutable in Git. Later contract edits, including these parent amendments,
are bound by the current task; they do not pretend to match old contract blobs.

## 2. Prepared public interface and ownership

Existing invocation without proof options retains its exact production behavior,
summary, four child checks and one-test progress-schema witness until 004. It keeps
reading state; no real canonical proof copies are introduced in 003.

Prepared invocation adds the following options to `tools/check_publication.py`:

```text
--proof-policy PATH --proof-records-dir DIR
[--complete]
--archive-identities PATH --archive-report PATH
[--bootstrap]
```

Existing required `--base`, `--head`, `--task`, repeated `--stage` and
`--schema-python` remain. Proof policy/records options are paired and select prepared
mode explicitly. Archive identity/report options are required in prepared mode and
forbidden in production mode; `--complete` and `--bootstrap` are prepared-only.
No environment fallback, live Projects access, auto-conversion or silent input default.
Selected stage names are unique; CLI order is retained. `--complete` requires the
selected stage set to equal the record's required stages and invokes existing
`proofs.require_complete(bundle, task_id)`; no missing stage is implicitly omitted.
Without it, selected pending packets may publish source checks with complete:false.
A missing selected packet refuses. Neither mode means task done or PR merged.

Policy/records supplied paths resolve relative to caller cwd, then must be canonical
contained repository paths with nonsymlink ancestors, regular policy/records and
loader discovery rules. They must be tracked at candidate HEAD with working bytes
matching that clean checkout. Publication compares those same relative locations at
base; no second base path. Disposable synthetic repositories provide prepared inputs
in 003; local converter output outside a repository is loader evidence, not publishable
source. Identity/report paths are likewise contained tracked regular files at HEAD,
and must match the bytes of the same paths at policy.archive_commit. No path to an
untracked private receipt can satisfy pinned archive provenance.

Keep `proofs.load`, `context` and `require_complete` and all existing public defaults.
Add only these loadable stdlib seams in `check_publication.py`:

```python
check_proof_policy(base: str, head: str, *, policy_path: Path,
                   records_dir: Path, identities_path: Path,
                   report_path: Path, bootstrap: bool = False) -> dict
check_proof_bindings(bundle: dict, task_id: str, stages: list[str], *,
                     head: str, complete: bool = False) -> list[dict]
```

Here Paths are absolute admitted checkout paths; Git revisions are resolved full
commits. First returns the guard object in §5; second returns stage objects there.
They revalidate supplied bundles/inputs rather than trusting callers. Use existing
Git helpers and workflow grammar. No fake status-bearing task objects, second loader
or copied review-order engine. ValueError/OSError mean refusal, not partial results.
Implementation may have private helpers; these are the bounded test seams.

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
Existing workflow cannot disappear. Retain historical packet versions in Git; forward
canonical authors/revision/paths may change only honestly, with currency enforced for
selected current work. Do not silently erase a pending/rejected packet to bypass review.
Newly enrolled archived IDs must match archived source kind and full reconciled identity;
genuinely new IDs need a per-ID brief with literal Task ID, canonical Work type and Issue
stanzas matching the record and included in a selected complete reviewed packet.
No normal-policy edits can authorize exemption changes or forward work on exempt IDs.

With neither policy nor records at base, require explicit `--bootstrap`. Presence of
only one is corruption. Reject if any ancestor of base already introduced those
proof locations: deletion of both cannot reset eligibility. This is the single initial
introduction, including explicit disposable rehearsal; no perpetual base-absent fallback.

In both modes require the full pinned archive commit, its `progress/state.json`,
complete `identities` object (proof-records conversion shape) and complete converter
`report` at the supplied pinned paths. In bootstrap, archive commit must be an ancestor
of current target base, not an outgoing/future commit. Coordinator publishes and
independently fetches/readbacks that archive/mapping/report before relying on them;
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
match today's HEAD. Current bindings apply to selected task/stages only (and §1's
replacement packet); all-record grammar/set/history checks still run. Supplied exempt
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
`Project check failed: ...` stderr/exit1. Prepared publication runs this independent
mode AND existing progress validate/render checks while state is authoritative. 004
alone removes the tracker children/default compatibility; no tests of deleted buttons.

Add `schema.proofs` CTest with `schema-validation;acceptance;tooling` labels and
TIMEOUT120, same env and `Python3_EXECUTABLE -I -S -B` as existing nodes, running
`tests/schema_validation/test_proof_schemas.py`. Existing schema.repository (8) and
schema.publication (3) remain; keep their assertions and exact old witness. Existing
schema-validation matrix lanes execute the added node through their current label
selection. Preserve all action/package/hash pins, SDK admission, check names, app
bindings and required branch protections. No live Projects/token in ordinary CI.

Prepared publication separately runs the real `ProofSchemas.test_proof_and_policy_contract`
aggregate and validates every supplied policy/record with the actual Draft202012Validator,
with offline deny-retrieval registry. Its setUpClass must perform existing SDK metadata
admission before standards imports. That single suite must count/run exactly one test;
failures/errors/skips/expected failures/unexpected successes are failures. Validate the
selected input bundle inside that aggregate invocation using its admitted validator;
no fake child witness replaces validation. Keep the existing aggregate's assertions.
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

Each stage keeps existing `name`, `revision`, `path_count`, `complete`, then adds
`baseline_supersessions`: array in packet-path order of exactly
`{"path": PATH, "baseline_revision": SHA, "owner_stage": "implementation",
"owner_revision": SHA}`; empty when none. path_count counts original packet paths.
The historical tests revision is never reported as HEAD-current replacement source.
Guard is exactly `{"mode": "existing" | "bootstrap", "record_count": N,
"archive_record_count": N, "legacy_exempt": boolean}`; counts describe candidate
and extracted non-epic records. No claim that bootstrap compared an existing policy.
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
| P2: valid contained prepared bundle, base==origin/main, unchanged selected contract, pending approvals | Success complete:false, empty supersessions; source/guard integration |
| P3: same bundle with all required complete stages selected | Success only with current bindings for all stages; `require_complete` reused |
| P4: mutate selected blob, chmod, missing/nonregular file or unavailable revision | Refuse; retain existing literal-path/content/mode cases |
| P5: metadata-only append or unrelated source change | Current selected bindings pass; changed selected oracle/contract fails |
| B1: actual 002 eight-path test packet plus pinned implementation pair | Four original oracle bindings remain; four exact NI supersessions; six implementation paths current; all old rejected receipts retained |
| B2: B1 minus implementation closing, missing old NI object, wrong revision, nonancestor, changed replacement or path omission | Refuse each independently; cannot hide failure behind NI sentinel |
| B3: implementation lists a changed test/fixture/contract or arbitrary tool path | Refuse; overlap alone grants no supersession |
| B4: 003 reviewed NI tests packet before implementation | Imports/CLI load; intended positive behavior fails named NI, production P1 remains passing; after complete implementation only two allowed seams may supersede |
| H1: delete record/shrink enrollment/stages; alter identity/kind/exemption/archive; edit/reorder/drop old review or historical object; drop/reorder limitations/evidence | Refuse each single mutation, even unrelated to selection |
| H2: additive archived ID matches full mapping/kind/history; new ID has complete reviewed per-ID brief | Pass; wrong mapping/kind/unreviewed brief fails |
| H3: unrelated archived completed task has retired paths and full preserved history | Load/guard pass; selecting it as current fails missing/current binding, never retroactive full-tree validation |
| H4: frozen exempt ID without workflow | Historical load/report allowed; require_complete and forward --complete refuse; malformed supplied exempt record still fails |
| A1: base lacks both inputs, explicit bootstrap, complete pinned source/mapping/report and exact extraction | Bootstrap success/counts; no existing-policy claim |
| A2: missing mapping/archive/report, one-sided absence, false counts/comparisons, omitted source task, extraction mismatch, bootstrap omitted or used with existing policy | Refuse; unavailable input is setup failure, not intended red |
| A3: older ancestor --base, missing origin/main, archive not ancestor of target, both inputs deleted after prior introduction | Refuse before base-absent bypass; updated target requires coordinator rerun |
| G1: private file added then removed or side-merge-only private path | Refuse every-tree check; retain existing fixture cases |
| S1: valid proof/schema examples plus actual selected inputs in admitted interpreter | Two separate exact one-test witnesses plus original eight/three schema checks preserved |
| S2: existing semantic-valid/schema-invalid state case; fixture-local proof schema that rejects only the selected synthetic ID while aggregate examples still pass; empty-success/missing executable, missing SDK/ref, extra witness byte or skip | Refuse actual schema/admission/witness; setup errors not assertion red |
| R1: call repository-only with deliberately invalid tracker/render but intact repository artifacts | Independent checks pass; default/explicit tracker checks still refuse |
| R2: corrupt acceptance map/test path/evidence link/preset/Markdown target | Independent and default checks refuse; no assertion removed |

B1 can be a bounded full-history reproduction using pinned actual source; ordinary
unit/CI cases should use synthetic histories with literal independent inputs and the
same ownership rules. Do not generate hundreds of real records or expected values
from candidate code. Schema integration must demonstrate actual validation, not only
mock child exit codes. Existing 154 tooling + proof-schema1 + schema8 + SDK-publication3
passes are historical 002 evidence, not runs or new-behavior evidence in this session.

## 7. Next sessions and loadable red boundary

After contract closing approval, a separate tests author owns focused additive
publication/project/SDK tests and shared synthetic fixture changes. Keep old assertions,
including production summary/witness, unusual literal paths, all outgoing privacy,
checkout/merge/dirty-state and real validator refusals. Test expected results come
from §6 and the parents, not the implementation. Preserve scoped AGENTS and ownership.

That author may add public Python signatures in §§2/4 and CLI parsing for prepared
options; each unimplemented new operation raises
`ValueError('PROOF_PUBLICATION_NOT_IMPLEMENTED')`. Production branches remain functional.
CLI prepared mode emits `Publication error: PROOF_PUBLICATION_NOT_IMPLEMENTED` with
exit1/empty stdout after parsing. Tests prove imports/options/setup independently;
positive behavior assertions then observe named NI after the full independent test
review cycle. Negative safety tests must reach actual safety checks; NI is not evidence
of refusal. Repository-only NI remains isolated from default production until fresh
implementation. Tests packet binds its own oracles/fixtures and the two NI source paths;
implementation authorship credits actual source authors without pretending the oracle
author approved implementation. No new schema NI: both proof schemas already exist.

Coordinator records exact tests source/paths/authors, all ordered review receipts and
actual assertion red. Fresh implementation then owns runtime/schema-driver/CMake/CI
changes and gets a separate full independent code cycle and relevant checks. Current
stage is contract authoring only. 004 removes explicit prepared/legacy mode selection,
old state/render consumer compatibility and schema witness dependency through separately
reviewed cutover/ADR-0012 work; no permanent second authority or legacy facade. Product
and WORKFLOW-006/007 execution remain stopped; full v0.45 scope is unchanged.
