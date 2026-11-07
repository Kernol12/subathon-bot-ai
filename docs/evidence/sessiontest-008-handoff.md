# SESSIONTEST-008 — reviewed supplier baseline and observed red

Coordinator `/root`, 7 November 2026. Branch
`task/SESSIONTEST-008-supplier-baseline`, `/tmp/subathon-sessiontest008`.
[PR82](https://github.com/Kernol12/subathon-bot-ai/pull/82).
[Brief](../tasks/EPIC-007/SESSION-008.md), accepted supplier contract blob
`c5f5a6e71664042e18475068d3caf577b0c8d009`.

## Result

Fresh author `session_supplier_baseline_author` wrote `55896ce6`, imported as
`2451e50`: the exact minimal API, two immediate `std::logic_error` markers,
39 independent tests, literal vectors, separate SDK foundation and closed passive
43-case harness. [Author evidence](sessiontest-008-author.md).
No token/session behavior is implemented. Existing assertions, dependency pins,
contracts and previous receipts remain unchanged.

Different reviewer `session_supplier_baseline_review` approved exact full packet
`f9ca39a` in `2014c0c` before any new body ran, including private error-only native
observers, source-derived oracles, limits, API/stubs/harness and CI.
[Review and attribution](sessiontest-008-review.md). Its native review bounds the
selected libsodium-owned hash work/stack separately from application124 bytes and
external libc/loader/first-init/global process/platform guarantees.

## First executions and independent attribution

Root executed these once, at clean review revision
`2014c0c56db1d46a1206cc7716dbb1a1c2382136`:

1. `ctest --preset session-resolution-debug -R '^scaffold.session_resolution_environment$' --no-tests=error --verbose --output-junit /tmp/subathon-sessiontest008-first-sdk/ctest.xml`
   returned0: exactly one SDK PASS, runtime1.0.22/ABI26/4, three independent framed
   BLAKE2b vectors, fixed32 comparison and clearing. No feature body ran.
2. Only after that actual green, `ctest --preset session-resolution-debug -L '^session-resolution$' --no-tests=error --verbose --output-junit /tmp/subathon-sessiontest008-first-red/ctest.xml`
   returned8:39 cases/39 failures/0 errors/0 skips against unchanged immediate stubs.

[Immutable first-run originals](sessiontest-008-first-runs.tar.gz) contain native
JUnit, LastTest, failed inventory, raw stdout/stderr, direct exits, revisions,
executable/SDK hashes and chronology, captured before another CTest could overwrite
them. Same different reviewer `6bef738` / import `6cbcec5` verified all ten file
hashes and all39 cases' sole intended marker: LastTest39, stdout78 echoes, no setup,
crash, timeout, skip or unrelated assertion failure. No body was replayed.
Assertions and later loop rows after each immediate throw were not executed and
are not passing coverage.

Core limit0 is recorded. Root's tool command supplied a minimal PATH/locale;
those variable values are absent from the original receipt JSON and were not
independently certified or filled afterward. This evidence limit does not invent
unknown fields or justify replaying the first run.

## Dependency and hosted CI evidence

[Immutable SDK preparation](sessiontest-008-sdk-preparation.tar.gz) retains the
exact admitted source archive2008529 bytes/SHA
`adbdd8f16149e81ac6078a03aca6fc03b592b89ef7b5ed83841c086191be3349`,
configure/build/install receipts,678 preserved vendor files and raw logs. New local
static archive836704 bytes/SHA
`f8e295ad9a6d8c6d3c657e4d2f33d590da161f6ed5a81dfae09d1067fab3d017`.
The source/build used the existing admitted dependency; no new pin or global install.
Configure's three compiler probes returned0/0/134 with core limit0, including a
SIGABRT/core report; external crash-handler retention was not inspected. Eight
Libtool convenience warnings, zero build compiler errors. These were SDK build
observations, not product red. The first archive-packaging attempt6da7450 was empty
after an overly strict nested-path check; correctedc22a47c validates every source
and archived hash. Original SDK receipts remain untouched.

[Hosted run42363172584](https://github.com/Kernol12/subathon-bot-ai/actions/runs/42363172584)
at exact840ed5f64a70e30b4b9a82dffee2e4f35b9bd1d8 passed the approved workflow.
[Actual receipt](sessiontest-008-hosted-ci.json) records its new build, exact closed
39-feature discovery and separate SDK1PASS output. The workflow runs ONLY stable
SDK readiness plus the existing passing password suite; intentional feature red
stays separate, with no inversion or skipping. The collector excluded Actions'
colored source-command echo when matching emitted output; no CI rerun was needed.

Local author/reviewer builds and passive discovery passed. Root tooling19,
project443/122/current HTML/whitespace and all9 workflow shell blocks/Python
heredoc syntax passed without new body execution. Current main ebb4368 is integrated;
all443 IDs/1735 edges/other completed states and reviews are preserved.

## Remaining gates and next assignment

Baseline is done by actual test review, SDK green, intended red and independent
attribution. Publication is separate: require every final completion-head CI check,
current rules/approvals/resolved discussions and normal PR82 merge. Only then start
SESSION-008 from integrated main with a fresh implementer, this accepted contract,
all39 unchanged assertions/fixtures, original receipts and exact review record.
Implementation needs passing acceptance, separate source review and stable CI.

Issuance/RNG, real SQL/current session source, trustworthy clock, accepted login,
permissions, full lifecycle/transport and all provider/platform/security/release
scope remain their owning tasks. Concurrent SESSIONCONTRACT-003 is document-only;
its branch handoff records the separate fresh author assignment. Root owns shared
state/HTML/checkpoint/CI/pins/publication, with explicit bounded harness delegation.
