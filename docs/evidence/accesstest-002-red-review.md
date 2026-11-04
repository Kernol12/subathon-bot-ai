# ACCESSTEST-002 independent FIRST red attribution

Reviewer `/root/codec_author`, 2026-11-04, separate from author `service_author`
and root's execution. Clean review branch starts at actual evidence commit
`34cc66db4f4b3af05e1cd0f93914f03161caed4f`. Only this note changes.

**VALID intended behavioral red.** All 30 reviewed cases executed and failed
because the sole evaluator still immediately throws
`EffectivePermissionsNotImplemented`. No unrelated failure invalidates the run.
This is failing-baseline attribution, not passing acceptance, production or merge.

The [receipt](accesstest-002-red.json) and [run note](accesstest-002-red.md) record
clean execution head `8acc9b9cf88bb67762c2060ee5fb16a5345a273f`,
2026-11-04T18:24:09.993522Z–18:24:10.048423Z, core dumps disabled, command:

```sh
ctest --preset effective-permissions-debug -L '^effective-permissions$' --no-tests=error --output-on-failure --output-junit /tmp/subathon-accesstest002-first-red/red.xml
```

Actual exit 8; XML reports 30 tests/30 failures/0 disabled/0 skipped. Each exact
inventory name has its own GTest RUN/FAILED output and failed CTest status. All 30
source/inventory/XML names agree, with the reviewed CTest prefix. The 0.054901-second
recorded process interval and XML's 19:24:10 local timestamp are consistent.

Independently rehashed the [full output](accesstest-002-red.txt) and
[JUnit](accesstest-002-red.xml), and compared them byte-for-byte with the original
scratch files; the receipt copy is identical too:

| Artifact | SHA256 |
| --- | --- |
| Output | `e1598fa3d276c23a4757f49abfbd3dec0c221cb59726b8387974ffc058e2d06c` |
| JUnit | `5f3872f2cddec5f17019ebf9e53b65131893d90b74c2f1dd6426431eb5741c1c` |
| Receipt | `4a51a73c70c5f37c60a6dfc8776806fe5b3f1b8d89f99db685420a24d99e642f` |

All four receipt source blobs match the execution head and current review tree:
tests `fc82d17ede5d2afe66e42572c17762510e5682f4`, inventory
`d0c25108ec233cdc11260aa1263974805b0fba15`, header
`edd6ffc25379e5036d5e1622346eaa3f272a1c5c`, immediate stub
`4cf7bcca19e5ab879b62c70c1521d9f977abfd3b`. No source, input or oracle was
edited for this run; fixture validity is supported by the preceding full test
review, rather than inferred from an unimplemented evaluator.

I inspected every failure location: 63 decision-helper failures at line 130,
143 wrong-exception-helper failures at 144, and nine direct catch failures at
533/629/663/693/750/797/817. All 215 contain the exact approved marker what-text.
Those catches follow actual evaluator calls; the frozen evaluator has only its
immediate distinct marker throw. This accounts for the marker's causal source.
The sole remaining failure, line 820, is `ASSERT_TRUE(retained.has_value())` after
the lifetime call's marker at 817. It safely returns before dereferencing the
absent result; it is a downstream consequence, not a second defect. Thus all 216
failure locations are accounted for. No setup/dependency/load/timeout/crash/skip
failure, successful marker catch, fixture rewrite or failure inversion is present.

Chronology checks confirm test approval `1d8e9335`/import `6805011`, harness approval
`7d7f651`/import `ddb6fac` and actual label correction approval
`ebe17a9e`/import `8acc9b9` precede execution. Corrected selected Ubuntu CI
[41886146750](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41886146750)
at `4674822` has the same source/build/workflow artifacts as the execution head.
Its retained log confirms exact 30-case enabled discovery, SDK 21 facts/1 passing
environment and old 36 passing checks, with no permission body. The earlier label
guard failure remains historical infrastructure evidence, not behavioral red.

Actual checks were read-only Git history/blob comparisons and Python stdlib full
XML/output/hash/inventory/failure-location assertions; all passed. No red, compiler,
discovery, SDK/foundation or feature body was rerun. `python3 tools/check_project.py`,
`python3 tools/progress.py render --check` and `git diff --check` passed: 406 tasks/
122 decisions, links/presets/current HTML and whitespace. Sole-note staged scope/
whitespace was checked before commit. Root owns final metadata, latest-head CI/review/
discussion/rules checks and normal merge. ACCESS-002 implementation, separate code
review/passing acceptance and account/session/delegation integration remain later.
