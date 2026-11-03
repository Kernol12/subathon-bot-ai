# STORAGE-017 — independent production source/configuration review

Reviewer: bundle_reviewer, 2026-11-03. **APPROVED actual source/configuration** at
root candidate680209e67a8c4931eb5c703be3a16d7449a8dccd, after complete source
inspection, independent new18/full228 acceptance and inspection of its completed
official selected CI below. Completion-metadata/latest-head merge gates remain
separate; this review makes no future-head CI or broader product approval.

## Exact scope and artifacts

Fresh isolated task/STORAGE-017-review starts from actually integrated main
a40b42fc6d6e2a419f90f3abfd246c29dfd8173c, after PR45 merged and root claim455258c.
Imported actual author sourced675eb9e16ce87a8848fd46700f11d78b4661f0b; read actual
author evidencea9d184d/492add53745631b3603a01c913c85fce41c6717c. Root owns five
shared promotion/comment files, copied exactly into this tree for review/checks.
Only this review document is reviewer-authored. Earlier review worktrees and their
verification copies remain untouched; no original red was repeated.

| Reviewed artifact | Exact Git blob |
| --- | --- |
| Runtime bundle CPP |834336d34f532508cf35d9eabb58f8ed8f0e71bf |
| Public header, one obsolete comment removed |8614bcef00ecbcbe2f95d24751ece82b6db537ce |
| Root CMake |a713c09c2a87164d342698774cc16ba11c9a2a2f |
| Presets |0acd89ba7fc005737c208f202681b36c459a77e2 |
| Qt CMake registration |687a9d3e5ec12afebb2c29e05f2e2637c14fa8b6 |
| SQLite workflow |c5d152f6fcbd78c414cdfefd542e29db4152a420 |
| Frozen feature CPP, unchanged |0e4f4f006d2a0e161857e38255cac487f7de83a9 |
| Existing values/helper, unchanged |b0ca921cb27068ecc33228d136b82506fcb6e0a3 |
| Independent fixture JSON / helper, unchanged |bea1273cc6858c3998066e26e11e7d870a02c1bc /67d4fbd28a4c92e24c7b9b10e063638a9ba90541 |

Read root/server/tests instructions, STORAGE-017 brief, accepted logical/physical
bundle and [concrete claim contract](../contracts/sqlite-original-claims-v1.md),
frozen tests and previous full/final baseline reviews. Original clean c24 first-red
raw SHA90f3c739a735c13ca4eddecf0055ce7322cf1247e6dd52bc030e0956b1ee19b0
remains valid and preserved. Production changes wait on that already completed
baseline, rather than supplying their own expected values or replacing its red.

## Source assessment beyond test outcomes

- Public entrypoint validates ALL existing options, then known role, positive
  Receipt and exact CID presence before constructing Access/touching session SQL.
  CID0/fullUINT64_MAX remain values. Fixed selectors acquire no string-key,
  aggregate input-byte or input_items quota. Construction/copy remains inert.
- Subject selection is GLOBAL tagged kind/ID with bound typed BLOB8, no operation
  scope filter. Projected actual kind, ID and operation-key SQL classes/lengths
  are decoded strictly; predicate matching alone is not trusted. Only genuine
  subject absence permits ORIGINAL-kind receipt fallback. Source actual kind/
  Receipt/key receive the same strict decoding. Signed original corrections have
  no positive-money/effect filter.
- Selected key resolves through the SAME full existing lookup. Dangling or corrupt
  subject fails without source fallback. Actual selected operation must be
  ORIGINAL; subject kind/CID (or receipt-role ID) must match, while contribution
  subject intentionally permits a different candidate Receipt. Source selection
  checks actual Receipt and intentionally permits a different actual subject.
  Existing own-claim identity checks remain. Absence returns after bounded selector
  reads, without candidate admission/current state or unrelated namespace audit.
- Full existing lookup preserves immutable source/plan/historical/current-CID
  identity closure, three snapshots, Exact/count/group/ordinal/conservation,
  result/audit and bounded64/65 structural history checks. It does not infer fresh
  policy, revalue or compare historical data with today's mutable dimensions.
  No old stage Intent equality, reversal semantics, schema or producer changes.
- The only old lookup edit extracts its exact owned-record field walk. Independently
  compared every normalized line with old source130: identical order, members and
  charges. Full lookup still checks actual returned vector totals and complete body
  bytes first. New wrapper uses that SAME walk and checked ByteCount.add(1), zero
  extra vector elements, after full closure and selected identity. No subtract-one
  option mutation, quota reset, duplicated accounting interpretation or overflow.
- Every selector/full-lookup statement uses the same actual session and cumulative
  writer budget. Existing execute maps only already-aborted known writer failures;
  local selected/result failures use actual whole-unit rollback before exposing
  known storage failure. Unknown rollback remains original WorkFailure and outranks
  local categorization. Capability/protocol errors propagate unchanged. New code
  adds no cleanup after an already terminal failure and no own BEGIN/COMMIT,
  connection, adoption, healthy-read/destructor cleanup, replay or publication.
  Fully owned return values remain provisional until caller commit/completion.

