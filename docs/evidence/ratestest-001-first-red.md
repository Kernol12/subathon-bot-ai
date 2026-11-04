# RATESTEST-001 first observed rate feature red and binary-safe recovery

Root ran the reviewed rate suite once on clean
`050e5660ed5c5afd962fda32806ba12640841a8a`. Native JUnit records local
2026-11-04T20:39:46 CET (19:39:46 UTC derived),53 tests/53 failures/0disabled/0skipped.
[Native JUnit](ratestest-001-first-red.xml),
[exact native log](ratestest-001-first-red.raw.log),
[readable log](ratestest-001-first-red.txt),
[failed-case inventory](ratestest-001-first-red.failed.txt),
[recovery receipt](ratestest-001-first-red.json).

```sh
ctest --preset rate-snapshot-debug -L '^rate-snapshot$' --no-tests=error \
  --output-on-failure --output-junit /tmp/subathon-ratestest001-first-red/first-red.xml
```

**Collector defect, kept separate from behavioral red:** the enclosing Python used
`subprocess.run(text=True)`. A deliberate fixture trace contains byte0xff; UTF-8
decoding raised UnicodeDecodeError after child output collection. The Python
wrapper exited1. No direct CTest exit status was retained, so none is claimed.
The capture exception itself is not a feature-test failure or red evidence.

Before any further CTest command, root copied the actual native LastTest.log,
LastTestsFailed.log and JUnit bytes. Native XML parses directly and shows53 complete
failed cases, each containing the explicit rate unimplemented marker. LastTestsFailed
has exactly53 entries. Native LastTest has364 marker occurrences and364 GTest failure
locations. All four production entrypoints are unchanged immediate marker throws.
Different actual attribution is required to validate every location and the recovered
run; root does not substitute a wrapper exception for that review. No run was replayed.

The readable log uses UTF-8 decoding with `errors=backslashreplace`; the raw log is
retained byte-for-byte, including its one0xff byte. No input/assertion/oracle/expected
value changed. Full hashes and raw sizes are in the receipt. Native log SHA256
bf5f5c5a9e612884ba7c4be3ddcbd7cc2daa1c066564d7ace7613f742ac2d0cf;
JUnit53a3f2806fcd309dd8d28db38245a4d0a6eb533e8032d9e77df1f53511af57f4.
The missing direct command status is an explicit evidence limit; complete native
case outcomes, rather than an invented exit8, support the observed-red claim.

Prerequisites actually preceded the run: full53 test approvaldcf5dcaa; unchanged
test64fd60b2/fixture8936f4fa/header3a06ff18/stub4cf1c1ed; new root integration/
harness approval7ae17c55/import050e566. First selected host Boost1.83 foundation14
passed after actual different selection review. Selected Ubuntu
[run41897596827](https://github.com/Kernol12/subathon-bot-ai/actions/runs/41897596827)
atbf5986d passed exact53 enabled discovery, independent real1.83 foundation14/1of1
andold36. No feature body ran in green CI. An unrelated existing SQLite job failed
an apt Microsoft-index HTTP403 before build; only that job was retried normally and
attempt2 passed. No signature/test/protection gate was weakened.

GCC16.2.1/GTest1.18.0/authentic explicit Boost1.83/CMake4.4.5/Ninja1.13.2, core dumps
disabled. No provider/network or RATES-001 implementation ran. Independent attribution,
final metadata/current-head CI/discussions/protection/normal merge remain. Do not
replay FIRST or claim a direct exit status that was not recorded.
