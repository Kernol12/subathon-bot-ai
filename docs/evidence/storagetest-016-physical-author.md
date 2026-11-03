# STORAGETEST-016 physical DOCUMENT author

Author: bundle_implementer, 2026-11-03. Explicit physical-document-only assignment
after concrete approvalc0ed4b6, importedc3c5568. Fresh isolated
`task/STORAGETEST-016-physical-author` starts from actual mainaad1ad11 and is
fast-forwarded to approved root77c6def21adab177bb7f4157207149f5b0fb61e6 before edits.
Root owns migration order/shared metadata/build/config/publication. Own exactly
the new physical-contract Markdown and this note; no production schema/API is added.

Actual candidate commit `8b363ed0904662fdb9320e1e5879e89d7a8fdbbf`, document
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
 the expectation. Step6 framing is **89806 bytes**, SHA256
 `e09430872e5102cddc84270110ac95886cb54277960e45c0613fa71168652fcd`.
- `python3 tools/check_project.py`: PASS396 task records/122 decisions, links,
 presets/current HTML. `python3 tools/progress.py render --check`: current.
 `git diff --check`: PASS. Only the two assigned Markdown paths change.

Ignored `local-evidence/storagetest016-physical/` retains the proposal script and
report. SHA256 script334128295b6db67a14115f531ee6bd686e11e4883763224d56e5b72860ae97fc;
report44196b9f8eea7f18f6894fbf73a1009739f6b3afb4cd3b6c631d7b9e8f24010d.
The report independently pins statement lengths4654,8804,1248,1284,795,1280,1665,
468,504,1559,96,97 and columns22,45,10,11,6,10,8,3,3,8. Its numbers refer to this
actual final physical candidate; earlier89396-byte scratch status is superseded.

Different actual full physical-document review is NEXT. Minimal API/result-owned
byte formulas and schema artifacts, named stubs, independent frozen foundation,
selected official green, feature tests/review/compiler/discovery and FIRST intended
red are later separately assigned gates. STORAGE-016 production is not authorized
by this document diagnostic; service004 keeps its own prerequisite/baseline/red.
No owner policy decision or scope waiver is requested. All integrations/FX,
authority/recovery/financial/UI/platform/backup/live/release obligations remain.