No blocking source finding. Root's actual header correction removes only the
obsolete stub-status comment; API types/signatures/layout are otherwise byte-exact.
All prior tests, fixture data, schema/manifests and helper remain unchanged.

## Promotion and independent actual verification

Inspected all five root-owned diffs. CMake option description and preset display
only become acceptance wording. Qt registration preserves all18 names and only
removes their draft label/updates its comment. Workflow retains old steps,
dependency/archive pins, check names, exact compiled guards and required foundation
diagnostics, then adds UNCONDITIONAL new18 acceptance after its exact compiled/
enabled-registration guard. No disabled/failure inversion/suppression/manual toggle.
This workflow promotion does not itself create a GitHub required-status-check rule.

Linux x86-64, GNU16.2.1/C++20, CMake4.4.5/Ninja1.13.2, Boost1.92,
QtCore/Sql/Test6.12.0, host SQLite3.54.0. Actual isolated commands/results:

- `cmake --preset sqlite-original-claim-debug`: PASS.
- `cmake --build --preset sqlite-original-claim-debug --parallel 2`: fresh100
  steps, ZERO warning/error diagnostics; final no-op confirmation exit0.
- `ctest --preset sqlite-original-claim-debug -L '^sqlite-original-claim$' --verbose`:
  **18/18 PASS,0.82s**, all35 scenario rows plus init/cleanup give71 Qt passes,
 0failed/skipped/blacklisted. Exact162/161 and161/160 SUBJECT/SOURCE boundaries,
  full20/19, real selected corruption, staged ownership and actual uncertainty pass.
- `ctest --preset sqlite-original-claim-debug --verbose`: **228/228 PASS,7.85s**;
 225 Qt processes/715 Qt passes,0failed/skipped/blacklisted. Old35 bundle remains
  included, with all earlier native/fixture assertions preserved.
- Compiled `-functions` EXACT18 equals enabled CTest label-selected18; global228.
  JSON properties show no draft/disabled/WILL_FAIL on new18. Actual reviewed
  source/config/test tree equals root680209e for relevant paths.
- `python3 tools/check_project.py`: PASS396 records/122 decisions/links/presets/
  current HTML. `python3 tools/progress.py render --check`, `git diff --check`: PASS.

The local log checker first expected older CTest's longer summary wording;
correcting that parser confirmed these recorded results without rerunning suites.
Raw reviewer diagnostics stay under ignored local-evidence/storage017-review:

| File | SHA256 |
| --- | --- |
| configure.txt |e80929a901bf5b38949e24715353e4909bd36c2093cb881fe585bd7e93b0e8d5 |
| build.txt |b80b2af15bd14dd3272ce2c6ef14dbefb1c915e8fa6c2cc6c8cbaefc8c3c5f29 |
| build-confirmation.txt |8c35b1682f73592d30612d7b0fabdb384fcfff35cb5a076cb48b85b1368aa794 |
| targeted.txt |43deec56c144140db099ab31cd19e94bbf7a67fcea1944f05e2176ab471218e6 |
| full.txt |b295a0610661f1ac1b0ba0fcf27ab387fb08a08d8bb0811dd0f13a8e19f61f15 |
| functions.txt |896797a7eef57019b222159e37cd81719e56cf31cf030e0fb1111f631d92cf11 |
| discovery.json |eff0722d3634d25581db9ca807ca878ceed82b5994988c69f86494d93820b159 |
| all-discovery.json |0e8b4fcaa8db3d96db080e9652e59b1dfabafad94820fd61e58e91e34ef90c5d |

## Actual source-revision official CI

Also read completed [SQL41605888462](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41605888462)
log/result and final PR rollup: exact source/config head680209e,08:47:22–08:48:51Z,
all11 COMPLETED/SUCCESS. Checkout identifies its PR merge266f63f into maina40.
Actual fresh100/ZEROwarnings/GNU13.3/selectedBoost108300/verifiedofficialQt6.12/
SQLite3.53.4, required3 fixture guard+5PASS0skip61ms, compiled18 guard,
UNCONDITIONAL18 PASS1.29s and old stable suites including35 PASS are present.
Raw log SHA2569fa9ed917b4c489cdbcc87be6b3c0c29ffe768ba18828f5d16fe6e7819ae3f1e;
completed result23b8a933f35dcc169e56528e405f313fd7f18c7754720e10760035fb6b27c3ff;
final rollup3c6c4f724e9807f63b965d9139bdcfaf9cb9b7452822247ac2873807c8482be7.
These actual source-revision outcomes do not establish future completion-head CI.

Source/config acceptance approval is complete. Root must still satisfy actual
completion-evidence and all applicable latest review/discussion/merge gates.
Hold016, service004/current access/command policy,
providers/FX, delivery/recovery/backup/UI/platform and full agreed release remain
separate required work. No live-provider effect or release completion is claimed.
