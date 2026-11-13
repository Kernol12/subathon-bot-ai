# Proof records v1 — MIGRATE-002

Authored contract/examples, specification v0.45 §§17–18.
Author: `codex:migrate002-contract-author-20261113:/root` (gpt-6.1-sol).
Correction author: `codex:coordinator-59095155`.
Parent: [approved migration contract](github-task-migration-v1.md), source
`b506434`; this refines its §9 without changing policy or claiming review.
Inspection base: `fb324d23c20df24fad504e50f0046726de6cc61c`.

## Authority and implementation boundary

002 supplies a standard-library `tools/proofs.py`, two Draft 2020-12 schemas
`progress/proof.schema.json` and `progress/proof-policy.schema.json`, a disposable
one-time converter, and focused tests. Production continues to read only state.
Do not populate `progress/proofs/` or `progress/proof-policy.json` from real tasks
in 002. The schemas describe prepared inputs, not a second editable proof authority.
003 owns opt-in publication/project/schema-driver/CMake/CI integration and the
cross-revision guard below; 004 alone changes defaults, extracts canonical proof,
archives and removes compatibility. No board, dependency, assignment or status API.
WORKFLOW-006/007 digest/results policy and product execution remain stopped.

002 adds admitted real-schema tests for the two schemas and disposable examples,
keeping current progress/OpenAPI/example tests and their assertions intact. It does
not change the production publication driver or its one-test witness. The ordinary
loader never imports SDK libraries. Actual standards validation uses only the
existing isolated, admitted SDK and offline references, not a new schema interpreter.

## Public Python and command interfaces

`tools/proofs.py` exports these functions (Path arguments are explicit; no ambient
working-directory lookup, network, Git, writes or global state mutation):

```python
read_json(path: Path) -> object
discover_records(directory: Path) -> list[Path]
validate_policy(policy: object) -> None
validate_record(record: object) -> None
validate(policy: object, records: list[dict]) -> None
require_complete(bundle: dict, task_id: str) -> None
load(policy_path: Path, records_dir: Path) -> dict
context(bundle: dict, task_id: str, *, stage: str | None = None,
        compact: bool = False) -> dict
convert(source: dict, identities: dict, archive_commit: str) -> dict
```

`load` returns exactly `{"policy": policy, "records": {task_id: record, ...}}`,
records inserted by sorted canonical filename. It parses then validates everything;
never return partial results. `validate` does the same in-memory validation/set
checks without filesystem discovery. `context` accepts a validated bundle but
revalidates it before selection so unrelated invalid proof cannot be hidden.
`convert` returns exactly `{"policy": ..., "records": [...], "report": ...}`;
records sorted by ID. Inputs and returned historical objects must not alias mutable
objects: no mutation of source/bundle by conversion or projection.

`ValueError` means malformed JSON, duplicate keys at any depth, NaN/Infinity,
invalid shape/semantics, missing/extra enrollment, unsafe discovery, unknown task or
stage, or reconciliation discrepancy. Filesystem failures may raise `OSError`.
JSON decoding is strict UTF-8 without a BOM; invalid bytes/BOM fail as ValueError.
Python integer fields require type(value) is int: reject booleans and integral floats.
No SDK exception escapes these standard-library interfaces. Messages identify the
file/task/field and cause; wording is diagnostic, not a new pinned error vocabulary.
Integer fields reject booleans. Unsupported versions and unknown keys fail.

Prepared commands in 002 (validate/context are read-only; convert writes only disposable output):

```sh
python3 tools/proofs.py validate --policy PATH --records-dir DIR
python3 tools/proofs.py context TASK-ID --policy PATH --records-dir DIR --stage tests --compact
python3 tools/proofs.py convert --source PATH --source-root ROOT --identities PATH --archive-commit FULL-SHA --output-dir NEW-DIR
```

Before 004, validate/context require both explicit options (no fallback to canonical
proof or state); after 004 their defaults become the canonical paths. Success prints
indented UTF-8 JSON (`indent=2`, `ensure_ascii=False`, newline), with stable insertion
order. Validate output is exactly `{"version": 1, "record_count": N}`. Context output
is below. Convert output is exactly `{"version": 1, "record_count": N}` after successful
output preparation. Validation errors return 1, empty stdout, `Proof error: ...` on
stderr; argparse usage errors retain exit 2. No live verification or approvals follow
from success. SDK admission/setup failure is never intended assertion red.

