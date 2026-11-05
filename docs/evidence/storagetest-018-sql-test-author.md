# STORAGETEST-018 — SQL store and manifest acceptance author

Author `/root/capture_codec_tests`, 5 November 2026. Separate branch
`task/STORAGETEST-018-sql-tests`, worktree
`/tmp/subathon-storagetest018-sql-tests`, base task revision f7e4653.
Coordinator owns shared build/CI/progress, migrations/schema allocation and PR68.
Only this note and `tests/qt/sqlite_reference_capture_tests.cpp` are authored here.

This is an AUTHORED candidate, pending actual independent test review, compile,
mandatory separately reviewed foundation green and intended missing-behavior red.
No new manifest/store/codec entrypoint or feature test has been invoked. No old
baseline/production/fixture/build/state source is modified.

Authority: specification v0.45 §§3/5/17/18, DB-01/API-02/NUM-04; concrete
[018 contract](../contracts/sqlite-initial-captures-v1.md),
[physical companion](../contracts/sqlite-initial-captures-schema-v1.md), accepted
[consumer](../contracts/reference-application-adapters-v1.md), frozen
`sqlite_reference_capture.hpp`. Supplier document review and inert marker source
already exist separately; this candidate does not approve its own tests.

## Independent setup and expected values

Uses DIFFERENT author's `support/sqlite_reference_capture_fixture.hpp` plus its
literal JSON, originally3dc3e5c with actual reviewed-driver correctione35 and
five-part-predicate correction3f5082e pending different review at authoring time.
Root must import the actual reviewed foundation revision and observe its green
before first feature execution. This test author did not author or approve those
shared fixture artifacts. Setup calls only the accepted existing writer/migration
runner with literal full7, raw prepared SQL and raw observers; NEVER the new
manifest, store or codec. Old migration runner installs92-object schema first;
a separate clearly named test marker table is added afterward to prove cleanup.
Each case has independent disposable state and earlier committed A intake/support.

The direct expected record is literal A(I,synthetic.reference.v1/A/N/E,
Receipt7/CID0,C/X/V/P1/Y); B has opaque E+NUL, ReceiptMAX/CIDMAX,
C2/X2/V2/P2/1/Y2. The actual SQL fixture has policyW/Grace120000; no accidental
substitution of the separate pure-codec fixture's p/Immediate config occurs.
Complete equality checks all13 columns/logical members; no encoder output creates
expected records. New methods only appear in feature operation/assertion paths.

## Focused acceptance inventory

30 feature slots; selected-corruption has8 independently initialized rows and
cumulative-budget has2, giving38 feature scenarios. Qt init/cleanup are separate,
not feature cases. Root should register each30 named feature slot, not `_data` or
helpers. Names are the exact private-slot definitions in source.

| Contract examples / obligation | Authored acceptance |
| --- | --- |
| Physical prefix1–6/step7 | Complete manifest coordinates, statements/order/projections, all7 independent literal hashes, step7 payload71601 and92objects |
| IC18-03 | Absent full-key lookup ignores deliberately corrupt off-scope B; installation scoped |
| IC18-04/05/17 | Real stage plan/SRX/index in one session, own provisional read, outside invisibility until actual commit, independent complete raw verification, owned result after facade/work/writer destruction |
| IC18-05/06/07 | Exact already-present; conflict before absent proposed targets; installation+receipt alias; BOTH indexed closures validated before conflict |
| IC18-08/09 | Absent direct receipt/CID/plan/SRX →missing_reference; healthy different source/event →source_mismatch; present malformed intrinsic SRX link →invalid_stored_data; indexed SRC/SRA/SRX/original facts/support/request/rule/index corruption →whole rollback; corrupt original outranks replacement conflict |
| IC18-10 | Current contribution dimensions, later plan head/revision and altered retry facts do not replace immutable original support/plan1 |
| IC18-11 | Every store option0/MAX; input overage before refs/domain; empty ref before Receipt0; SRI revision0/2/MAX; actual stale/foreign/terminal writer errors; pure refusal preserves earlier work |
| IC18-12 | Largest A document157 fits/156 refuses; complete index row4150 versus4149, with all refs individually fitting; fitting semantic mismatch versus overage; reference23 versus ceiling22 before ownership |
| IC18-13/14 | Two actual queued candidates: first commit wins, second conflict plus host rollback removes competing plan/SRX; explicit known rollback removes all three groups but preserves earlier intake; explicit later healthy attempt permitted |
| IC18-15 | Actual successful physical COMMIT with lost ack gives unknown/no value; lost actual cleanup ack outranks local corruption; acknowledged commit followed by synthetic body exception remains committed/work_exception/no normal result |
| IC18-16 | Already exhausted cumulative SQL statement/result-byte budgets produce exact mapped reasons only after real whole rollback; missing SQL table maps sql_failure after rollback |

