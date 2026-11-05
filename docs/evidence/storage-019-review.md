# STORAGE-019 independent decomposition and code review

5 November 2026. Reviewer `/root/capture_reviewer`, different from source author
`/root/capture_codec_tests` and root shared-file author. Isolated branch
`task/STORAGE-019-independent-code-review`, worktree
`/tmp/subathon-storage019-review`.

## Actual decomposition and claim

**APPROVED for the bounded decomposition/claim at
`36605eea826d01fff9d9f3013d82632772fbe431` only.** No source implementation has
been supplied or approved by this entry. Actual different code/configuration
review and passing acceptance/ordinary selected CI remain required.

Read root/server instructions, actual new019 brief, amended018/epic/index,
concrete supplier §1 and accepted baseline handoff/reviews. The contract expressly
permits splitting codec implementation from the dependent SQL adapter. New task
owns only eight structural SRC1/SRA1/SRX1/SRI1 codec bodies and optional source-
private borrowed helpers; no public ABI, SQL manifest/store, admission/evaluator,
authority, clock or provider behavior. Root keeps shared build/CI/state/schema
ownership. Existing27 assertions/literals and historical valid red remain fixed.
SQL31 remains separately enabled intentional-red and its entire scope is retained.

Independently queried [PR68](https://github.com/Kernol12/subathon-bot-ai/pull/68):
MERGED19:30:15Z, head6e90d4340d73749282a5b96be0cfb94e994cce27, actual merge
8435fb0888e1d1028195d955ead068c2380fe5a4. Local merge/head trees both equal
031df4288a24660c87d1c0c1606d2b7e13d56ded. The reviewed/red prerequisite is
integrated, rather than merely a done label or an unmerged claim.

Actual authoritative graph comparison with main8435 preserves all419 old IDs,
all1605 old edges and every old status. Exactly one new task and three edges give
420/1608:019→STORAGETEST018, STORAGE018→019 and EPIC009→019. Existing records
change only the explicit SQL prerequisite/epic aggregation and their explanatory
notes; no old dependency is removed or substituted. New019 spec refs use proper
§ notation. It is in_progress with explicit root/source ownership; SQL018 remains
planned and requires integrated019 before production SQL work.

Actual `python3 tools/check_project.py`, `python3 tools/progress.py render --check`
and `git diff --check` all exit0 at366:420 records/122 decisions/links/presets/
current HTML. No old red or native body was replayed. Later component completion
must pass27 codecs plus stable309, separately review actual source/config,
promote only passing27 to ordinary selected CI and preserve SQL31/future release
gates. This document will record actual source review after a written candidate
exists; this claim approval does not approve future code.

## Actual source review finding and independent regression

Read complete actual authorc2db28be/import146f0c3: CPP blob
`423f1197d05020781bc6b22b68ea4db98d149288`, private header
`9fc608d42062ad54525d47bbdc911e55bfc99028`, all704+21lines and author audit note.
Original27 passing cases were not treated as complete correctness proof.

Found a concrete decoder priority defect in `outer`: fitting empty format yielded
invalid_reference, and fitting unknown format with NULL document yielded
invalid_shape before unsupported_format. Consumer§2/concrete§3 explicitly order
outer limits/B/I→unsupported outer format→framing/decoded DTO semantics. This is
a written-contract defect, not an invented requirement or changed product choice.
Known-format NULL document must keep the original invalid_shape assertion.

Different regression author `/root/capture_fixture_author` supplied actual
`0deb60d26717dc24fa1f9d30e9c86239eaaebfb6`, root import850fd79, test CPP
`73d02569425e62b19fa23af6a5006b95c74a7825`. Independently APPROVED before
invocation: exactly one new method/eight named rows across all four decoders,
each empty format+valid independent literal body or unknown format+actual NULL
QByteArray. All fit default outer bounds. Exact unsupported_format/fixed message
is required; every original27 assertion and fixture remains unchanged. No
implementation generates an expected value.

Actual root harnessdbe8fa3 was independently APPROVED before invocation:
CMakee5460bb adds only method28; workflow62771ad requires exactly28 compiled/
registered/enabled ordinary codec cases and retains SQL31 separately intentional
red. Source423f1197 stayed held unchanged. Root then ran only this new regression
at clean dbe8fa3,19:45:12Z: direct CTest exit8, all8 real category mismatches,
2init/cleanup PASS/0skip. Independently parsed numeric-prefixed raw output to avoid
verbose double-counting: four actual invalid_reference3 and four invalid_shape4,
all expected unsupported_format7. No marker/setup/warning/compilation failure.
Raw `/tmp/storage019-priority-red.txt` SHA256
`56911fd705b32b7e0f7a7d8d94c086f82d13b06c8d5ec628ece1bc0db6de4afe`.
This reached behavior red is separate from the preserved original27 stub-red.
Only afterward was the bounded outer-order correction released to the author.

The remaining source/private-boundary audit finds no other blocker in this
candidate. Fixed typed views contain no owned buffers/tree or variable collection;
SRA scans embedded SRC directly into its nested configuration rather than a
duplicate view. Independent sizeof-only audit (no codec body invocation) gives
Rule144/Configuration288/Arrival400/Execution296/Initial184, Reader24; largest
scan view plus both outer/embedded readers448 bytes, below512. There are at most
19 actual borrowed B records and depth3. Fixed reader/sizing control stack is
separate from borrowed payload; no unbounded recursion, index array or cache.

Full-width C++ enums remain wide through tag validation before U8 emission;
all positive representable limits/reserved capacity, complete borrowed semantic
B/I, actual per-field U32 and document/output-container projections precede
ownership. Sizing covers full format+document and nested frame before Writer
allocation, with no arbitrary whole-document U32 cap. Decoder framing completes
before borrowed tag/ref/shape/value/binding checks and owned results. Nested SRC
frame/semantic bounds are dominated by the enclosing full arrival frame/DTO;
no temporary encoded envelope or eager DTO clone exists. Own allocation/length
failures map around all8 public codecs/private body paths, with RAII partial
cleanup. Private retained mode preflights actual refs under the original profile
before ownership, without fabricated format refs or a widened public-profile
decode followed by postchecks. It provides neither SQL nor admission/authority.

Final source approval awaits the actual bounded correction, passing28 and exact
final root configuration. This section does not approve an unwritten revision.

## Actual final source and configuration approval

**APPROVED for actual corrected source/configuration at
`790f757558a60409c4b90fdd808a526089c04752`.** Actual author correction
`421dc35e8638fe40848fed21a2008f37245dadb1` is imported unchanged. Re-read the
complete actual delta: bounds remain first, nonmatching outer format is checked
next, then known-format NULL body keeps invalid_shape. Only the redundant empty-
format invalid_reference branch is removed; all other scanner/validation/ownership
code is unchanged. This fixes the independently reviewed reached regression after
its actual red, while preserving the original27 assertions and behavior.

| Actual final artifact | Exact blob |
| --- | --- |
| server/src/reference_capture_values.cpp | fa3fc6d1775606604129522f6e071fd76318c70b |
| source-private reference_capture_codec_p.hpp | 9fc608d42062ad54525d47bbdc911e55bfc99028 |
| public reference_capture_values.hpp | 7824d0492e36fad80fcba3519d6538e81fcf509b |
| tests/qt/reference_capture_values_qt.cpp | 73d02569425e62b19fa23af6a5006b95c74a7825 |
| tests/qt/CMakeLists.txt | e5460bb5d07bf3475505a1f3cb88c64bf6f9897f |
| .github/workflows/sqlite-writer.yml | 62771ade6424ce2ddd1474e7d4c3ca26fcf1b797 |
| CMakePresets.json | 04cce645fb394780d0fdc91fc324aa8d048f81a4 |

Public header changes only the stale two-line implementation comment; every DTO,
limit/error/signature and historical marker class remains unchanged. Preset
changes only its display text. CI preserves exact dependency pins/check names/
permissions, requires complete28 compiled/registered/enabled codec acceptance,
rejects draft-red labels for that passing lane and executes it unconditionally.
SQL31 retains its separate enabled intentional-red guard and is not executed.
No generic failure suppression, assertion relaxation or public/private SQL bypass.

Actual reviewer validation, in isolated source review branch refreshed to this
root candidate (only this note is reviewer-authored):

```sh
cmake --preset sqlite-reference-capture-debug -DBoost_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/Boost-1.83.0 -Dboost_headers_DIR=/tmp/subathon-boost-1.83-sdk/prefix/usr/lib/x86_64-linux-gnu/cmake/boost_headers-1.83.0
cmake --build --preset sqlite-reference-capture-debug --target server_reference_capture_values_acceptance -j 4
ctest --preset sqlite-reference-capture-debug -L '^reference-capture-values$' --no-tests=error --output-on-failure --output-junit /tmp/storage019-review-codec28.xml
clang-format --dry-run --Werror server/src/reference_capture_values.cpp server/src/reference_capture_codec_p.hpp tests/qt/reference_capture_values_qt.cpp
python3 tools/check_project.py
python3 tools/progress.py render --check
git diff --check
```

All exit0. Independent fresh41-step target build, actual28 codec acceptance PASS
0.10s/0fail/disabled/skipped, including all8 additive rows. Raw reviewer output
SHA256 `7ea68f68689b17325e75d2c1c39c8e553ee15a4d8c58db8d5e7516c0ceebb910`.
Text preservation check removes exactly the new slot/data block and compares
the rest against integrated original CPPc207e423: byte-identical. YAML and24
embedded Python blocks parse; unconditional28 command/separate SQL31 guard pass
independent inspection. Project/render checks validate420 records/122 decisions/
links/presets/current HTML. Host GNU16.2.1/Qt6.12.0, selected Boost1.83.0,
CMake4.4.5/Python3.14.9.

Independently read actual root `/tmp/storage019-root-337.xml` and raw output:
337PASS/0fail/disabled/skipped,28 codec cases and ZERO SQL capture cases,24.73s.
Author separately passed fresh145-step/337 in24.74s. These are actual different
executions, not fabricated reviewer reruns of the old309 suite. Original marker
reds remain preserved, and no SQL feature body was invoked by this review.

No source/configuration blocker remains. The memory/priority/scratch conclusions
above require source inspection, not inferred test coverage. Actual selected
ordinary28 CI, latest-head publication and normal PR gates still must be observed
before final task integration. Private body helpers provide structural owned DATA
only; actual SQL metadata-before-payload integration/current authority/evaluation/
provider/recovery/UI/platform/package/full release remain separate tasks.

## Published metadata and actual ordinary selected CI

Bounded actual metadata candidate
`8d3cdcf14cd8173059c0ebb1241644d97d0460f2` is **APPROVED**. Compared the
complete790→8d changed-path set: evidence/architecture/test strategy, acceptance
map, progress/generated HTML and checkpoint only. Production, public/private
source, tests/fixtures, CMake/preset and workflow blobs remain the approved790
artifacts. Only STORAGE019's task record moves to in_review; selected_ci remains
not_run at that checkpoint, accurately pending observation. All420 IDs/1608
edges and every other task record remain unchanged. DB-01 and NUM-04 stay partial;
their appended scope distinguishes28 codec/native337 passes from SQL31 known-red
and broader authority/provider/recovery/platform/full-release obligations.
Handoff/current checkpoint preserve the actual independent regression/red/fix and
separate publication/merge gates. Project/render/whitespace checks all pass.

Independently queried actual run metadata and downloaded the FULL log for
[selected ordinary42098018635](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42098018635).
It completed SUCCESS at EXACT8d above. Full log:
`/tmp/storage019-review-official-42098018635.log`, SHA256
`5b8d6b5120b61bc7ce5d1e565b19e64cd98b68b51b3b3af8ec3692c71f041826`.

Actual verified receipt: selected Qt6.12.0/driver SQLite3.53.4 and pinned
Boost1.83; unchanged old SQL acceptance/required diagnostics pass; new foundation
four slots/6Qt PASS/0skip295ms; exact28 compiled/registered ordinary codec cases;
UNCONDITIONAL28/28 actual PASS0.10s at19:59:42Z, including the additive regression;
and exact31 enabled intentional-red SQL discovery afterward. No SQL capture body
was invoked. This is an actual passing changed-workflow observation, not a claim
derived from local green tests, earlier baseline CI or registration alone.

The initial dependency-install step was still in_progress during earlier queries;
this reviewer made no success claim then. Completed full metadata/log now supplies
that missing task-level CI evidence. Final done-status/handoff metadata, its
latest publication head's applicable checks, current rules/discussions and normal
merge remain separate root gates; success is not transferred to unseen changes.
No historical red or SQL feature body was rerun in this attribution review.

## Final component completion metadata

Actual root candidate `a0b4b10ff96405f46b76a4630c27797e5f80e4bb` is
**APPROVED for component completion metadata**. Read actual CI receipt, handoff,
native-evidence append, current coordinator checkpoint, authoritative019 status/
review record and generated progress. Compared8d→a0: only these metadata/evidence
paths and this already accepted review note change; no source, public/private
header, test/fixture, CMake/preset or workflow change. All prior source/config
approvals therefore remain applicable to the exact recorded artifacts.

Only STORAGE019 changes to done with passing tests/implementation/acceptance and
selected_ci, linked different source/config review and actual ordinary28 receipt.
STORAGE018 remains planned; platform_release stays not_run. All420 task IDs/1608
edges and every other task record are preserved. Current checkpoint expressly
distinguishes component completion from pending latest-publication CI/normal merge
and begins dependent SQL production only from integrated codecs. Old baseline and
regression receipts remain untouched; no requirement/release scope is reduced.

Independently queried exact8d checkruns:17 total, all completedSUCCESS. Selected
run42098018635 has run_attempt1, completedSUCCESS at exact8d. The slow dependency
installation completed normally, rather than a fabricated pass/cancellation waiver.
The final receipt's full-log SHA5b8d6b51 above and actual28PASS0.10/newfoundation
6QtPASS0skip295ms/old lanes/SQL31 discovery statements match independent inspection.
The clarified native shorthand means zero NEW SQL-capture feature cases; the old
SQLite suites were included and passed. It is not a claim of no SQLite execution.

Actual project/render/whitespace checks at roota0 all exit0:420 records/122
decisions/current HTML/links/presets pass. Root tree is clean before this note
import. This metadata approval and8d CI do not certify a later publication head's
checks or bypass current rules/discussion/mergeability gates. No feature body or
historical red was rerun during final attribution.