Discovery requires a real nonsymlink directory and policy regular file, rejects
symlink components in all ancestors of these supplied paths, including absolute paths, and enumerates immediate children in
filename order. Every child must be a nonsymlink regular canonical `TASK-ID.json`;
reject subdirectories, non-JSON/hidden files and all special files rather than ignore
possible proof. No recursive traversal. Filename and record task_id must agree;
IDs and `(repository, number)` pairs are unique. Required-record IDs and discovered
IDs are equal sets, including supplied exempt records. A valid empty directory with
an explicit valid empty required_records is permitted; missing input is not empty
success. The production frozen exemption list is never inferred from that emptiness.

## Closed JSON shapes

All objects below have `additionalProperties: false` except verification's explicitly
named observation map. All stated fields are required unless marked optional. No null
stands for an absent optional field except historical review and publication merge
fields as specified. Arrays retain original order; do not sort evidence/notes/reviews.
ID is ASCII `^[A-Z][A-Z0-9]*-[0-9]{3}$`; revision is lowercase 40-hex. Strings described
as nonempty must contain non-whitespace text. Local reference strings use canonical
lexical paths: slash separated, nonempty components, no empty/dot/parent component,
leading slash, backslash, colon or NUL. No filesystem resolution in record validation.
HTTPS references use the current `progress.evidence_path` URL checks (host required,
no credentials/whitespace, valid port), factored for tree-free reuse. A brief is a
lexical local reference or immutable archive URL of the form
`https://github.com/Kernol12/subathon-bot-ai/blob/FULL-SHA/PATH`; URL path uses the same
lexical safety after URL decoding. Archive brief URLs reject queries/fragments,
encoded slash/backslash separators, double encoding and noncanonical percent escapes;
use ordinary unescaped safe ASCII path components for these immutable brief links. Evidence permits local lexical or valid HTTPS
references. Historical paths/URLs do not require today's files or a network read.

| Object | Fields and constraints |
| --- | --- |
| proof | `version: 1`, `task_id: ID`, `issue: issue`, `kind: kind`, `brief: reference`, `evidence: string[]`, `limitations: string[]`; optional `workflow`, `publication`, `verification`, `review`, `supplemental_workflow_evidence` |
| issue | `repository: "Kernol12/subathon-bot-ai"`, `number: positive integer`; URL derived as `https://github.com/{repository}/issues/{number}`, never stored |
| policy | `version: 1`, `legacy_done: ID[]`, `legacy_done_sha256: lowercase 64-hex`, `archive_commit: revision`, `required_records: ID[]` |
| workflow | `required_stages: nonempty unique stage-name[]`, `stages: object` with optional `contract`, `tests`, `implementation` packets only; started stages subset of required_stages |
| packet | `authors: nonempty unique nonempty-string[]`, `revision`, `paths: nonempty unique lexical-path[]`, `reviews: receipt[]` |
| receipt | `phase`, `provider`, `model: nonempty string`, `reviewer: nonempty string`, `revision`, `outcome`, `evidence: HTTPS`, `findings: unique nonempty-string[]`, `blocking_findings: unique nonempty-string[]`; optional paired `started_at`, `finished_at` |
| publication | `state: open or merged`, `pr: HTTPS`, `head: revision`, `merge_commit: revision or null`, `merged_at: aware instant or null`, `observed_at: aware instant` |
| verification | arbitrary string keys, each value one of `blocked`, `draft`, `failed`, `not_run`, `observed_red`, `partial`, `passed`; exact original map, including empty map |
| review | null or object; optional `author`, `reviewer`, `evidence`, `baseline`, `revision`, `scope` strings and `outcome: approved or changes_requested`; exact original object, never promoted to structured receipts |

Kind is one of `code_review`, `cleanup`, `contract`, `environment`, `feasibility`,
`implementation`, `scaffold`, `test_baseline`, `test_draft`, `test_review`; epics
have no proof record. Evidence/limitations arrays may be empty and may contain repeated original strings;
evidence entries must each be nonempty valid references. Only limitations entries
may be empty strings.
Limitations have no aggregate 500-character cap: copy the entire notes array verbatim,
or [] if absent. Retain optional historical fields even when null/empty; do not
normalize or “repair” them. New forward observations belong in canonical workflow/PR
receipts, not edits to frozen historical fields.

