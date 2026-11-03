# STORAGE-002 — independent source and CI configuration review

Implementation authors: bundle_implementer (runtime) and bundle_validation (pure
value helper). Reviewer: bundle_reviewer, separate agent/worktree, 2026-11-03.
Verdict: **approved for source/configuration integration at the exact artifacts
below**. Normal latest-head GitHub CI/review/thread/mergeability gates remain.
This bounded component approval is no whole-product/platform/live verification.

## Exact reviewed artifacts

Runtime candidates f969ba6→93f75ef→3e1b6cb→2673893; pure helper824f58d→159850c.
Root integrated final source as6a6a200572a571a29b06325029bbbd0ee7491d97. Reviewer
used isolated storage002-review, local final source commit2a78f64 plus EXACT root
ec60f66 configuration copied for verification (root owns configuration authorship).
All final source/test/config blobs match root's reported actual final artifacts.

| Artifact | Git blob |
| --- | --- |
| Runtime source |a9056be460d6f0536d66341d09e70f177a6638e3 |
| Pure internal values helper |b0ca921cb27068ecc33228d136b82506fcb6e0a3 |
| Unchanged public header |8e24f0624673bf88f86405651f9aabee1026a226 |
| Integrated35-slot feature CPP |8bf359827feb544651ecf2f218a518f70c8de938 |
| Qt CMake |0e0d039c781a8106c99ef6bd6a5d831edbe8eb87 |
| SQL workflow |783dda747bcf3a766fdb4c7892c5680fc51fece1 |
| Presets |b9aabac860fecced1a29a290e875a56c4a4fafd1 |

Authority: specification v0.45 §§3,4,8,16/DB-01/NUM-01–04/OPS-02; concrete
sqlite-application-bundle-v1 and physical companion revision1, logical
application-transaction-bundle-v1, existing writer/retention/receipt/contribution/
plan contracts. Existing baseline review917/finala32 and original32 red90fa remain
valid historical gates; no old red was repeated or retroactively reattributed.

## Review and resolved findings

Read the complete runtime and internal helper, public interfaces and relevant
unchanged writer/plan/contribution implementations, concrete/logical/physical
contracts, frozen fixtures and all feature cases. Independently extracted all14
manifest SQL literals and compared them byte-exact with the frozen JSON; checked
prefix reuse/full66 inventory/22 generated NULL indexes/framing expectations.

Checked pure options/key/source/count/scalar precedence, separate vector n+1
max_size guards, unsigned raw-byte balance/link ordering, exact local and aggregate
conservation including signedMIN, canonical Exact bounds, intent/state deep equality
and logical owned-byte accounting. Runtime checks covered borrowed capabilities,
strict actual SQL classes/NULL groups/typeduint64, non-owning current contribution
resolution and application-only patch, complete metadata versus selected bodies,
three immutable snapshots/CAS/no token reuse, provisional duplicate/claim priority,
full historical closure, actual credited reversal bases, finite64/65 source/history
pages, coherent terminal progress FK paths, cumulative budgets and actual known
whole-unit rollback versus original capability/protocol/unknown WorkFailure.
All allocating write statements/tracking are prepared before own writes, including
the final changes() statement; actual post-write writer/result allocations remain
guarded by writer/adapter rollback and preserve cleanup uncertainty.

Resolved actual review/self-review findings:

- Pure helper824 counted a fabricated payment-evidence byte around an actual
  support Conditions member. Actual159 counts the real member directly. Root
  separately reviewed reviewer-authored additive291 and independently reproduced
  frozen1105=23+588+112+192+190 before actual reached red, then authorized correction.
  Reviewer does NOT certify its own test oracle; see the separate root review and
  [author arithmetic](storage-002-contribution-byte-regression.md).
- Runtimef969 wrongly applied input_items to a full historical read. Actual93
  performs structural validation without that input-only quota, then enforces actual
  returned aggregate counts/bytes. Separate [new test review](storage-002-metadata-regression-review.md)
  accepts existing20 returned elements with input_items1/result_items20.
