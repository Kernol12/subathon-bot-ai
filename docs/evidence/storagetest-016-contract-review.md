# STORAGETEST-016 — independent concrete hold DOCUMENT review

Reviewer: bundle_reviewer, 2026-11-03. **APPROVED DOCUMENT proposal** after actual
author correction0adfdec095cd6e879842bbcc688150cadad357c6, root import1c8ef5c.
Reviewed complete413-line contract701c710f83b0361a075c0d0bca7cf6ec07fb351e
and147-line author evidencec14f656728860b9d76d996128f7229a210802f87.
Original actual2ad07f7/b1f0a092 was reviewed first; five concrete findings below
were corrected before this approval. No prospective artifact is approved.

Fresh isolated task/STORAGETEST016-review begins from integrated main
aad1ad11dd2f826d11d81897319671aa10017b8c after PR46 and root claima284e78.
Root owns shared state/schema/build/workflow. Reviewer imported only the author's
two Markdown artifacts and owns only this review document. All prior review
worktrees and their verification copies remain preserved.

## Authority, scope and concrete assessment

Read root/server instructions, STORAGE-016 brief, ADR0003, specificationv0.45
§§3.3–3.5/4/5 and NUM-02/04, accepted service6d0cd67f, existing writer/retention/
plan/bundle boundaries and integrated017 global claims. Inspected every proposed
value, operation, disposition, branch matrix, closure/bound rule and H01–H16.

- Global typed subject gating is independent of operation scope/key, transient
  connection, replan or confirmation receipt. CID0/fullMAX and positive receipt
  identities retain their existing meanings; names/amounts/timestamps cannot
  correlate subjects. HoldRef/cause, mutation request and per-hold opaque version
  are distinct. Exact historical mutation retry precedes current freshness and
  cannot append/reactivate. A resolved hold cannot reopen through a new detection;
  old VersionRef reuse cannot create ABA. Internal positions are not domain IDs.
- Simultaneous independent causes remain independently active. Resolution uses
  the complete expected active VersionLocator set; a subset may resolve while
  other holds remain. Application-linked closure requires this namespace's whole
  active set closed without replacement, while other eligibility/permission
  blockers remain service obligations. Replacement closes old/creates new in one
  unit, keeps the subject held and preserves immutable linked history. All
  preparation precedes writes and post-write exceptions require real rollback.
- Active discovery selects ALL exact-subject anchors, including resolved ones,
  before filtering status. Head/state/complete history metadata cannot disappear
  through an inner join, positive initial cursor or first-page shortcut. Actual
  count/type/domain/parent/order checks and64/65 traversal cover selected rows;
  no lifetime materialization quota substitutes for them. Returned active/current/
  first bodies receive full declared closure; inactive/unselected old document
  bodies are explicitly outside that narrower certificate. History pages validate
  their actually returned bodies. Oversized active sets fail, never imply clear.
- Typed source/config/context/support/known-state and present selected-plan links
  require actual closure, not ref existence as authority. Historical observations
  need not equal current mutable contribution/head/policy. Selected application
  links reuse full002/017 closure. Missing/NULL/unknown data is not invented0 or an
  unrelated actor/action FK. Matching concrete crosspaths still require physical/
  API review and independent fixtures; this document does not prove their code.
- CapturedRevision requires a real stored selected positive revision and complete
  finalized occurrences. Whole-input aggregate overflow preserves that list and
  produces no successful subset. FailedCapture requires explicit numeric_conversion
  or unresolved_capture_policy, required retained alternative source/provenance,
  captured/proposed/prefix extent distinctions and known/unknown fields. Optional
  prior PlanKey gets the SAME full closure and cannot certify that the unfinished
  proposal became a captured plan. A failed finalized amount/head remains absent;
  no fake valid request, revision0 or PreparedBundle. The accepted service004
  selected-complete-plan protocol remains unchanged; upstream producer/evaluator/
  semantic codec work is separately required.
- TimerAdjustmentDeficit requires an actual finalized negative captured occurrence,
  selected RequireReview and full unapplied amount. It is distinct from numeric
  overflow. Explicit reviewed mixed composition remains external policy; absent
  meaning records UnresolvedPolicy, without inventing whole-product status,
  post-finish components or correction entitlement. Later correction has a distinct
  source/key and retained held origin/remaining facts; it cannot replay the old
  original or applied siblings. CORE-008/ACCT-005/006 gates remain mandatory.
