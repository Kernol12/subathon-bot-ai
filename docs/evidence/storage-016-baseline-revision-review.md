# STORAGE-016 — independent baseline revision review

2026-11-03, bundle_reviewer. **APPROVED** actual ROOT-authored history correction
1a32ed8 and additive regression70c430f; root is separate from both production
authors. Final test blob `82980fdba7d538e90c66608afd4e2fe45f229cff`,
[revision evidence](storage-016-baseline-revision.md) blob
`f79a3326b7bf86f191f105894b0ff1659ecbf9bf`. Read both complete diffs, that note,
logical701/physical83e/API46c, frozen history/source values and actual GDB3 evidence.
This approves the concrete revision, not unexamined source fixes or future runs.

The earlier full test review missed a genuine history request defect. Frozen
old_resolution owns ONLY hOld position2, checked v2, no-more,338 bytes/6 items.
The unchanged fixture has real initial position1/dOld. Contract history selects
positions STRICTLY AFTER cursor, so cursor0 must return1 AND2. Changing ONLY the
two request cursors0→1 selects that existing literal resolution-only oracle; the
input cursor still costs8 bytes. Every expected DTO/338/6 fit/one-under/full equality
assertion remains. No implementation may suppress position1 to satisfy the old
request. Source-author GDB3 confirms query hOld/0/64 returned positions1,2; the
expected oracle comes from the contract/fixture, not that observed result.

Three additive rows preserve all prior rows, assertions and helper machinery:

- required-audit-before-Exact: dOverflow keys/counts remain valid, its attempt audit
  document is null, and component_bytes2 makes the existing19-digit numeric failure
  Exact oversized. Required-envelope invalid_detection8 must precede Exact bounds.
- replacement-audit-before-set-order: the full frozen rReplace request has two
  valid distinct expected locators and an actual owned replacement. Reversing that
  set and nulling the replacement audit isolates earlier required-envelope8 versus
  later local ordering. The new selector correctly dispatches stage_resolution.
  Neither row imposes an order among unrelated same-stage defects; both use the
  existing pure-error helper and prove caller staging can still commit.
- narrow-supersedes-first-producer-mismatch: hOld's declared first becomes existing
  kind0/same-CIDMAX dFail, which actually belongs to hFail; hOld's real first version
  remains produced by dOld. Only first_detection_ref changes. Explicit changed-row
  and FK0 checks precede narrow find(hNew). Physical83e requires this referenced
  first-producer scalar link, not a sibling full document audit. The expected
  invalid_stored_data uses the existing inside-catch real rollback/snapshot checks.

Independently executed ONLY a stdlib SQLite3.54 disposable SQL setup diagnostic:
approved pending literals plus the exact ancestor UPDATE affect1 row, FK0; actual
typed dFail kind/subject/hold and dOld position1 establish the intended mismatch.
No public hold entry or new test/data body was run by the reviewer. Raw report
local-evidence/storage016-review/ancestor-regression-setup.json SHA256
`a33784be468a1a3770518b18a8d58bf7e54426e7afa47609acf753ca7b656c10`.

Root actual compile/discovery logs were inspected: linked revised test source,
ZEROwarnings, exactly35 compiled/registered names agree; no new slot, DISABLED or
failure inversion. Static data count is265→268. The frozen JSON63de09c5/helper7ca946f2/
environmenta17312fe and root harness remain unchanged. Source implementation is
not used to generate expectations. Actual root log SHA256:

| local-evidence artifact | SHA256 |
| --- | --- |
| storage016-baseline-revision-build.txt |27ea3b9705621f5aea6252a986c25fdb0f31ba41c4fcf38d72381cec761c2ea0 |
| storage016-baseline-revision-functions.txt |94056a922e885d2c658367be53890097a6d8ca043fa83d2252bc4ce5fcaf965f |
| storage016-baseline-revision-discovery.json |0e3c7891520c766be4aa6eb19921addbbf05f73b192d520b7f428d79f3a2190d |

Original valid265-marker red598, invalid earlier attempts and valid independent
reviews remain preserved; no full original baseline replay is required. Root must
run ONLY the actual new regression rows on unchanged buggy0fe/a18 and record their
reached intended failures BEFORE releasing the affected priority/lineage source
fixes. Corrected history cases should pass already contract-correct behavior;
do not manufacture a red for them. Full final runtime/helper/factoring/allocation/
config review,35/full264 acceptance (now268 Qt scenarios) and ordinary selected CI
remain implementation gates. No source or future result is approved here.

## Actual regression-red attribution addendum

Independently inspected the completed raw output/JSON record, actual Git objects,
root build log and read-only GDB log; **the three reviewed regressions reached
their intended behavior failures BEFORE source correction**. Recorded clean head
68dc77916e738085c78b0ec398d673d52b341466 pins unchanged test82980fdb,
runtime97c83b746fe1507670b03bf5430c45231facf551 and helpera18a0bd7, after
independent test approval1650658. Actual three function:data invocations at
14:20:25.413–14:20:25.514Z each exit1,2 init/cleanup PASS/1FAIL/0skip/blacklist,
0HOLD-MISSING. No original full baseline was replayed.

Both priority rows fail exact input-category assertions, consistent with the
independent actual11/9-versus-required8 diagnostic. The narrow row's generic
refusal conjunction alone is ambiguous: GDB at actual test319 proves body returned
normally and caught=false before the deliberate missing-storage-failure throw.
This establishes absent refusal, NOT changed durable bytes. The concrete stack
and selected data tag identify the intended narrow-ancestry regression.

Combined tracked-output SHA256231d325ac11f7ce201ff3b17146968a1d39dd5d5135714b060e5287f2da1bcfc;
individual53c581b46ef186892d1e2d8cc37b3916e71fb11a443302f7d4c17c14faacd43d,
3a43ab29304e374697a89b505498b678f8602564d6464194da0c1c19529a4927,
030ee26e13e5e960241e74db7a91cfa9ae2b8b2fe781e6a7d66cb8ce40aadd69;
GDB9dc953c9cbf4d1412c990a66757ac89cbc7b80a6a429984668e2a0206b5bea04.
The corresponding regression-red-record.json SHA256 is
c8d5057797d119707cd228bd81ce909064bdd34afae608cd80cdc2c87cfcb8f0.

No reviewer replay or source/test change. The observed regression gate supports
the two bounded fixes; actual changed helper/runtime/factoring/promotion and
passing full acceptance still require review. No prospective implementation approval.
