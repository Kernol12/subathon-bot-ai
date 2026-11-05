# STORAGETEST-018 independent SQL feature-test review

Reviewer `/root/capture_fixture_author`, different from SQL feature-test author
`/root/capture_codec_tests` and harness author `/root`. Review support branch
`task/STORAGETEST-018-sql-test-review`, isolated worktree
`/tmp/subathon-storagetest018-sql-test-review`, created from actual integrated
candidate573790b. Own ONLY this new review note. This reviewer authored the
separate literal fixture/helper/diagnostic, so DOES NOT approve those artifacts;
their actual independent review belongs to `/root/capture_reviewer` in
[foundation review](storagetest-018-fixture-review.md). SQL test source and its
actual CMake/CI harness are the independent scope here.

Initial SQL author4376ed7/import5dfa has CPP blob
`5b7acadd2d4278d9068445c1ca5ce33d464cc7c4`, 30slots/38scenarios. Actual root573
compiles it with reviewed fixture JSON878005d and prior helper source. No SQL
feature body or new codec/store/manifest was invoked in this review. Final
approval waits for the actual corrections described below, complete actual-source
reread and final build/discovery; this initial note is not an approval.

## Requirements, observations and findings

Read root/tests instructions, actual task/API, concrete/physical018 and consumer014
contracts, DB-01/API-02/NUM-04 and existing writer/fault semantics, source author
note, every actual SQL method/helper, integrated new CMake and full SQL-workflow
diff. Existing fixture data is separately reviewed input rather than a self-certified
oracle. Actual setup calls OLD migration runner with independent literal1–7 then
commits original A intake/support; feature calls are confined to operation paths.
A clearly named disposable marker table is added after schema readiness to observe
whole cleanup; it does not substitute for schema/manifest acceptance.

Initial source covers complete actual manifest/prefix/projection comparisons,
full-key absence/off-scope selection, first immutable owned binding and exact
CID0/MAX/NUL identity, active provisional stage/read versus outside visibility,
healthy duplicates/conflicts/receipt aliases, both selected indexes before refusal,
malformed selected roots and original support/plan, current dimensions/retries/
later head excluded from immutable selection, exact pure-input priority, actual
foreign/stale/terminal capability truth, physical whole cleanup, host cleanup of
competing staged orphans, queued first commit, actual acknowledgement-loss unknown
and post-commit body failure without normal publication. Expected records are
literal complete13-member DTOs, never produced by a new codec or store.

Independently recomputed A55-byte record with event1 replaced by4096 gives4150;
limit4149 is complete selected DTO overage while4150 fits projection and reveals
semantic mismatch. The largest actual A document is SRA1 at157 bytes;157 fits/156 refuses.
Provider is22 actual bytes; the selected request reversal reference is26 bytes. These are runtime outcome/byte-oracle assertions;
preallocation-before-ownership and actual LIMIT2/index/query scope still require
later implementation source review and are not measured by test names alone.

Two prereview issues were raised with author/root before first execution:

1. Selected corruption initially exercised invalid X00 frames, but did not separate
   structurally valid codec DATA from admitted complete indexed meaning, selected
   child cardinality/contiguous ordinals/canonical integer requests, or exercise a
   healthy exact reference ceiling. Requested bounded additive rows/assertions,
   retaining every existing assertion, rather than a generic validator/test harness.
2. The direct-missing-plan candidate retained healthy SRX Y bound to P while
   changing only candidate.plan to `missing`. Concrete018 §4 explicitly resolves
   selected closure in order and validates each present root before comparison.
   Thus existing healthy SRX/P versus candidate/missing is source_mismatch BEFORE
   a new plan lookup. IC18-08's direct healthy absent target is conditional on
   reaching that target without an earlier mismatch/corruption. A fictitious
   healthy SRX with a dangling required plan target would not resolve that conflict.
   Root and author independently agreed this is an oracle defect; retain this
   actual case with the exact reached source_mismatch expectation and a written
   reason. All other direct-missing assertions remain. This does not change the
   product contract or relax a reviewed/observed baseline: first feature execution
   has not happened and initial tests were not approved.

