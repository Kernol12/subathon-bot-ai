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