- Runtimef969 positive continuation queries could omit extra transitionordinal0 or
  progressposition0 while trusting declared counters/head. Actual93 proves unfiltered
  actual cardinality against every scoped visited chain/components; count+checked
  contiguous1..N proves min/max. Actual267 checks declared/actual counts BEFORE full
  vector materialization; narrow metadata does not inherit per-bundle/lifetime caps.
  Root FIRST2 red correctly reports quota/transition/progress failures, while state
  kind1 was already rejected by f969's initial equality continuation branch. That
  already-green state row remains valid coverage; no all-three-red claim is made.
- Actual3e checks aggregate returned-item additions and signed historical component
  counter growth, and preconstructs the final SQL statement before any writes.
- A genuine original terminal-session completion oracle defect was separately
  corrected1call by root AFTER this reviewer's [independent approval](storage-002-terminal-baseline-review.md).
  Sticky session_closed is preserved alongside physical rolled_back; no writer
  behavior, fixture or other assertion changed.

Programmatic removal of ONLY three new test methods/data helper and reversal of
that one separately reviewed call restores exact old featureCPP4b60fcaa. Removal
of ONLY three registrations/comment restores exact old QtCMake91035. All old
source/interfaces/fixture assertions and selected dependency/action pins remain.

## Actual independent checks

Environment: Arch Linux x86-64, GNU16.2.1, CMake4.4.5/Ninja1.13.2,
QtCore/Sql/Test6.12.0 with host SQLite3.54.0 and Boost1.92. Official selected Ubuntu
Qt6.12/SQLite3.53.4/Boost1.83 CI is a separate integrator-observed gate.

- Fresh initial cmake --preset sqlite-bundle-debug and full build passed. After
  final source/test/config integration, configure passed and full incremental
  build completed20 actual work steps, ZERO compiler warnings/errors.
- `ctest --preset sqlite-bundle-debug -L '^sqlite-bundle$' --output-on-failure`:
  **35/35 PASS,2.27s**, actual exit0. Three metadata data rows all run within their
  one registered slot. No skip, assertion inversion or failure masking.
- `ctest --preset sqlite-bundle-debug --output-on-failure`:
  **209/209 PASS,7.06s**, actual exit0. Parsed all206 Qt execution totals:
  zero failed/skipped/blacklisted. Full209 enabled CTest registration confirmed.
- Compiled -functions versus exact-label CTest JSON: exact35/35 enabled match;
  no DISABLED/WILL_FAIL. Existing required fixture diagnostics remain separate.
- git diff --check passed; independent preset comparison proves displayName-only
  change. Reviewed actual workflow diff preserves all old guards/actions/pins/
  permissions/check name and adds UNCONDITIONAL35-case execution after mandatory
  fixture/discovery guards. Local reviewer actionlint executable unavailable; no
  actionlint run claimed. Actual root/official workflow results remain separate.

Reviewer diagnostics retained under this review worktree's ignored
local-evidence/storage002-review/ (including final209-LastTest.log):

| Artifact | SHA256 |
| --- | --- |
| final configure |6e1186698bde324711973b335cf4535ceeddb0abd69c95dbd74a6ef68ac07226 |
| final build |a606ab31e94599dc6565311e5781b076d653c3c56341864b86312bfb17f7db79 |
| final35 |454ae23c04a030df6ec1878afac8523972c2837f97b337f4faeb710b68fa4bf1 |
| full209 |43811d972e14248fef8c405b079723a73e6d68aa96ec0b22914e5d8e8db9f40a |
| discovery35 |98764665442b6eeb7a1cefdf9105ea0bf12d7b2963d7bf33ee799a6904c35e34 |
| compiled35 |2006f7ef4cbfd81628897e9516ebd5968173b15bfa0b5abb746f91ac70488797 |

No unresolved source/configuration finding remains within this declared review
scope. GitHub latest-head official checks and ordinary merge gates still apply.
Storage envelopes retain synthetic/internal meanings; no semantic evaluator,
authority/permission, complete refund allocation/application service/job delivery,
provider correlation, recovery/backup/update/platform/UI/packaging proof follows.
All seven integrations/three FX providers and full release scope remain required.