- The nonowning adapter writes only hold records/history/heads/decision links. It
  cannot certify that arbitrary caller SQL staged no effects, that a linked bundle
  is new in this unit, or that caller commits/completes both. WHOLE-attempt no-success,
  same-unit composition/current authority and no clear/apply gap are explicitly
  service protocols requiring their own real tests. A staged/read/duplicate/link
  never becomes permission, durable Held or normal success after uncertainty.
- Positive reserved count limits, checked combined totals, exact owned byte roles
  and actual cumulative writer budgets are retained. Pure shape/limit errors precede
  ticket/SQL/cleanup and preserve prior work. Selected known failures establish
  whole-unit rollback; already-aborted primitives are not cleaned twice. Original
  capability/protocol/unknown WorkFailure survives, including cleanup uncertainty.
  Both Exact SQL classes precede both sizes and canonical grammar. No REAL,
  signed-MIN negation, saturation or false truncation is authorized.

## Actual findings and corrected revision

1. Mutation identity was ambiguous. Final document pins ONE shared raw-byte key
   across detection/resolution; kind is stored data. Opposite-kind reuse conflicts
   before freshness only after actual stored-kind closure is validated.
2. Application linkage could imply impossible same-unit/newness/commit proof.
   Final text explicitly limits it to selected data closure and retains owning
   service protocol/tests and no replay authorization.
3. H05 token conflict after a closed hold contradicted closed_hold priority. Final
   retry→closed NEW detection→active cause/current-version/token order and corrected
   H05 agree; resolution also pins complete-set priority before per-entry status.
4. Plan/failure/cause combinations and occurrence completeness were underspecified.
   The exhaustive final matrix pins numeric aggregate versus pre-conversion cases,
   timer captured-only and nonnumeric unresolved policy, with captured_complete/
   proposed_complete/known_prefix meanings and no omitted known occurrence.
5. H15 grouped pure input overage with rollback. It now separates no-cleanup pure
   refusal from post-SQL known rollback and original uncertainty.

All five actual corrections were inspected in the COMPLETE revised contract and
author note. H01–H16 were rechecked after the edits. No remaining blocking DOCUMENT
finding. Exact C++ signatures/owned result layouts, byte formulas, schema and
executable fixture/oracle design remain later independently reviewed artifacts.

## Independent literal checks and limits of this approval

Python3.14.8 stdlib integer/Fraction arithmetic, independent of production helpers:
H07 time55+5=60 while scoreMAX+1=9223372036854775808; H08 same exact attempt exceeds
signed64 without a finalized value. H09 MIN magnitude is9223372036854775808, but
subtracting actual Main1 leaves MAX debt9223372036854775807; -1-MAX=MIN. H10
0 Main/0 debt/-150 unapplied conserves request-150 without changing Main100/debt7.
H14 metadata64+1=65 and active257>256 are distinct limits. PASS. Other H examples
were reviewed as literal identity/CAS/closure/protocol requirements, not executed
hold behavior. Ignored local-evidence/storagetest016-review/contract-math.json
SHA256f8b10e2881c5d310b15c02e3fbb79d3e3da7bbfdc9a84dfffb95ee0f47e35f7b.

Actual `python3 tools/check_project.py`: PASS396 records/122 decisions/links/
presets/current HTML; `python3 tools/progress.py render --check` and
`git diff --check`: PASS. Git diff against aad confirms no core/server/test/
fixture/schema/build/workflow changes. No native/Qt/provider run, hold API/stub,
physical migration, fixture, feature test or intended red is supplied by this phase.

This approval permits the coordinator to assign the next reviewed proposal stage;
it does not complete STORAGETEST-016. Physical step6 must preserve exact prefix1–5/
66, then actual minimal API/stubs, independent foundation/tests/review, compiler/
discovery, selected official environment and FIRST intended red remain required.
STORAGE-016 production waits those gates and separate implementation/code review/
acceptance/promoted CI. Service004 retains its own prerequisites/baseline/red.
All agreed integrations/FX/renderers/locales/themes/security/platform/packaging/
backup/live/release obligations remain. No owner decision or scope waiver is needed.