Rollback assertions use prior marker removal and an independent in-catch actual
BEGIN IMMEDIATE/ROLLBACK, then actual writer transaction outcome. Healthy refused
calls preserve caller work where no candidate was staged; staged refusal units are
explicitly rolled back by the host, never committed with orphan plan/context data.
`exercise` captures and rethrows unexpected body exceptions after actual work
completion, preserving explicit marker failure attribution. Expected errors are
caught narrowly and exact public category/message checked; setup/SQL exceptions
are not relabeled missing behavior.

## Commands, harness and limits

- Read applicable root/tests instructions and current concrete documents/API.
- Created separate support branch/worktree from actual taskf7e4653.
- `clang-format -i tests/qt/sqlite_reference_capture_tests.cpp`: exit0.
- Static source inventory:30 feature slots,2data methods,8+2data rows.
- `git diff --check`: exit0.
- Native compile/discovery: pending root's shared harness; NOT claimed.
- Different actual test review, foundationgreen and FIRST intended red: pending;
  no feature execution occurred and no pass or red result is claimed.

Root harness should link `subathon_sqlite_reference_capture` + `Qt6::Test`, AUTOMOC;
compile definitions match the fixture diagnostic's seven vector-path macros.
Register30 feature slots with intentional-red labels kept outside passing CI;
ordinary automatic acceptance requires later implementation and source/config gates.

Preallocation/borrowed fixed metadata scans, exact index/query plans/LIMIT2,
no unbounded history scan, thrown allocation mapping and no undeclared SQL bypass
still require actual implementation source/config review. Raw fixture corruption
is test-only, not production API capability. Source text/hash uniqueness is not
used as a substitute for full-key SQL identity. busy during in-transaction SQL
cannot honestly be manufactured through this single-writer setup; ordinary writer
busy/admission gates remain old acceptance and source audit. No power-loss/recovery,
current authority/application effects, live provider/FX/UI/platform/release proof
follows from these synthetic real-SQL component tests.

## Independent-review author revision (before first execution)

Different SQL reviewer `/root/capture_fixture_author` examined actual4376ed7 and
requested focused coverage beyond malformed frames. This additive revision keeps
every existing case/assertion except the explicit not-yet-approved plan error
oracle correction below. New coverage derives directly from018 §4's complete
admitted selected closure: one unknown0 category with BOTH retained SRC and SRA
embedded SRC changed consistently; retained SRC policyW→Z while SRA retainsW;
fully framed SRX receipt7→8, CID0→1 and ruleR→S mismatches. These edits preserve
lengths and raw codec grammar, showing structural DATA is not admitted closure.
They use independent frozen fixture bytes, not new encoders/decoders.

Eight additive corruption rows exercise absent/ordinal2/extra rule and request
rows, an extra modifier (required selected child counts1/0/1 and contiguous
ordinal1), and noncanonical exact numerator `0600000` despite a valid BLOB SQL
shape. Healthy reference ceiling22 succeeds,21 gives pure caller-key input_limit,
then the existing selected23-byte field under22 still gives result_limit and
whole rollback. No previous constraint or rollback assertion is weakened.

Resulting inventory:31 feature slots,3data methods; selected-corruption16rows,
structural crossbinding5rows, budget2rows, therefore51 feature scenarios.

The reviewer also identified a genuine authored-oracle priority ambiguity:
`candidateMissingRootsRemainExplicit` changed only proposed plan P→`missing`
while retained SRX Y still names P. Original candidate expected missing_reference.
Root coordinator explicitly decided on018 §4's ordered receipt→contribution→
retained SRC/SRA/SRX→plan resolution: a healthy present retained SRX describes a
DIFFERENT candidate plan, so source_mismatch precedes any lookup of the absent
proposed plan. This revision changes only that assertion's category to
source_mismatch, preserving the case and every other direct missing-root check.
IC18-08's broad direct-missing-plan example is conditional on actually reaching
that healthy direct lookup; this arrangement cannot isolate it. A fabricated SRX
with a dangling required intrinsic plan target would create corruption and cannot
prove healthy absence. No isolated missing-plan oracle is claimed. Different SQL
review must expressly accept this reasoning/revised assertion before first red;
no observed baseline exists to rewrite and no implementation was altered.

Actual commands for revision: `clang-format -i
 tests/qt/sqlite_reference_capture_tests.cpp`, `clang-format --dry-run --Werror
 tests/qt/sqlite_reference_capture_tests.cpp`, `git diff --check`: all exit0.
Static inventory only; no native SQL/feature invocation. Root handles compiled
registration, actual foundation green and reviewed intended red separately.
