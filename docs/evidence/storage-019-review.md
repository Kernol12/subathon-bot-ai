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
