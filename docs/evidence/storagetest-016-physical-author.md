# STORAGETEST-016 physical DOCUMENT author

Author: bundle_implementer, 2026-11-03. Explicit physical-document-only assignment
after concrete approvalc0ed4b6, importedc3c5568. Fresh isolated
`task/STORAGETEST-016-physical-author` starts from actual mainaad1ad11 and is
fast-forwarded to approved root77c6def21adab177bb7f4157207149f5b0fb61e6 before edits.
Root owns migration order/shared metadata/build/config/publication. Own exactly
the new physical-contract Markdown and this note; no production schema/API is added.

Initial actual candidate commit `8b363ed0904662fdb9320e1e5879e89d7a8fdbbf`, document
blob `972e67a6ab37de927e41582d197783581cf0393c`,464 lines; SHA256
`82de5cc6aadd142c1aa6356893a5c44aae90967f30c6e95ebfaaac789e5c7eaf`.
Authority is full approved concrete701c710f/reviewc0/authorc14f, service6d0cd67f,
ADR0003, specificationv0.45 NUM-02/04/DB-01, and accepted migration/retention/
receipt/contribution/plan/bundle physical boundaries. Actual public types/readers
and private bundle decoders were inspected to avoid inventing a historical getter
or pretending an owning legacy transaction is a harmless borrowed read.

## Concrete delta and invariant reasons

Proposed step6 ID `durable-application-holds-v1`, format1/storeidentity unchanged,
starts0–6. Twelve exact statements add ten empty tables, eleven NULL autoindexes
and two nonunique query indexes:23 new objects/full89. No ALTER/trigger/seed/
allocator/transaction control or catalog mutation is introduced. Old prefix1–5's
IDs, SQL, full7/13/20/30/66 projections, payloads and hashes remain exact.

- H01/H03: global tagged-subject index selects ALL anchors before status; each
 hold has its independent head, retaining simultaneous reasons and CID0/MAX.
- H02/H05: one raw mutation key across BOTH stored kinds; actual-kind full closure
 before equality/conflict; exact retry selects historical produced versions.
 Per-hold lifetime version uniqueness/checked contiguous positions prohibit ABA,
 reopen-on-retry, silent cause overwrite or sparse fake exhaustion.
- H04/H06: complete expected active locators, immutable entries, old/new version
 and replacement detection links permit subset resolution or atomic replacement;
 heads move last. Closing with an application link requires all active holds closed
 and no replacement. Crosslinks use finite selected payloads/historical locators,
 never recursive expansion through sibling current HoldViews or whole lifetimes.
- H07–H09: captured versus failed-capture matrix, explicit extent/stopped_for,
 bounded canonical Exact and nullable finalized values retain overflow honestly.
 No valid PreparedBundle/request, plan0 or finalized0 is required to store failure.
 Main0/debt0 with MIN attempts9223372036854775808; Main1 leaves representableMAX.
 Storage records representation/equality, not numerical producer or authority proof.
- H10/H11: timer RequireReview is distinct from overflow. Supplied composition/
 decision annotations and complete application links are DATA only. The held
 original/remaining selector stays separate from a distinct correction key/source;
 old processed claim cannot be replayed. No unresolved mixed/postfinish/continuation
 policy is selected by a physical column or successful storage result.
- H12–H14: exact selected subject COUNT/type/domain summaries, unfiltered initial
 pages and64/65 metadata history validate hidden zero/negative/malformed rows,
 missing endpoints, complete parent/status/producer chains and head/count agreement
 before classification/allocation. Metadata does not acquire result/lifetime caps;
 complete active result cannot truncate257 into an apparently clear256.
- H15/H16: approved defaults/preflight/error priority and cumulative writer budgets
 remain binding. Pure malformed input does no SQL/cleanup; known selected failures
 establish actual whole-unit rollback; original capability/protocol/unknown survives.
 No own commit/destructor cleanup, normal-payload shortcut or premature durability.

Source mapping explicitly separates actual event receipt from original support
receipt/CID, current contribution dimensions from immutable anchors, and historical
observation state from today's head/configuration. Present prior PlanKey in
FailedCapture gets complete selected source/body/history closure without declaring
that the failed proposal became that revision. Absent plan/head remains unknown.
Whole-input application linkage preserves the actual new source/proposed plan and
same tagged subject while each historical held event remains immutable; a later
confirmation can differ from original support and earlier event on the same CID.
These exact crosspaths remain subject to actual physical review.

Immediate typed FKs protect existing namespace membership, parent/head/version/
entry and held-origin locators. Cyclic first-detection/produced-version ownership
is deliberately validated by runtime crosschecks rather than cyclic insert order.
All writes/statements/results are prepared before own staging; registry → new
causes/observations/occurrences/detections → produced versions → resolution rows →
heads satisfies immediate FKs. Real post-write failure must roll back the whole unit.

## Actual proposal diagnostics and static checks