Policy required_records is lexically sorted and unique. legacy_done retains the
original ordered list, is unique and must equal the original frozen 120-member set;
its checksum is SHA256 of UTF-8 sorted IDs joined by LF with a final LF. Require
`04afa2e0c32d4d88183e8b104f9d65402c81bd4e32521406c3d36a98e881e573`
as well as recomputation. The implementation retains the explicit frozen ID tuple
as proof-policy constants after tracker retirement. Tests keep an independent literal
ID list under tests/support/frozen_proof_policy.py with this pinned checksum; no
fixture is generated from implementation or a removed state file. Reordering does not change membership but ordinary later
publication freezes the original list too. No “old”, “done” or absent-proof exemption.

### APITEST-016 supplemental historical shape

Only `APITEST-016` admits `supplemental_workflow_evidence`. Preserve its entire object,
not just the generic workflow projection, including rejected receipts, native_session,
artifact/result tuples, caveats, execution_history and completion/compatibility reviews.
The exact shape baseline is the value at
`progress/state.json`, task `id == APITEST-016`, field `supplemental_workflow_evidence`
at `b50643412a4bdcbb6ee71facd43e0295220f2d0a`.

To specify this large closed historical shape without copying a second proof object:
for every baseline object, admit exactly its keys, all required, recursively closed;
for every baseline array, admit exactly its length and positional element shapes
(Draft prefixItems, minItems=maxItems, items=false); for each leaf admit its JSON
primitive type (integer distinct from boolean, null only at null leaves). Top-level
keys are exactly `format`, `complete_workflow`, `complete_workflow_sha256`,
`digest_convention`, `standard_projection_sha256`, `authority`, `reason`,
`compatibility_reviews`. `complete_workflow` has exactly `required_stages`, `stages`,
`completion_reviews`. This recursive rule defines every nested allowed key and type;
no arbitrary-object escape hatch or generic forward extension grammar. Ship its
structural schema inside proof.schema.json, with no duplicated receipt scalar values.
The one-time conversion and later publication guard additionally require exact
historical value equality against their pinned source; structural validation alone
cannot authenticate history. This is preservation, not a new digest: retain original
hashes/conventions as uninterpreted historical values, never recompute approvals or
make supplemental content satisfy canonical stage minima.

## One workflow grammar, two path boundaries

Reuse `workflow.validate_stage`, `stage_complete`, `stage_context` and publication
semantics; factor shared kind minima/path-independent grammar in workflow.py instead
of constructing fake status-bearing task objects or implementing another review engine.
Keep the existing validate_stage call signature/default behavior for production callers;
a clearly named keyword-only tree-free mode may suppress filesystem checks for proof
loading only. In that mode lexical checks and bookkeeping exclusions still run.
`progress.validate` and `workflow.validate_task` must keep their existing strict default:
missing current brief/evidence/stage files still fail, as do done acceptance/baseline,
separate-review, publication, dependency and claim checks. Do not route production
state through proof exemption rules or weaken its baseline assertions.

The shared grammar remains exactly: phase initial/verification/closing requires
claude/codex/claude respectively; first receipt is initial and no later initial;
reviewer differs from every declared author; blockers subset of findings; outcome
changes_requested iff blockers nonempty, otherwise approved. Closing immediately
follows approved verification on the same revision. Optional observed timezone-aware
ISO times occur as a pair with finish >= start, using current workflow.instant rules.
No fabricated distinct-human identity or authenticated approval claim. Packet completion
is the last approved closing plus its immediately preceding approved verification,
both on packet.revision. Earlier initial may bind an earlier revision. A stale final
pair makes a packet pending, not invalid historical grammar. Rejected closing may be
retained but requires a new approved verification/closing pair to become complete.

`require_complete` revalidates the entire bundle and fails with task/stage diagnostics
unless the selected task has workflow satisfying every required stage via the existing
workflow.stage_complete predicate. Missing workflow, absent required packets or stale/
pending final approvals refuse, including for supplied exempt records; historical
exemption is not permission to claim new completion. It returns None on success,
without Git/source/PR authentication. This is the 002 operation satisfying stale-proof
rejection in issue627; current file binding and actual final merge remain003/coordinator
responsibilities. A valid historical pending packet may load and show complete:false
but require_complete must refuse it. Tests distinguish these operations explicitly.

