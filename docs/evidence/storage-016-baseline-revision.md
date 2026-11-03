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


## Actual reviewed regression red before source correction

Actual separate test revision approval1650658 pins test82980fdb/revision notef79a3326.
Root integrated UNCHANGED buggy runtime0fe (97c83b74) and helper1cf (a18a0bd7),
then compiled five steps/ZEROwarnings with35 discovered slots. At CLEAN root
68dc779,2026-11-03T14:20:25.413–14:20:25.514Z, ONLY the three new data rows
were run as `executable function:data-tag`. EACH actual exit1,2 init/cleanup
PASS/1 scenarioFAIL/0skip/blacklist;0HOLD-MISSING. No original full baseline replay.
[Exact combined per-process output](storage-016-regression-red.txt) is retained.

The priority failures are exact input-category assertions, agreeing with the
independent native helper observations of11/9 instead of8. The narrow lineage
failure reports the refusal helper's combined assertion. Root read-only GDB at
actual test319 proves its body returned normally and `caught=false` before the
required-storage-failure throw. That distinguishes absent expected exception from
an unproven durable-byte mutation; do NOT call the generic assertion text evidence
that bytes changed. The stack identifies the selected corruption regression.
GDB log `local-evidence/storage016-lineage-regression-gdb.txt` SHA256
9dc953c9cbf4d1412c990a66757ac89cbc7b80a6a429984668e2a0206b5bea04.
IDE exposes only scaffold run configurations and no Qt-test runPoint, so this was
read-only batch GDB on the synthetic binary; no user breakpoint/session changed.

Actual isolated output SHA256:

- audit/Exact53c581b46ef186892d1e2d8cc37b3916e71fb11a443302f7d4c17c14faacd43d;
- replacement/order3a43ab29304e374697a89b505498b678f8602564d6464194da0c1c19529a4927;
- narrow ancestry030ee26e13e5e960241e74db7a91cfa9ae2b8b2fe781e6a7d66cb8ce40aadd69.

The independently reviewed additive tests have now reached genuine behavior red.
Root releases the two bounded source fixes: staged pure validation and complete
scalar ancestor/reciprocal metadata closure. Preserve tests82980fdb and all old
oracles; source/code review and passing35/full264 acceptance remain future gates.
Corrected history inputs will be verified against already-correct semantics; no
artificial missing-feature run or passing product claim is made here.

## Follow-up captured-token phase regression

Final helper ec78b697/1b8af36 added conditional presence checks to the required
body pass. Actual independent source review found that first CapturedRevision
state-token absence is checked there BEFORE the required request.audit document.
API46c lines198–200 and actual category review35b1092 explicitly retain known
state-token presence as a cause/plan MATRIX requirement. Required envelope shape
precedes that matrix stage. This is an existing ordered pair, not an invented
ordering between unrelated failures within one stage.

One additive row, `required-audit-before-captured-token-matrix`, starts from the
valid frozen dOverflow input, resets ONLY its first-observation state token and
makes its attempt audit null. All keys/counts stay valid. The public-method oracle
is invalid_detection8, and the existing test proves earlier caller staging can
still commit. Current ec78 rejects invalid_plan_evidence before that envelope.
No old row/oracle/assertion or fixture changes;35 slots/268→269 scenarios.

Independent review and an actual reached-red on unchanged ec78 are required
before releasing this next helper correction. Full264 green on the previous
268-row candidate remains valid evidence for that candidate, not this new row.

Actual separate approval001896f85ad5b0211075a38b1ae0b96a1b292991, integrated
f67ae73, pins revised testd740a175 and this note3d6ac2f4. Root three-step native
rebuild completed without warnings. At SAME CLEAN f67ae7393d105a696ae835649162a3110d8a1568,
unmodified helper ec78/testd740, actual public row14:54:48.238859–.266877Z:
exit1,2init/cleanupPASS/1FAIL/0skip/blacklist,0missing markers. Failure is the exact
input-category assertion; reviewer's independent pure probe observes6 instead of8.
[Exact public output](storage-016-token-regression-red.txt), SHA256
95d80748d94dddbe89c3ca389791910b403cf5705cb7a29c0717f6e8ac584dc4.
Root then explicitly released ONLY moving captured token-presence checks out of
body_cause/body_detection, retaining matrix checks. Other optional-group rules
remain unchanged. This new genuine red precedes source correction; no baseline
replay or weakened assertion. Final source/config review and269-row green remain
future gates.