## Actual initial checks and harness audit

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug
ctest --preset sqlite-reference-capture-debug --show-only=json-v1
clang-format --dry-run --Werror tests/qt/sqlite_reference_capture_tests.cpp
git diff --check
```

Actual initial configure/build145steps/link/discovery all exit0:366 registrations
= old308 + separately reviewed foundation1 + codec27 + SQL30. Exact compiled
-functions inventory equals registered SQL30; all enabled intentional-red labels,
no DISABLED/WILL_FAIL/inversion. Show-only JSON retained at
`/tmp/storagetest018-sql-review-discovery.json`. No test method/setup/feature body
ran. Host GNU16.2.1, CMake4.4.5/Python3.14.9, Qt6.12.0 and selected Boost1.83.0.

Root CMake links the public new store and QtTest, uses the correct independent
fixture paths and preserves every30 named slot under separate enabled draft-red
labels. Workflow changes only append mandatory independently reviewed foundation
execution and exact27codec/30SQL compiled registration/label/no-inversion guards;
old suites, pins, check names, permissions and commands remain. Intentionally red
feature bodies are separate from ordinary passing CI, without skipped cases or
failure inversion. Actual selected CI observation is root's later gate. This
review does not configure branch rules or invent a required check.

## Actual first revision reread

Actual author b5a7205/CPP931cb973 contains eight additive selected child/canonical
corruption rows and five structurally valid document crossbinding cases. Reread
all actual edits, preserving original malformed-data cases and whole rollback
assertions. Recomputed framed mutation positions from the grammar: SRC policy
byte13, first category byte111, embedded SRC begins at SRA byte26; SRX receipt
last byte67, CID last byte75 and rule-ref data byte93. Their fixed-length edits
retain known codec grammar and alter the intended admitted/binding fact. Matching
unknown0 category edits affect both retained SRC and embedded SRA; the policyZ
case intentionally changes only retained SRC. Child deletion/ordinal2/extra rows
and ASCII0600000 satisfy SQL structural checks while violating selected1/0/1,
contiguous ordinal1 or canonical integer request meaning. No new codec creates
these arrangements. Actual plan-mismatch assertion is corrected to source_mismatch
with the required ordered-closure explanation; separately ACCEPTED that precise
oracle correction after rereading actual code and appended author rationale.

A further genuine additive oracle defect was identified before invocation:
healthy reference ceiling22 would admit the key/provider but reject the selected
request's `reference.original-time.v1` reversal reference, which is26 bytes.
Concrete018 §2 bounds each selected reference; existing plan field semantics also
explicitly categorize request reversal_behavior_ref as reference rather than value.
Requested healthy ceiling26 and selected25 result_limit (caller key22 still fits),
retaining the original field23/ceiling22 assertion. A separate field27/ceiling26
arrangement isolates the selected index field overage. Rule explanation30 is a
value/owned field, so it does not invent a reference ceiling30. Final approval is
still pending the actual corrected artifact.

## Final actual SQL-test approval

**APPROVED for actual SQL feature-test source and its CMake/CI harness at
root55046aa, after actual final revision reread and independent build/discovery.**
No remaining blocking SQL-test defect found. This approves the tests for the later
intended-red gate; it is NOT an observed red, SQL feature pass, implementation
approval, self-approval of fixture/helper/diagnostic, or full product acceptance.
Root retains actual selected-CI foundation observation and FIRST same-clean-head
feature execution; neither was performed or certified by this reviewer.

| Final artifact | Actual Git blob |
| --- | --- |
| SQL feature CPP | ea7b53c50fd0697835ef4ecce59dd0d89689337e |
| Root test CMake | f3987b413c623f84bc9fde7cf349e034a178475b |
| Selected SQL workflow | ed141178ceedfec168197b9a86d04772bf87ff78 |
| Separately reviewed fixture input | 878005d95a0c12d69df6b836ac09cd59ba5168ba |
| Separately reviewed fixture helper | 82cbf17b5265381e89bd5c32c1076eed734ead2e |

Final author b32c8c3/import5b44815 preserves all original assertions except the
explicit independently ACCEPTED plan error-oracle correction described above;
no observed baseline was replaced. Additive final source correctly uses healthy
reference ceiling26, selected25 result_limit/whole rollback, original field23/22
category unchanged, additional isolated field27/26 and pure caller-key21 refusal.
Reread the actual final changed source and appended author reason: expressly
ACCEPT this reference-boundary correction as well. No implementation, fixture
meaning, required closure, old assertion or product requirement was changed.

Final inventory:31 enabled SQL slots,51 scenarios (16 selected-corruption rows,
5 structural-crossbinding rows,2 cumulative-budget rows; all other methods one).
The exact31 compiled method names equal all31 registered CTest commands. CMake
and workflow add the one real new method and update all guards mechanically to31;
old checks/pins/commands and enabled draft-red separation remain unchanged. The
new structurally valid closure cases and exact reference boundaries close the
identified focused gaps without inventing a successful structural-codec admission
certificate. Every selected-error path checks the actual category/fixed text,
whole marker removal and independent lock acquisition inside the catch; the
underlying real writer result must remain rolled_back/no normal value. Real
commit-unknown and cleanup-unknown remain distinct from known rollback.

Actual final checks in this reviewer worktree:

```sh
git merge --ff-only 55046aa
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug
ctest --preset sqlite-reference-capture-debug --show-only=json-v1
clang-format --dry-run --Werror tests/qt/sqlite_reference_capture_tests.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0; final configure/incremental source build/link succeeds with no compiler
warning observed. Show-only has367 total = old308 + independent foundation1 +
codec27 + SQL31. Independent static/compiled inventory confirms31/51 and no disabled
or inverted feature registration. Workflow YAML plus all40 shell blocks and24
embedded Python blocks parse; their bodies were NOT executed in this reviewer
assignment. Final discovery SHA256
`0549d17093f13403bd0809754c16f7600efb653ef87235fe779436d4964366d9` at
`/tmp/storagetest018-sql-review-final-discovery.json`. Project/render checks pass
419 task records/122 decisions/current HTML. No Qt/SQLite fixture, feature setup,
codec/store/manifest body, native acceptance or red command was run by this
reviewer; compiler probes and method-list discovery are build evidence only.