Kind minima: contract → contract; test_draft/test_review/test_baseline → tests;
implementation → tests and implementation; all other admitted kinds → implementation.
Any explicit extra required stage remains required. Optional workflow absence can
represent historical/publication-only proof; it never implies completion. Supplied
workflow must satisfy minima even for exemptions. Unstarted required stages are absent
from stages, not fake empty authors/revisions. Pending stages pass structural loading
and block final completion. New claimed non-exempt work must have workflow naming
minima before dispatch; source conversion enforces this where claim is observable.
Loader without board state cannot detect an unrecorded claim.

Publication semantic validation retains current rules: pr in evidence; open has null
merge fields; merged has full merge revision and merge time <= observed_at. Both times
aware. An old merged observation never closes a new packet or authenticates a merge.
Only later explicit `--complete` checks every required current stage; actual final PR
merge/readback remains separate.

Bookkeeping excludes state/HTML/checkpoint, proof record files and proof-policy.json
from packet paths in both strict and tree-free modes, using one shared predicate/
constant; no meaningful existing assertion is removed. 003 selected-stage publication still resolves full Git commits,
checks contained regular files and literal Git mode/blob equality at revision and
HEAD; retired paths load tree-free but cannot be published as current. Policy enrollment
never changes packet currency. Changes to schema/loader/guard behavior require their
own substantive bound review; bookkeeping exclusion cannot hide code/policy changes.

## Context projections

Full context is exactly `{"proof": record, "required_stages": names,
"limitations": warning}` plus `stage_context` when requested. names is workflow's
required_stages or [] without workflow. Full proof includes optional historical fields;
selected stage_context is workflow.stage_context, adapting task_id to its error/identity
input only, without exposing a fabricated task or board state.

Compact proof retains only version/task_id/issue/kind/brief/evidence/limitations and
optional publication, verification and review; required_stages remains top-level.
Failed/not_run historical verification stays visible rather than silently omitted. With --stage, stage_context
has exactly the existing progress.compact_stage_context projection: name, complete,
next_gate, packet (authors/revision/paths or null), ordered review_refs (phase/provider/
model/reviewer/revision/outcome/evidence), historical_blocking_findings and metrics
(review_count/elapsed_seconds/timed_reviews). Factor this projector for reuse; do not
fork metrics. No selected stage means no packet/metrics, not an arbitrary default.

Warning text: `Recorded information only; Git contents, live GitHub/CI, review identity
and authorization are not verified.` Compact appends ` Compact view omits complete
workflow, other stages, supplemental history, non-blocking finding details and
closing-only finding metrics; use full mode or review links.` Neither context shows
availability/dependencies/status/owner or fetches an unenrolled planning issue.

## Disposable one-time conversion

identities is a closed object `{"version": 1, "entries": [...]}`; each entry exactly
`{"task_id": ID, "issue": issue}`. Mapping covers every source task including epics,
with equal ID sets, no duplicates or duplicate issue identity. No title matching,
network lookup, default issue number or implicit unmapped exclusion. Board-only
additions are reconciled explicitly in 004, not invented by this converter.

Source is a disposable copy of tracker v1. Validate its structural/current workflow
invariants using existing source validation against an explicitly supplied source root
at CLI (`--source-root ROOT` is required), preserving current file requirements there.
Python convert validates consumed source shapes/enrollment/grammar tree-free; CLI must run the
strict source validator before calling it, supplying [] for an absent notes field
in a validation copy only. Real paths are not rewritten to make it pass.
The Python convert API permits a bounded disposable source containing only selected
synthetic tasks while retaining the full frozen policy. It does not require absent
frozen members to be present as done tasks. It validates present tasks, identity-set
equality, consumed fields, kind/claim/workflow invariants and exact policy; source
without workflow_policy refuses. This is solely converter input validation, not a
new authoritative task validator or a relaxation of progress.validate. CLI strict
source admission still requires the complete source inventory. --source-root may be
the repository root for read-only resolution; source itself must be a separate copy.
Absent notes alone means []; other malformed consumed inputs fail. No production write path.

Examples 1–2 below exercise this bounded Python API, not a CLI full-source claim.
For one positive CLI case use a disposable copy of current real tracker source and
its current read-only root with a generated, explicitly synthetic/non-authoritative
one-to-one mapping (e.g. fixture ordinals). Do not claim those fixture numbers are
real issues, publish outputs, or commit a hundreds-record fixture. Expected per-field
preservation is derived from input identity/copy rules, not observed converter output.
Only this one whole-inventory orchestration case uses current tracker input; existing
strict-source assertions remain independent. 004 must explicitly review retirement/
adaptation of this temporary CLI case and converter as tracker consumers; no surviving
offline CI test may fetch archived history or depend on a removed ledger.

