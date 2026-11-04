# MILESTONETEST-001 independent FIRST-red attribution

**APPROVED — actual intended missing-behavior red.** Reviewer `service_reviewer`,
different from original test author `service_author`, 2026-11-04. Inspected candidate
`e0d465794f7bc55030b82cfbafe20d659b39df1a` in isolated
`task/MILESTONETEST-001-first-red-review`; only this note changes.

The [receipt](milestonetest-001-first-red.json), exact scratch return-code file and
native outputs retain the FIRST command on clean
`c2bcb08837aa3e324a153d7d31a71298c7a74a3d`,
`20:36:28.318352Z`–`20:36:28.417446Z`:

```sh
ctest --preset milestone-progression-debug -L '^milestone-progression$' --no-tests=error --output-on-failure --output-junit /tmp/subathon-milestonetest001-first-red/first-red.xml
```

**Direct CTest exit 8; 53 executed / 53 failed / 0 disabled / 0 skipped.** Native
[JUnit](milestonetest-001-first-red.xml) parses directly, timestamp 21:36:28 CET
(20:36:28Z). The exact unique case set equals all 33 ordinary TEST declarations plus
all 20 raw named IndependentMP instances, preserving NO_PRETTY_VALUES names. Every
case has fail status, one JUnit failure, its complete GTest execution and the exact
four required labels. Raw CTest stdout reports those same failures, not missing
executables, collection failure or a passing expectation for an unimplemented body.

## All actual failure locations

Binary-safe parsing and source inspection attribute **110 GTest failure events** to
the unchanged sole immediate `MilestoneProgressionNotImplemented` throw:

| Actual category | Count | Attribution |
| --- | ---: | --- |
| Uncaught test-body marker | 12 | GTest unknown-file exception report; direct projector call fails before result inspection |
| Wrong-exception helper marker | 62 | Source line 147; helper expects MilestoneProgressionFailure and reports the distinct marker instead |
| EXPECT_NO_THROW marker | 36 | Line 190: 20 literal parameter rows; line 393: 6 reference-boundary rows; lines424/435/445/508/526/538/551/565/576/584:1 each |
| Total | 110 | Every failure contains the exact module marker |

All 53 cases contain at least one explicit marker; all observed failure blocks
contain exactly one. Parameter-row trace labels match their raw test names. Loop
multiplicities match source: seven required-reference refusals, three duplicate
rows, three target-order rows, five prefix rows plus the empty-target prefix case,
eight phase-priority rows, six accepted reference positions and all five options
at zero/max+1/size-max (15). Nonfatal refusal/EXPECT_NO_THROW checks allow later
independent rows to reach the projector. The sole feature therefore reaches its
missing-behavior throw on all 110 observed invocations.

**No consequential assertion failure or unrelated failure occurred.** Native
stderr is only `Errors while running CTest`. The 12 uncaught-body markers end those
individual tests: later result, mutation/destruction, copy/move, second-proposal or
refusal assertions in those bodies did not run. Other assertions after each
throwing EXPECT_NO_THROW block likewise did not all execute. This review does not
claim those downstream success assertions passed or every later call in a body ran.
No setup, dependency, SDK, fixture-construction, timeout/crash, skip or inversion
substitutes for the actual marker failures.

## Preserved evidence and prerequisite order

| Independently rehashed native artifact | Bytes | SHA256 |
| --- | ---: | --- |
| [stdout bytes](milestonetest-001-first-red.raw.log) | 149665 | 0a59458091a6b322a755773d5a2bdba1594c09d125a412ec1ab3c29688ec0ec3 |
| [stderr bytes](milestonetest-001-first-red.stderr.log) | 27 | 18e00877dc1071c85cd4f479d451bebeef8309022e46cf8c63b0bed5151984a4 |
| Native JUnit | 151631 | a6d93ccf21070718fe317243bd493d14a89ab110266f6eed0f3e44dd4a902391 |
| Original returncode.txt (`8` plus LF) | 2 | aa67a169b0bba217aa0aa88a65346920c84c42447c36ba5f7ea65f422c1fe5d8 |

Every original `/tmp/subathon-milestonetest001-first-red` artifact matches receipt
sizes/hashes. The [readable stdout](milestonetest-001-first-red.txt) is exactly raw
UTF-8 decoding with backslashreplace. This particular stdout has no high bytes;
opaque high-byte INPUT fixtures do not mean those bytes appear in the stub log.
Byte-first capture retains the direct status before decoding, with no collector
loss. A wrapper returning after recording failure does not invert that native red.
Receipt Git blob is `e584300bc946b16db05e8b621062cb9dc0efff00`.

Full test approval `208d058058be90ab1016a5c7d1fe75ed345ae521` and actual changed
harness approval `75e62c2a`/import `c2bcb088` precede FIRST. Test
`019973844dceb289aa82ec8aabd3d9274faef98e`, vectors
`409be19d57b2cba4025e236a67a539af63b08be4`, header
`9a1678137ba6f85cbfbb545630f08d36a261790b` and immediate-stub TU
`4e3291f0e832237470a8cd0bc349e0106ad8fd77` are identical at full review, selected
CI, FIRST and this evidence candidate. Six reviewed build/label/workflow blobs
are identical between selected CI, FIRST and this candidate. The post-FIRST diff
adds only evidence, changing no assertion, fixture or implementation.

Read retained selected Ubuntu [41907483021](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41907483021)
log at `a15a70eec1d86657b483414e0aa99f7fa439fddd`: exact enabled 53-case guard and
37 green checks (separate reviewed 25-fact Boost1.83 foundation plus old 36), complete
at 20:32:26Z, before FIRST. No feature body runs in that green lane. Log SHA256
`8ecc6abc0194a6e0c14bcdc3e5952d43b3cd29ba3ab7def2604ef7e16fa27d1b`.
The prior selected-host proof and original source/test/foundation reviews are
preserved, not repeated by this attribution.

Read-only Python XML/binary/hash/source-set/trace comparisons,
`python3 tools/check_project.py`, `python3 tools/progress.py render --check` and
`git diff --check` pass. This reviewer ran no CTest, SDK/foundation/native/test/
feature/marker body, build or red replay. Approval is only this actual observed-red
attribution, not milestone production, later metadata/CI/merge, actions/history
transaction authority, UI/storage/platform or release acceptance. Root still owns
final metadata review and fresh normal publication/merge gates.