Limits remain explicit: direct absent initial-plan outcome was not isolated by
this fixture because its earlier retained SRX already binds P; the case now
checks the actual ordered mismatch instead. No genuinely healthy dangling SRX
fixture was fabricated. Busy after active BEGIN, allocator-before-copy semantics,
LIMIT2/index query shape, bounded metadata preflight, allocation mapping, no
growing-history scan and all undeclared-access exclusion still need later source
audit. These tests are synthetic disposable component acceptance, with neither
current authority/evaluation/application effects, recovery/power-loss/provider/FX,
platform/UI/package nor full release verification implied. STORAGE018 production
continues to wait for the actual integrated reviewed/red baseline.

## Observed SQL setup defect and actual revised-baseline approval

Root's actual clean55046aa SQL attempt, after prior approval4247210 and selected
CI42090598539 green, is INVALID as a complete missing-feature baseline. This
reviewer inspected preserved raw `/tmp/storagetest018-sql-first-red.txt`, SHA256
`e13621f40fd74aa36ea68baca186e5374658eb14d942f9fd431f942badc372bf`:51 actual
Qt failure rows comprise50 explicit MISSING_REFERENCE_STORE markers and ONE
non-marker `cumulativeSqlBudgetsRequireActualWholeRollback(result-bytes)` setup
error, `SQLite work operation failed`, before the new store. This reviewer missed
the per-cell overhead in the original setup review; the earlier approval does
not turn this actual setup failure into an intended marker. Preserve that first
attempt rather than relabel/rewrite it. Separate valid27 codec-marker FIRST red
at that head is unaffected and must not be replayed for this correction.

Independently checked the EXISTING writer specification, not an implementation-
generated expected value: sqlite-writer-v1's budget table and writer-transactions-v1's
Trusted inputs table charge EACH value8 base bytes plus QByteArray payload, for
both bound and result values. Thus payload100 costs108 and cannot fit the selected
result_value_bytes100; payload92 costs exactly100. Actual old consume_value agrees
with this previously accepted contract. The marker INSERT returns no result cell,
so it does not consume any result-value budget before this one-cell SELECT.

Actual different author correction a8a440e, imported to this review branch as
e73ef54, has CPP blob `58b22366b2f853125fd3a79fe9146d2283ccd1ec`. Reread the
ENTIRE actual delta: only the old setup SELECT100 becomes SELECT92 and an additive
pre-feature assertion requires exactly one row/one non-null actual QByteArray cell
whose complete bytes equal92 zero bytes. That equality also proves exact size92.
Result budget100, subsequent new find, exact expected result_limit category,
whole marker removal/independent in-catch lock release and real rollback/no value
assertions remain. No baseline assertion is dropped, skipped or relaxed and no
production/fixture/schema/contract meaning changes. Source31slots/51scenarios and
CMake/workflow31 guards remain unchanged.

**APPROVED for this actual specification-based SQL setup correction and revised
SQL baseline source58b22366.** This is separate review of a genuine test defect;
it supplies NO observed valid revised red or feature implementation authority.
Root must publish the actual revised head, observe its selected foundation/CI and
run the corrected SQL suite separately; preserve original invalid output and the
completed codec FIRST red. All prior final SQL source/config review remains valid
except this explicitly revised setup fragment. Implementation waits for integrated
actual valid baseline evidence.

Actual reviewer-only checks:

```sh
git cherry-pick a8a440e
cmake --build --preset sqlite-reference-capture-debug --target server_sqlite_reference_capture_acceptance
ctest --preset sqlite-reference-capture-debug -L '^sqlite-reference-capture$' --show-only=json-v1
clang-format --dry-run --Werror tests/qt/sqlite_reference_capture_tests.cpp
git diff --check
```

All exit0; incremental actual compile/link has no compiler warning observed, exact
compiled/registered31 methods still enabled/intentional-red. Read-only original-log
inspection and contract math performed as described. No fixture/setup/new feature
body or revised red command was invoked by this reviewer.