Enroll each non-epic with any supplied workflow/publication/review/supplemental field
(non-null review; empty review object still counts), nonempty evidence, any verification
value outside not_run/draft, or a claim outside the frozen exemption. A claim is status
in_progress/in_review/done; blocked/ready alone is not an inferred claim. Exempt tasks
with evidence still enroll. Epics remain archive-only. A non-exempt claimed task missing
workflow is a discrepancy, never automatically authored proof. Preserve exact kind,
brief, evidence, optional historical objects and all review order/authors/revisions;
limitations = notes verbatim. Do not strip rejected receipts or upgrade failed/not_run.
Archive every original source task/order/title/relationships/activity; the converter
output is not a replacement archive. Its policy uses the explicitly supplied archive
commit and frozen source exemption list, never a guessed future revision.

CLI source/mapping/root/output paths are explicit, with no defaults; refuse production
state as source and any output within the repository. Inputs may be read from a
caller-created disposable copy; reject symlinks, overlapping inputs/output and existing
output directory. Prepare/validate all outputs in a private sibling temporary directory
then publish into the new destination without replacing any existing path; on failure no success report/destination, clean
own temporary files. Never overwrite existing data. No race-free or hostile-concurrent-writer guarantee
is claimed for this trusted one-time tool. Reserve NEW-DIR with exclusive mkdir before
publication; if it already exists (even empty), fail. Move prepared child files into
that owned directory, removing only owned output on failure; never rename over an
existing destination or follow a symlink. Inputs/root/output check every ancestor
for symlinks, as discovery does. Write proof-policy.json,
proofs/TASK-ID.json and report.json only. All files use the same indent=2,
ensure_ascii=False, final-LF serialization as stdout. Proof keys follow table order
with optional fields appended in table order; policy keys follow table order; report
keys follow its declaration below. Original nested object/array order is preserved. No sync/reconcile/approval command, no Git,
network, production state/HTML writes, archive publication or issue side effects.

Report is a closed object: version1, source_task_count, record_count, archive_only_ids
(sorted), enrolled_ids (sorted), mappings (entries in source task order),
field_comparisons (one per enrolled task: task_id and boolean equality results for
kind/brief/evidence/limitations/workflow/publication/verification/review/
supplemental_workflow_evidence, treating absence distinctly), discrepancies (string[]).
Successful comparisons all true, discrepancies empty; otherwise refuse rather than
emit partial success. Report does not contain planning state or private paths. 004
adds the approved full reconciliation/archive report, canonical input/output digests,
remote verification, relationship/title mapping and discrepancy resolutions required
by the parent contract; conversion here neither performs nor claims that audit.

## Base-absent bootstrap and later publication (003/004)

These are normative inputs to 003 tests, not 002 guard implementation. Normal guard
freezes archive_commit, exemption list/fingerprint, identities/kinds; refuses shrink,
record deletion, required-stage shrink, canonical review-history non-prefix edits,
limitations non-prefix edits and any changes to historical verification/review/
supplemental objects. Additions require matching record plus archived identity/kind
or genuinely new per-ID reviewed brief (Task ID, canonical Work type, Issue stanzas).
A new brief must be in the selected reviewed packet. No skip or intent override.

Before either guard mode, 003 requires --base to resolve to the local origin/main
commit, and that commit to be an ancestor of candidate HEAD. Arbitrary older ancestor
bases refuse even if they have no policy. Immediately before accepting the receipt
and protected merge the coordinator fetches/reads current target main and verifies
it equals that receipt base; if target moved, update/reconcile and rerun. This is a
mandatory operational gate: offline code cannot authenticate remote-tip freshness.
Synthetic Git fixtures explicitly create origin/main; an older --base test must fail
before bootstrap. A stale local origin/main cannot be claimed current without the
coordinator check. Thus normal or bootstrap mode cannot erase newer target enrollment
by choosing an older endpoint. Within an unpublished outgoing range, transient
add/remove edits are not accepted target history; all outgoing privacy/source checks
still run. Preservation is relative to current accepted target, not arbitrary ancestry.

