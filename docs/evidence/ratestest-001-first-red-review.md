# RATESTEST-001 independent FIRST-red attribution

**APPROVED — valid intended missing-behavior red, with the capture limitation below.**
Reviewer `service_reviewer`, different from original test author `codec_author`,
2026-11-04. Independently inspected actual recovery candidate
`d3b94cdc6747fb96bfd291e49017b68b3e29f585` in isolated
`task/RATESTEST-001-first-red-review`. Only this review note changes.

The single recorded FIRST feature command ran on clean
`050e5660ed5c5afd962fda32806ba12640841a8a`:

```sh
ctest --preset rate-snapshot-debug -L '^rate-snapshot$' --no-tests=error \
  --output-on-failure --output-junit /tmp/subathon-ratestest001-first-red/first-red.xml
```

The [native JUnit](ratestest-001-first-red.xml) parses directly: **53 tests / 53
failures / 0 disabled / 0 skipped**, timestamp `2026-11-04T20:39:46` CET
(`19:39:46Z` derived). The [raw LastTest](ratestest-001-first-red.raw.log) contains
all 53 complete executions and its normal end-of-run footer; each case records
`Test Failed`, one executed GTest and zero passing tests. Their exact unique names
match the frozen 53 TEST declarations, JUnit, receipt and
[LastTestsFailed inventory](ratestest-001-first-red.failed.txt). Each JUnit case
has fail status, one failure, no skip and the exact four required labels.

## Actual failure attribution

Inspected every raw GTest failure block and its source-location trace against the
unchanged acceptance source and four immediate stub bodies. **All 364 locations**
explicitly contain `RateSnapshotNotImplemented` and its exact module marker.
There are 91 `EXPECT_NO_THROW` failures at helper line 44 and 273 wrong-exception
`EXPECT_THROW` failures at helper line 28, covering 141 distinct traced call sites.
The latter expect `RateSnapshotFailure`; the marker is an ordinary nonfatal
failure, never caught as a passing expected domain error.

| Actual reached entrypoint | Marker locations |
| --- | ---: |
| make_rate_money | 61 |
| make_fetched_rate_observation | 94 |
| make_effective_rate_snapshot | 155 |
| quote_effective_rate | 54 |
| Total | 364 |

All observed failures are attributable to these missing feature bodies. No native
setup/link/dependency/SDK failure, unexpected assertion category, disabled case,
skip or failure inversion substitutes for red. The 13-option, two-invalid-values,
four-entrypoint loop contributes its complete 104 markers. The additive mixed
ASCII/high-byte scenario separately reaches its own accepts call at source line
538 after the earlier scenario at 519. Nonfatal helpers preserve later independent
rows; assertions and additional sequential calls inside a throwing lambda do not
all execute. This attribution does not claim that those later branches or expected
success behavior were observed.

## Capture defect and preservation

Root records a Python `subprocess.run(text=True)` decoding exception after child
output collection, with **wrapper exit 1**. The **direct CTest exit status was not
retained and remains unknown**; this review does not infer exit 8. The collector
exception is not counted as a feature failure. Independently confirmed that the
native log has exactly one invalid UTF-8 byte, `0xff` at zero-based offset 11271.
The [readable projection](ratestest-001-first-red.txt) is exactly raw UTF-8 decoding
with `backslashreplace`; the raw bytes remain available.

| Independently rehashed retained artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Native LastTest | 732918 | bf5f5c5a9e612884ba7c4be3ddcbd7cc2daa1c066564d7ace7613f742ac2d0cf |
| Native JUnit | 693116 | 53a3f2806fcd309dd8d28db38245a4d0a6eb533e8032d9e77df1f53511af57f4 |
| Readable LastTest | 732921 | fa5c9e1056234fd58d776bb418e69349cd1a991319a5159884a4c5ef2f3776c9 |
| Native failed inventory | 4230 | 75d27327f3bda79bf7070b704c3bbb49407123f976ae154a56aff361f4878853 |

All six original scratch artifacts, including the cost file and duplicate JUnit,
match the [recovery receipt](ratestest-001-first-red.json). Receipt bytes equal the
retained scratch receipt: SHA256
`30bc9d7bb8b70341b1ed55a20f88fa1f54a4be3b099a71b155e500fe84748036`, Git blob
`c0bea24b04fe024835d03d6298ef72f3790137a2`. Root records preservation before any
further CTest command; no replay was used for this review. Complete independent
native case/failure records establish intended red despite the missing aggregate
process status. No supplemental execution is required to establish those outcomes.

## Prerequisites, integrity and limits

Full test approval `dcf5dcaaf8c33d76223278149b018d56efa5a50c` precedes FIRST.
Acceptance `64fd60b27359db8ba7b3d17f77f8e349ae7e30f9`, vectors
`8936f4fae9bd17d5e592256e0c395871bcea4849`, header
`3a06ff18d29c3bd2c5953e6747130aa84cbee8f5` and immediate-stub TU
`4cf1c1ed6b54efe3625baf97903de19d3ebacf8d` are identical at that reviewed source,
selected CI, FIRST and current candidate. Root/main/config approval `7ae17c55`,
imported as `050e566`, preceded FIRST. All six relevant build/label/workflow blobs
are unchanged between selected CI, FIRST and this recovery commit; the subsequent
diff contains only evidence/state/generated HTML, not source/assertion/oracle changes.

Read the existing host receipt and retained selected CI log, without replaying
SDK/authentication/source reviews. Different selection approval `a5963b39` precedes
host 1.83 foundation 14/14 at `19:11:53Z`, receipt `f070ca7f`. Selected Ubuntu
[41897596827](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41897596827)
at `bf5986d5ae418fb2c921b147b18927aa3f3ae996` records macro 108300, exactly 53 enabled
discovered feature cases, foundation 14 facts / 1 PASS and old 36 PASS, completing
at `19:31:08Z`, before FIRST. That green lane did not execute the rate feature bodies.
Retained CI log SHA256 is
`0b1b659e6943a6a29ad4d569960b4d9562d018c211f87d780ce35709772f91ff`.
The unrelated apt HTTP403/retried SQLite job is separately recorded, not a rate
feature failure or grounds to change its baseline.

Binary-safe Python parsing/hash/source-trace checks, `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` passed. This reviewer
ran no CTest, test/native/SDK/foundation/feature/marker body, generator or new build.
Approval concerns this observed-red attribution only. It supplies no production
RATES-001, allocation/valuation-bridge, provider/platform or future CI/PR/merge
approval. Final metadata, current-head checks and normal merge gates remain root's
work; RATESTEST-009/RATES-009 and full release scope remain required.
