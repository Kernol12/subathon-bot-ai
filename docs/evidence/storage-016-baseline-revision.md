# STORAGE-016 baseline revision — history input correction and additive regressions

Root is the test-revision author, separate from both production authors. Actual
independent revision review and reached-red evidence are required before fixing
newly exposed production defects. Existing fixtures/oracles/assertions remain fixed.

## Genuine history request defect

Original test0cce9545 requests `history({"hOld",0,{},64})` twice while expecting
frozen `history_pages.old_resolution`, which contains ONLY actual position2 and
pins338 bytes/6 items. Logical701 history positions1..N with initial cursor0 and
API46c bounded events AFTER cursor require positions1 AND2 at0. Position1 is a
real healthy initial detection, not an extra event to suppress in production.

The two query inputs now use after_position1. All expected values, full DTO
comparisons and338/6 exact-fit/one-under output assertions remain unchanged; the
8-byte cursor field has the same input charge. This corrects the request to match
the intentionally resolution-only frozen oracle. No fixture mutation, production
history change, test deletion/skip or relaxed comparison is permitted.

Actual implementation checkpoint0fe329c independently returns two positions1,2
for hOld/after0/page64, no-more. Source author's GDB paused at HoldAccess::history
with this query and a stack to test575; preserved `/tmp/storage016-history-gdb3.txt`
SHA256859d21f3358a502043c2c58dc10845ecd2ce5468781119a4d60f12015c04bca6.
Root read the actual stack/values. IDE router reports no sessions, no worktree test
runPoints and no owned breakpoints; source author had documented failed IDE launch
and used read-only GDB fallback. No user breakpoint/session was altered by root.
Static contract plus frozen input independently establishes the correction; the
implementation's output does not generate the expected result.

The original missing-feature red598 remains preserved. This genuine post-stub
input defect does not authorize changing implementation semantics to omit position1.
Corrected history cases should pass contract-correct code; do not manufacture a
regression red for already-correct behavior. Record the actual revised baseline,
separate review and targeted outcome.

## New production defects awaiting additive regression gate

Independent helper review confirmed two cross-STAGE errors in a18a/1cf31df:
required audit envelope shape must precede cause Exact component checks, and ALL
resolution replacement bodies must precede expected-set ordering. API46c explicitly
orders required shape before matrix/Exact and local ordering; this establishes no
new order among unrelated defects inside one stage.

Independent actual0fe runtime review also found missing scalar first-producer
validation on a referenced supersedes ancestor for narrow find(hNew). Physical83e
requires that closure and reciprocal replacement metadata, without loading sibling
full views. A same-subject existing but different detection is a precise witness;
active(MAX) auditing every anchor is not a substitute for narrow find closure.

Root will add focused independently reviewed public-method cases, observe their
actual failure on unchanged buggy implementation, then explicitly release only
the affected source fixes. This note does not approve or claim any future result.

## Actual additive regression candidate

Exactly THREE rows are added to existing slots (35 compiled slots remain; total
scenarios265→268). No old row/assertion, independent oracle or fixture is removed.

- `required-audit-before-Exact`: valid dOverflow keys/counts, null attempt audit,
  component_bytes2 makes its existing numeric failure Exact oversized. Expected
  invalid_detection is the earlier REQUIRED-envelope stage, before Exact bounds.
- `replacement-audit-before-set-order`: valid frozen rReplace keys/counts, reversed
  expected-active set and null replacement DetectionData audit. Expected
  invalid_detection comes before local set ordering; no unrelated same-stage
  tie-break is invented. Both preserve caller staging through the existing helper.
- `narrow-supersedes-first-producer-mismatch`: change ONLY hOld.first_detection_ref
  to existing same-CIDMAX kind0 dFail (actually belongs to hFail), retaining real
  FK targets. Explicit FK0 and changed-row assertions establish setup. Narrow
  find(hNew), not active(MAX), must detect the referenced ancestor's mismatch with
  its real first produced version/dOld and return invalid_stored_data after actual
  whole-unit rollback. No sibling full-view or unrelated-document audit is required.

Actual reviewer direct-native a18 diagnostic already observed incorrect errors11
(exact_component_limit) and9 (invalid_resolution), versus required8
(invalid_detection). That scratch result is implementation evidence, not execution
of these newly authored public-method regressions. Actual review of this revision
and reached-red on unchanged buggy implementation are still pending. Both affected
production authors are explicitly HOLDING corrections until that gate is recorded.