One legitimate base-absent case is initial policy introduction at cutover (and its
explicit disposable rehearsal in 003): base has neither policy nor records. Absence
of only one is corruption. Do not infer success from base absence. Candidate must
pass all schema/grammar/set checks and the exact frozen fingerprint. Guard must have
available full archive_commit, the source state and complete reconciled ID/issue/kind
mapping appendix pinned at that archive, plus coordinator evidence of remote archive
verification required by 004. Guard does not authenticate that evidence or fetch it.
Unavailable archive/source/mapping is a setup failure.

Derive the exact extracted enrollment from archived source using the converter
predicate; compare every extracted candidate identity/kind, original limitations,
evidence, optional historical objects, required stages and ordered reviews against
archive, not just a sample or checksum. Extra candidate enrollment must pass the
new-ID reviewed-brief rule, or archived-ID matching provenance. Archived stage authors/
revision/paths and history are preserved; forward continuation may append receipts,
append limitations and honestly update canonical packet authors/revision/paths without
shrinking required stages. No extra exemption and no rewriting archived history.
Report bootstrap checked and counts; do not report an ordinary base-policy comparison.
In 003 this operates only on disposable explicit inputs; production still uses state.
After bootstrap every ordinary guard uses the existing-base rules; a deleted base
policy cannot bootstrap again while proof records exist. Shallow CI checks candidate
shape/set/grammar, never pretends to have performed this full-history guard.

Before 004 rollback discards disposable output and leaves state authoritative.
After 004 pause dispatch, use pinned archive/proof and actual merged receipts; never
restore old state as a parallel authority or silently lose subsequent proof. Full
remote publication/readback, drift checks and reviewed rollback remain coordinator
obligations under the parent contract.

## Hand-derived bounded examples and expected outcomes

These are authored oracles, not converter observations. Use the real frozen policy
for synthetic proof/policy and bounded Python conversion tests without adding/removing exemptions; synthetic IDs below are outside
it. No hundreds of production record fixtures.

1. Disposable source CONVERT-901: kind contract, planned, evidence [], verification
   {acceptance: not_run}, review null, notes ["Only synthetic checks."]; mapping issue
   9901. No record: unclaimed with no supplied proof. Change evidence to
   ["https://example.test/receipt"]: exactly one record with limitations
   ["Only synthetic checks."], same evidence/verification/review, no workflow;
   required_records ["CONVERT-901"]. Loading passes, context has names [], requesting
   contract stage refuses, require_complete refuses. Changing notes to two equal
   strings preserves both in order. Epics with the same evidence remain archive-only.
2. CONVERT-902: implementation, in_progress, required_stages [tests, implementation],
   stages {}; notes ["Baseline observed; implementation pending."]; verification
   {baseline: observed_red, github_ci: failed}. Copy those exact fields and caveats;
   loading passes, tests context packet null/next_gate authoring/complete false;
   require_complete refuses. Remove workflow: conversion refuses claimed missing proof.
3. Contract packet uses authors ["codex:synthetic-author"], revision forty `b`s,
   paths ["docs/retired-synthetic.md"]. Ordered initial Claude at forty `a`s, approved
   Codex verification at forty `b`s, rejected Claude closing at forty `b`s with
   findings/blockers ["SYN-C1"], then approved independent Codex verification and
   approved Claude closing at forty `b`s. All model/reviewer/evidence fields nonempty
   and distinct from author; each report HTTPS. Grammar loads tree-free, complete
   true, historical blockers [SYN-C1], review_count 5, elapsed_seconds null,
   timed_reviews 0. Rejected report stays receipt 3. Without last two receipts it
   loads pending; a closing immediately after the rejected closing is invalid.
   Existing file absence remains invalid through production progress.validate;
   later current publication refuses this retired path. Changing only one final receipt revision makes grammar invalid; changing both
   to forty `a`s preserves grammar but makes completion false and require_complete
   refuses. The publication sentence is a 003 oracle, not 002 execution evidence.
4. Required_records [CONVERT-901] with empty directory refuses; extra CONVERT-902
   refuses until explicit enrollment. A malformed supplied frozen-exempt record also
   refuses. Duplicate JSON keys, issue 9901 reused, boolean version, unknown field,
   dot/parent path, symlink or nested file refuse. Original self-review fails.
5. **003 guard oracles, not 002 execution:** Base absent plus missing archive mapping refuses, even with matching fingerprint.
   Verified available archive/mapping plus every extracted entry exactly preserved
   passes bootstrap; dropping one enrolled historical-evidence record refuses.
   Later unrelated additive enrollment leaves an unchanged stage's mode/blob binding
   current; changing kind/shrinking history refuses regardless of packet approvals.