System Python3.14.8/stdlib SQLite3.54.0, Linux x86-64. No Qt/native build or test,
new port invocation, baseline fixture authoring, intended red, provider or live
database action ran. Scratch database and scripts exist ONLY under `/tmp` and
ignored local evidence. These are document grammar/catalog/framing diagnostics,
not hold behavior acceptance or independent reviewer approval.

- `python3 /tmp/holds016-physical-proposal.py`: PASS. Independently authored SQL
 and manual10-table/11-autoindex/2-index inventory plus frozen prefix66 are applied
 to `/tmp/holds016-physical-proposal.sqlite`. Actual89 raw tuples/CREATE text/NULLs
 equal the expected projection;10 column counts match; foreign_key_check empty;
 integrity_check ok. The proposed document's12 literal SQL lines equal those
 diagnostic statements exactly. An initial scratch reader used `tbl_name` instead
 of the frozen JSON's `table_name`; fixed before successful diagnostics, never red.
- Independent stdlib F framing/hashlib reproduces EACH unchanged prefix1–5's
 literal payload bytes, full payload_hex and SHA256, with sizes2000/10535/18126/
 29015/62465 and projections7/13/20/30/66. No production factory/codec generates
 the expectation. Current corrected step6 framing is **89890 bytes**, SHA256
 `9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a`.
- `python3 tools/check_project.py`: PASS396 task records/122 decisions, links,
 presets/current HTML. `python3 tools/progress.py render --check`: current.
 `git diff --check`: PASS. Only the two assigned Markdown paths change.

Ignored `local-evidence/storagetest016-physical/` retains the proposal script and
report. Corrected SHA256 script48cf978690b59426efbb2fc4eea6b267233a8f4c171498444a97d40ac83a9ffe;
report2597760f05e551be22380d03ba5733ee9ba517adf424a487a36e603e36b294b6.
The report independently pins statement lengths4696,8804,1248,1284,795,1280,1665,
468,504,1559,96,97 and columns22,45,10,11,6,10,8,3,3,8. Its numbers refer to this
actual corrected physical candidate; earlier89396-byte draft and initial89806-byte
review candidate are superseded, not current payloads.

Different actual full physical-document review is NEXT. Minimal API/result-owned
byte formulas and schema artifacts, named stubs, independent frozen foundation,
selected official green, feature tests/review/compiler/discovery and FIRST intended
red are later separately assigned gates. STORAGE-016 production is not authorized
by this document diagnostic; service004 keeps its own prerequisite/baseline/red.
No owner policy decision or scope waiver is requested. All integrations/FX,
authority/recovery/financial/UI/platform/backup/live/release obligations remain.

## Actual independent-review corrections

Reviewer found a reproducible Statement1 SQL three-valued-logic hole: absent
application_link_kind with a complete held-origin group could evaluate CHECK as
NULL and pass despite its intended absent-link shape. The non-NULL alternatives
now explicitly require application_link_kind IS NOT NULL. Actual-kind shared raw
mutation uniqueness, all valid absent/whole/timer alternatives and all23 objects
remain; only42 bytes are added to Statement1, and its table SQL contributes that
same42-byte change to the framed projection. Thus payload grows84 bytes to89890.
Reviewer initial raw evidence and old89806/e094 framing remain historical. Initial
script/report bytes are archived with -initial names and their original recorded
SHA33412829/44196b9f; a byte reconstruction was checked against those exact hashes
after the current diagnostic files were refreshed.

Corrected fullDDL/catalog/FK/integrity/projection diagnostic passes again. A raw
stdlib SQL predicate diagnostic checks eight declared guard cases: absent/whole/
timer and optional known remaining fit; absent-with-held, absent-with-remaining,
whole-with-held and timer-without-held evaluate exactly0, not SQLNULL. This scalar
diagnostic is not full hold data/API/Qt acceptance. Raw null-guard-diagnostic.json
SHA256182914237f0c2df316cbdcd8cf594bd2f2a1b4b256667553bb726861bb543e2d.

The other reviewed correction explicitly requires ACTUAL unfiltered typed COUNT/
domain/ordinal/key proof against every declaration BEFORE allocation/reserve/full
materialization of occurrences, resolution expected/entry/replacement collections
and active views. All subject metadata/history is walked before active counting/
full loading; understated counts are invalid_stored_data and cannot bypass caps.
Metadata still has no lifetime materialization quota. No API/source/schema artifact,
fixture/test assertion or product policy changed; actual revised review remains
pending.

A third actual review finding identified an H11 composition bypass: kind0 same-
subject/source/plan could otherwise point at the already-processed Beside timer's
old original and avoid kind1's distinct-correction guard. Revised crosspaths pin
kind0 to whole-input numeric/unresolved and explicit DeferWholeInput timer causes;
ANY closed Beside timer forbids kind0. Application-linked Beside resolution requires
kind1 and the exact held origin/distinct key/source, while no-app decisions remain
DATA under owning policy. A Defer timer's complete-original kind0 path remains;
no distinct-correction or partial/full continuation policy is inferred from a
shape-valid kind1 declaration. This adds explicit runtime linkage validation, no
DDL/payload change or new product execution policy.
