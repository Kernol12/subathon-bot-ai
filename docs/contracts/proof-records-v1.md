# Proof records v1 — MIGRATE-002

Authored contract/examples, specification v0.45 §§17–18.
Author: `codex:migrate002-contract-author-20261113:/root` (gpt-6.1-sol).
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
No SDK exception escapes these standard-library interfaces. Messages identify the
file/task/field and cause; wording is diagnostic, not a new pinned error vocabulary.
Integer fields reject booleans. Unsupported versions and unknown keys fail.

Prepared read-only commands in 002:

```sh
python3 tools/proofs.py validate --policy PATH --records-dir DIR
python3 tools/proofs.py context TASK-ID --policy PATH --records-dir DIR --stage tests --compact
python3 tools/proofs.py convert --source PATH --identities PATH --archive-commit FULL-SHA --output-dir NEW-DIR
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
symlink components in these supplied paths, and enumerates immediate children in
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
lexical safety after URL decoding. Evidence permits local lexical or valid HTTPS
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
have no proof record. Evidence/limitations may be empty and may contain repeated or
empty original strings (reference syntax still applies to each evidence entry).
Limitations have no aggregate 500-character cap: copy the entire notes array verbatim,
or [] if absent. Retain optional historical fields even when null/empty; do not
normalize or “repair” them. New forward observations belong in canonical workflow/PR
receipts, not edits to frozen historical fields.

Policy required_records is lexically sorted and unique. legacy_done retains the
original ordered list, is unique and must equal the original frozen 120-member set;
its checksum is SHA256 of UTF-8 sorted IDs joined by LF with a final LF. Require
`04afa2e0c32d4d88183e8b104f9d65402c81bd4e32521406c3d36a98e881e573`
as well as recomputation. Reordering does not change membership but ordinary later
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
from packet paths. 003 selected-stage publication still resolves full Git commits,
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
optional publication; required_stages remains top-level. With --stage, stage_context
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
Python convert validates source shapes/enrollment/grammar tree-free; CLI must run the
strict source validator before calling it, supplying [] for an absent notes field
in a validation copy only. Real paths are not rewritten to make it pass.
Absent notes alone means []; other malformed inputs fail. No production write path.

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
then rename into the new destination; on failure no success report/destination, clean
own temporary files. Never overwrite existing data. Write proof-policy.json,
proofs/TASK-ID.json and report.json only. No sync/reconcile/approval command, no Git,
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
for synthetic tests without adding/removing exemptions; synthetic IDs below are outside
it. No hundreds of production record fixtures.

1. Disposable source CONVERT-901: kind contract, planned, evidence [], verification
   {acceptance: not_run}, review null, notes ["Only synthetic checks."]; mapping issue
   9901. No record: unclaimed with no supplied proof. Change evidence to
   ["https://example.test/receipt"]: exactly one record with limitations
   ["Only synthetic checks."], same evidence/verification/review, no workflow;
   required_records ["CONVERT-901"]. Loading passes, context has names [], requesting
   contract stage refuses, final completion refuses. Changing notes to two equal
   strings preserves both in order. Epics with the same evidence remain archive-only.
2. CONVERT-902: implementation, in_progress, required_stages [tests, implementation],
   stages {}; notes ["Baseline observed; implementation pending."]; verification
   {baseline: observed_red, github_ci: failed}. Copy those exact fields and caveats;
   loading passes, tests context packet null/next_gate authoring/complete false;
   closure refuses. Remove workflow: conversion refuses claimed missing proof.
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
   later current publication refuses this retired path. Changing either final pair
   revision to forty `a`s makes completion false (or grammar invalid if pair differs).
4. Required_records [CONVERT-901] with empty directory refuses; extra CONVERT-902
   refuses until explicit enrollment. A malformed supplied frozen-exempt record also
   refuses. Duplicate JSON keys, issue 9901 reused, boolean version, unknown field,
   dot/parent path, symlink or nested file refuse. Original self-review fails.
5. Base absent plus missing archive mapping refuses, even with matching fingerprint.
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

This packet is authored only. Independent full contract review precedes test
authorship; test review precedes loadable NI-seam assertion red and fresh implementation.
No runtime, baseline red, review approval, publication or live/platform result is claimed.