Existing meaningful workflow/context, real-Git publication and actual-SDK tests are
owning regression oracles. Preserve especially the production missing-stage-file
assertion, review cycles, exact compact output, outgoing side-history/private paths,
checkout identity, SDK-invalid-but-semantically-valid state and empty-success witness
refusals. 003 defines a separate proof schema driver target for exactly one aggregate
proof/policy test and retains exact success bytes
`{"tests":1,"failures":0,"errors":0,"skipped":0}\n`; no vacuous empty suite, skipped case
or interpreter exit-zero substitute. Schema validation is shape validation, not
workflow/identity/source authentication. A stale historical packet may load pending;
current completion/publication refuses it. No test/assertion deletion is authorized.

## Validator responsibilities and authored-red setup

Semantic acceptance implies schema validity. Both schemas and Python reject closed
shape/required-key/type/enum/pattern violations. Draft integer permits 1.0 by standard;
Python deliberately rejects integral floats too, as a narrower runtime input contract.
Schema is necessary, never sufficient. Python-only checks: frozen-list checksum/set,
required/discovered ID equality, filename/issue uniqueness, decoded path/URL/valid-port
rules, stage-kind minima/subset and review order/independence/blocker/outcome/timing,
publication PR membership/time order, completion currency, and no mutable aliasing.
Schema may encode simple conditional shapes, but cannot replace those semantic gates.
All positive authored examples must pass both where their shape is relevant. Shape/
unknown-key/enum/raw-path negative cases require both validators to refuse. Duplicate
JSON keys/invalid UTF-8/BOM/nonfinite values fail read_json before schema validation;
symlinks/discovery fail load; bool integers fail both, integral floats fail Python.
Stale final approvals fail require_complete (context reports pending); self-review or
invalid order fails semantic load; required missing record fails bundle validation.
Invalid decoded URL/port and cross-field negatives are semantic refusals; do not pin
a schema refusal for constraints it cannot express. Example5 belongs 003 only.

Test author owns additive tests/tooling/test_proofs.py, the independent frozen ID
constant in tests/support/, and a separate tests/schema_validation/test_proof_schemas.py
with its own metadata admission before SDK imports. Do not add candidate proof schema
loading to RepositorySchemas.setUpClass or its production one-test witness. During002
run the new schema file directly with the already admitted interpreter -I -S -B;
003 adds its CMake/CI registration. Existing schema.repository/schema.publication and
the progress witness remain unchanged in002 and must still pass during authored red.
The separate file validates schemas/hand-derived examples, including exactly one
aggregate method later selectable by003's one-test schema witness. It independently
validates the pinned APITEST-016 value with manual mutations: extra/missing key,
leaf type, array length and integer→boolean. Do not derive expected outcomes from
any schema generator. Preserve its frozen raw input in one reviewed historical
support artifact when004 removes state; no generated suite of per-task fixtures.

After this contract's full review, the test author may supply loadable tools/proofs.py
NI definitions with exactly the public signatures above; each new entry point raises
ValueError('PROOF_RECORDS_NOT_IMPLEMENTED'). CLI parses all stated options/subcommands
then emits the named exit1 NI with empty stdout; argparse misuse stays exit2. The
same author may add keyword-only tree-free mode to workflow.validate_stage defaulting
False; True initially raises that named NI after shared argument validation. Strict
False remains unchanged. No production progress call uses the new mode yet. New
schema files are valid Draft schemas {"$schema":"https://json-schema.org/draft/2020-12/schema",
"not":{}}: loaded successfully, they reject every instance by schema assertion. This
is a deliberate meaningful red seam, never a missing-schema/import/dependency error.
Positive loader/converter/context/completion assertions must fail at the named NI;
positive actual-schema assertions fail ValidationError from the valid reject-all stub.
Negative tests alone passing on NI establish nothing. Test review precedes execution;
then record intended positive-assertion red distinctly from setup failure. Fresh
implementer replaces these explicit seams and schemas only after red is observed.

The historical MIGRATE-001 packet includes the002 brief at its completed source;
this packet's brief-link edit does not alter that history. Never republish001 as a
current packet or rewrite its source bindings to follow this edit.

This packet is authored only. Independent full contract review precedes test
authorship; test review precedes loadable NI-seam assertion red and fresh implementation.
No runtime, baseline red, review approval, publication or live/platform result is claimed.
